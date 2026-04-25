#include "renderer/WorldRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Shader.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <chrono>

WorldRenderer::WorldRenderer(World& world) : m_world(world), m_running(true) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    for (unsigned int i = 0; i < numThreads; ++i) {
        m_meshWorkers.emplace_back(&WorldRenderer::meshWorkerLoop, this);
    }
    rebuildSectionList();
}

WorldRenderer::~WorldRenderer() {
    m_running = false;
    m_cv.notify_all();
    for (auto& worker : m_meshWorkers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void WorldRenderer::meshWorkerLoop() {
    using clock = std::chrono::steady_clock;
    while (m_running) {
        MeshTask task;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_cv.wait(lock, [this] { return !m_taskQueue.empty() || !m_running; });
            if (!m_running) break;
            task = m_taskQueue.front();
            m_taskQueue.pop();
        }

        const auto buildStart = clock::now();
        ChunkMeshData meshData = ChunkMesher::buildSectionMesh(m_world, *task.chunk, task.sectionIndex);
        const auto buildEnd = clock::now();
        
        MeshResult result;
        result.key = task.key;
        result.meshData = std::move(meshData);
        result.version = task.chunk->getSectionVersion(task.sectionIndex);
        result.buildMs = std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();

        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(result));
        }
    }
}

void WorldRenderer::rebuildSectionList() {
    m_sections.clear();
    for (const auto& chunk : m_world.getChunks()) {
        addSectionsForChunk(chunk);
    }
}

void WorldRenderer::addSectionsForChunk(std::shared_ptr<Chunk> chunk) {
    if (!chunk || chunk->getState() == ChunkState::Empty) return;
    
    for (int sectionIndex = 0; sectionIndex < Chunk::SECTION_COUNT; ++sectionIndex) {
        std::uint64_t key = sectionKey(chunk->getX(), chunk->getZ(), sectionIndex);
        auto it = m_sections.find(key);
        if (it == m_sections.end()) {
            SectionRenderEntry entry;
            entry.chunk = chunk;
            entry.sectionIndex = sectionIndex;
            const float baseX = static_cast<float>(chunk->getX() * Chunk::WIDTH);
            const float baseY = static_cast<float>(Chunk::getSectionMinY(sectionIndex));
            const float baseZ = static_cast<float>(chunk->getZ() * Chunk::DEPTH);
            entry.bounds.min = {baseX, baseY, baseZ};
            entry.bounds.max = {
                baseX + static_cast<float>(Chunk::WIDTH),
                baseY + static_cast<float>(Chunk::SECTION_HEIGHT),
                baseZ + static_cast<float>(Chunk::DEPTH)
            };
            m_sections[key] = std::move(entry);
        } else {
            // Update chunk pointer in case it was replaced (e.g. Generated -> Complete)
            it->second.chunk = chunk;
        }
    }
    m_stats.sectionCount = m_sections.size();
}

void WorldRenderer::updateDirtyMeshes(int limit) {
    // 1. Process background results with a budget
    int resultsProcessed = 0;
    while (resultsProcessed < 64) { // Increased GL upload budget to 64 per frame
        MeshResult result;
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            if (m_resultQueue.empty()) break;
            result = std::move(m_resultQueue.front());
            m_resultQueue.pop();
        }

        auto it = m_sections.find(result.key);
        if (it != m_sections.end()) {
            it->second.bounds = result.meshData.bounds;
            it->second.mesh.upload(result.meshData.opaque);
            it->second.translucentMesh.upload(result.meshData.translucent);
            it->second.uploadedVersion = result.version;
            it->second.isBuilding = false;
        }

        ++m_stats.meshBuilds;
        m_stats.meshBuildMs += result.buildMs;
        resultsProcessed++;
    }

    // 2. Scan for new dirty sections if we have budget
    if (limit <= 0) return;

    int buildsStarted = 0;
    for (auto it = m_sections.begin(); it != m_sections.end();) {
        auto& entry = it->second;
        
        // Remove sections whose chunks are no longer in the world
        if (!m_world.isChunkLoaded(entry.chunk->getX(), entry.chunk->getZ())) {
            it = m_sections.erase(it);
            continue;
        }

        if (entry.chunk->isSectionDirty(entry.sectionIndex) && !entry.isBuilding) {
            ChunkState state = entry.chunk->getState();
            if (state != ChunkState::Empty && state != ChunkState::Generating) {
                entry.isBuilding = true;
                entry.chunk->clearSectionDirty(entry.sectionIndex);

                MeshTask task { it->first, entry.chunk, entry.sectionIndex };
                {
                    std::lock_guard<std::mutex> lock(m_taskMutex);
                    m_taskQueue.push(task);
                }
                m_cv.notify_one();
                if (++buildsStarted >= limit) {
                    ++it;
                    break;
                }
            }
        }
        ++it;
    }
}

void WorldRenderer::renderOpaque(const Frustum& frustum, Shader& shader) {
    m_stats.visibleSections = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();
    glDisable(GL_BLEND);
    for (const auto& [key, entry] : m_sections) {
        if (!entry.mesh.hasGeometry() || !frustum.intersects(entry.bounds)) continue;
        entry.mesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.mesh.getTriangleCount();
    }
}

void WorldRenderer::renderTranslucent(const Frustum& frustum, Shader& shader) {
    shader.use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // Culling re-enabled to prevent "fences" (seeing backfaces of the water mass from inside)
    glEnable(GL_CULL_FACE); 
    for (const auto& [key, entry] : m_sections) {
        if (!entry.translucentMesh.hasGeometry() || !frustum.intersects(entry.bounds)) continue;
        entry.translucentMesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.translucentMesh.getTriangleCount();
    }
    glDepthMask(GL_TRUE);
}




void WorldRenderer::renderDebug(const Frustum& frustum, Shader& shader, bool showChunkBoundaries) {
    if (!showChunkBoundaries) return;
    
    Tessellator* t = Tessellator::instance;
    shader.use();
    
    glEnable(GL_DEPTH_TEST);
    t->startDrawing(GL_LINES);
    t->setColorOpaque(255, 255, 0); // Yellow boundaries
    
    for (const auto& [key, entry] : m_sections) {
        if (entry.sectionIndex != 0) continue; // Use base section to find chunk pos
        
        float x = (float)(entry.chunk->getX() * 16);
        float z = (float)(entry.chunk->getZ() * 16);
        
        // Define AABB for the entire chunk column (0-128)
        AABB columnBounds = { {x, 0, z}, {x + 16, 128, z + 16} };
        if (!frustum.intersects(columnBounds)) continue;
        
        // Vertical lines
        for (int i = 0; i <= 16; i += 16) {
            for (int j = 0; j <= 16; j += 16) {
                t->addVertex(x + i, 0, z + j);
                t->addVertex(x + i, 128, z + j);
            }
        }
        // Horizontal lines every 16 blocks
        for (int y = 0; y <= 128; y += 16) {
            t->addVertex(x, y, z); t->addVertex(x + 16, y, z);
            t->addVertex(x + 16, y, z); t->addVertex(x + 16, y, z + 16);
            t->addVertex(x + 16, y, z + 16); t->addVertex(x, y, z + 16);
            t->addVertex(x, y, z + 16); t->addVertex(x, y, z);
        }
    }
    t->draw();
}

void WorldRenderer::removeFarSections(int playerCX, int playerCZ, int keepDistance) {
    auto it = m_sections.begin();
    while (it != m_sections.end()) {
        int cx = it->second.chunk->getX(), cz = it->second.chunk->getZ();
        if (std::abs(cx - playerCX) > keepDistance || std::abs(cz - playerCZ) > keepDistance) it = m_sections.erase(it);
        else ++it;
    }
}

std::uint64_t WorldRenderer::sectionKey(int cx, int cz, int sectionIndex) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 40) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz)) << 8) |
           (static_cast<std::uint64_t>(sectionIndex) & 0xFFu);
}
