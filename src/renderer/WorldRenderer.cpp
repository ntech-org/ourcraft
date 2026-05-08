#include "renderer/WorldRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Shader.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
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
            task = m_taskQueue.top();
            m_taskQueue.pop();
            m_queuedTasks.erase(task.key);
        }

        const auto buildStart = clock::now();
        ChunkMeshData meshData = ChunkMesher::buildSectionMesh(m_world, *task.chunk, task.sectionIndex);
        const auto buildEnd = clock::now();

        MeshResult result;
        result.key = task.key;
        result.meshData = std::move(meshData);
        result.requestedVersion = task.requestedVersion;
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
    for (const auto& chunk : m_world.getAllChunks()) {
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
            entry.uploadedVersion = chunk->getSectionVersion(sectionIndex);
            entry.bounds.min = {0.0f, 0.0f, 0.0f};
            entry.bounds.max = {16.0f, 16.0f, 16.0f};
            m_sections[key] = std::move(entry);
        } else {
            // Update chunk pointer in case it was replaced
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
            if (result.version != result.requestedVersion) {
                it->second.isBuilding = false;
                it->second.chunk->touchSection(it->second.sectionIndex);
                ++m_stats.meshBuilds;
                m_stats.meshBuildMs += result.buildMs;
                resultsProcessed++;
                continue;
            }
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
            uint32_t currentVersion = entry.chunk->getSectionVersion(entry.sectionIndex);

            ChunkState state = entry.chunk->getState();
            // Only rebuild if chunk is fully processed
            if (state == ChunkState::Complete || state == ChunkState::Decorated || state == ChunkState::Lighted) {
                entry.isBuilding = true;
                const std::uint64_t taskKey = it->first;
                bool enqueued = false;
                {
                    std::lock_guard<std::mutex> lock(m_taskMutex);
                    if (m_queuedTasks.insert(taskKey).second) {
                        MeshTask task;
                        task.key = taskKey;
                        task.chunk = entry.chunk;
                        task.sectionIndex = entry.sectionIndex;
                        task.requestedVersion = currentVersion;
                        task.priority = 0;
                        m_taskQueue.push(std::move(task));
                        enqueued = true;
                    }
                }
                if (!enqueued) {
                    entry.isBuilding = false;
                    ++it;
                    continue;
                }
                entry.chunk->clearSectionDirty(entry.sectionIndex);

                m_cv.notify_one();
                if (++buildsStarted >= limit) {
                    break;
                }
            } else {
                // Clear dirty flag but don't rebuild yet - chunk not ready
                entry.chunk->clearSectionDirty(entry.sectionIndex);
            }
        }
        ++it;
    }
}

void WorldRenderer::renderOpaque(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
    m_stats.visibleSections = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();
    glDisable(GL_BLEND);
    for (const auto& [key, entry] : m_sections) {
        glm::vec3 relativePos = glm::vec3(
            glm::dvec3(entry.chunk->getX() * 16, entry.sectionIndex * 16, entry.chunk->getZ() * 16) - cameraPos
        );
        if (!entry.mesh.hasGeometry() || !frustum.intersects(entry.bounds, relativePos)) continue;
        
        shader.setMat4("model", glm::translate(glm::mat4(1.0f), relativePos));
        entry.mesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.mesh.getTriangleCount();
    }
}

void WorldRenderer::renderTranslucent(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
    shader.use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // Culling re-enabled to prevent "fences" (seeing backfaces of the water mass from inside)
    glEnable(GL_CULL_FACE);
    for (const auto& [key, entry] : m_sections) {
        glm::vec3 relativePos = glm::vec3(
            glm::dvec3(entry.chunk->getX() * 16, entry.sectionIndex * 16, entry.chunk->getZ() * 16) - cameraPos
        );
        if (!entry.translucentMesh.hasGeometry() || !frustum.intersects(entry.bounds, relativePos)) continue;

        shader.setMat4("model", glm::translate(glm::mat4(1.0f), relativePos));
        entry.translucentMesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.translucentMesh.getTriangleCount();
    }
    glDepthMask(GL_TRUE);
}




void WorldRenderer::renderDebug(const Frustum& frustum, Shader& shader, bool showChunkBoundaries, const glm::dvec3& cameraPos) {
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
        
        glm::vec3 relativeChunkPos = glm::vec3(glm::dvec3(x, 0, z) - cameraPos);

        // Define AABB for the entire chunk column (0-128)
        AABB columnBounds = { {0, 0, 0}, {16, 128, 16} };
        if (!frustum.intersects(columnBounds, relativeChunkPos)) continue;

        shader.setMat4("model", glm::translate(glm::mat4(1.0f), relativeChunkPos));

        // Vertical lines
        for (int i = 0; i <= 16; i += 16) {
            for (int j = 0; j <= 16; j += 16) {
                t->addVertex(i, 0, j);
                t->addVertex(i, 128, j);
            }
        }
        // Horizontal lines every 16 blocks
        for (int y = 0; y <= 128; y += 16) {
            t->addVertex(0, y, 0); t->addVertex(16, y, 0);
            t->addVertex(16, y, 0); t->addVertex(16, y, 16);
            t->addVertex(16, y, 16); t->addVertex(0, y, 16);
            t->addVertex(0, y, 16); t->addVertex(0, y, 0);
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
