#include "renderer/WorldRenderer.hpp"
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
    for (const auto& chunkPtr : m_world.getChunks()) {
        std::shared_ptr<Chunk> chunk = chunkPtr;
        for (int sectionIndex = 0; sectionIndex < Chunk::SECTION_COUNT; ++sectionIndex) {
            std::uint64_t key = sectionKey(chunk->getX(), chunk->getZ(), sectionIndex);
            if (m_sections.find(key) == m_sections.end()) {
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
            }
        }
    }
    m_stats.sectionCount = m_sections.size();
}

void WorldRenderer::updateDirtyMeshes(int limit) {
    m_stats.sectionCount = m_sections.size();
    while (true) {
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
    }

    int buildsThisFrame = 0;
    for (auto& [key, entry] : m_sections) {
        if (!entry.chunk->isSectionDirty(entry.sectionIndex) || entry.isBuilding) continue;
        if (entry.chunk->getState() != ChunkState::Decorated && entry.chunk->getState() != ChunkState::Generated) continue;

        entry.isBuilding = true;
        entry.chunk->clearSectionDirty(entry.sectionIndex);

        MeshTask task { key, entry.chunk, entry.sectionIndex };
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_taskQueue.push(task);
        }
        m_cv.notify_one();
        if (limit > 0 && ++buildsThisFrame >= limit) break;
    }
}

void WorldRenderer::render(const Frustum& frustum, Shader& shader) {
    m_stats.visibleSections = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();

    // Pass 1: Opaque
    glDisable(GL_BLEND);
    for (const auto& [key, entry] : m_sections) {
        if (!entry.mesh.hasGeometry() || !frustum.intersects(entry.bounds)) continue;
        entry.mesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.mesh.getTriangleCount();
    }

    // Pass 2: Translucent
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Disable depth write to allow alpha stacking
    for (const auto& [key, entry] : m_sections) {
        if (!entry.translucentMesh.hasGeometry() || !frustum.intersects(entry.bounds)) continue;
        entry.translucentMesh.draw();
        m_stats.visibleSections++; m_stats.drawCalls++; m_stats.triangles += entry.translucentMesh.getTriangleCount();
    }
    glDepthMask(GL_TRUE);
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
