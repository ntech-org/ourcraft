#include "renderer/WorldRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Shader.hpp"
#include "util/Profiler.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <chrono>
#include <iostream>

WorldRenderer::WorldRenderer(World& world) : m_world(world), m_running(true) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    numThreads = std::min(numThreads > 2 ? numThreads - 2 : 1u, 4u);
    for (unsigned int i = 0; i < numThreads; ++i) {
        m_meshWorkers.emplace_back(&WorldRenderer::meshWorkerLoop, this);
    }
    m_world.enableSectionChangeTracking();
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
    for (auto& fence : m_ssboFences) {
        if (fence) glDeleteSync(fence);
        fence = nullptr;
    }
    if (m_sectionSSBO) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sectionSSBO);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glDeleteBuffers(1, &m_sectionSSBO);
    }
}

std::uint64_t WorldRenderer::columnKey(int cx, int cz) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) |
           static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz));
}

WorldRenderer::SectionKey WorldRenderer::makeSectionKey(int cx, int cz, int si) {
    return SectionKey{cx, cz, si};
}

WorldRenderer::ChunkColumn* WorldRenderer::findColumn(int cx, int cz) {
    auto it = m_columnIndex.find(columnKey(cx, cz));
    return it != m_columnIndex.end() ? &m_columns[it->second] : nullptr;
}

const WorldRenderer::ChunkColumn* WorldRenderer::findColumn(int cx, int cz) const {
    auto it = m_columnIndex.find(columnKey(cx, cz));
    return it != m_columnIndex.end() ? &m_columns[it->second] : nullptr;
}

void WorldRenderer::freeSectionMesh(SectionRenderEntry& entry) {
    if (m_batchedOpaque.isInitialized() && entry.batchedAlloc.valid) {
        m_batchedOpaque.free(entry.batchedAlloc);
        entry.batchedAlloc = {};
    }
    entry.mesh.clear();
    entry.translucentMesh.clear();
    entry.uploadedVersion = 0;
    entry.hasMesh = false;
}

bool WorldRenderer::isSectionMeshReady(const Chunk& chunk) const {
    ChunkState state = chunk.getState();
    return state == ChunkState::Complete;
}

bool WorldRenderer::sectionHasRenderableMesh(const SectionRenderEntry& entry) const {
    if (m_batchedOpaque.isInitialized()) {
        return entry.batchedAlloc.valid || entry.translucentMesh.hasGeometry();
    }
    return entry.mesh.hasGeometry() || entry.translucentMesh.hasGeometry();
}

void WorldRenderer::queueSectionMesh(const std::shared_ptr<Chunk>& chunk, int sectionIndex, int urgency) {
    if (!chunk || sectionIndex < 0 || sectionIndex >= Chunk::SECTION_COUNT) return;
    if (!isSectionMeshReady(*chunk)) return;

    const SectionKey key = makeSectionKey(chunk->getX(), chunk->getZ(), sectionIndex);
    const std::uint32_t version = chunk->getSectionVersion(sectionIndex);
    const int distance = std::max(std::abs(chunk->getX() - m_playerCX), std::abs(chunk->getZ() - m_playerCZ));
    const int priority = urgency * 100000 + distance;
    auto pending = m_pendingMeshVersions.find(key);
    if (pending != m_pendingMeshVersions.end()
        && pending->second.chunkPtr == chunk.get()
        && pending->second.version == version
        && pending->second.priority <= priority) return;

    MeshTask task;
    task.chunk = chunk;
    task.cx = chunk->getX();
    task.cz = chunk->getZ();
    task.sectionIndex = sectionIndex;
    task.requestedVersion = version;
    task.generation = m_meshGeneration;
    task.priority = priority;
    m_pendingMeshVersions[key] = PendingMeshInfo{chunk.get(), version, priority};
    m_pendingMeshQueue.push(std::move(task));
}

void WorldRenderer::requestSectionMesh(SectionRenderEntry& entry) {
    if (!entry.chunk) return;
    entry.isBuilding = false;
    entry.buildingVersion = 0;
    if (!entry.chunk->isSectionDirty(entry.sectionIndex)) {
        entry.chunk->touchSection(entry.sectionIndex);
    }
}

void WorldRenderer::initBatchedRendering() {
    if (!m_batchedOpaque.init(256 * 1024 * 1024, 64 * 1024 * 1024)) {
        std::cerr << "Failed to initialize batched mesh" << std::endl;
        return;
    }

    constexpr std::size_t maxSections = 100000;
    m_sectionGPUData.reserve(maxSections);
    m_opaqueCommands.reserve(maxSections);

    glGenBuffers(1, &m_sectionSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sectionSSBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
                      static_cast<GLsizeiptr>(kSectionBufferCount * maxSections * sizeof(SectionGPUData)),
                      nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT |
                               GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    m_sectionSSBOPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                          static_cast<GLsizeiptr>(kSectionBufferCount * maxSections * sizeof(SectionGPUData)),
                                          GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::cout << "Batched rendering initialized (VBO: 256MB, IBO: 64MB, SSBO: "
              << (maxSections * sizeof(SectionGPUData) / (1024 * 1024)) << "MB)" << std::endl;
}

void WorldRenderer::meshWorkerLoop() {
    OC_THREAD_NAME("MeshWorker");
    using clock = std::chrono::steady_clock;
    while (m_running) {
        MeshTask task;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_cv.wait(lock, [this] { return !m_taskQueue.empty() || !m_running; });
            if (!m_running) break;
            task = m_taskQueue.top();
            m_taskQueue.pop();
        }

        std::shared_ptr<Chunk> chunk = task.chunk;
        if (!chunk) continue;

        OC_ZONE_SCOPED_N("MeshSection");
        const auto buildStart = clock::now();
        ChunkMeshData meshData = ChunkMesher::buildSectionMesh(m_world, *chunk, task.sectionIndex);
        const auto buildEnd = clock::now();

        MeshResult result;
        result.cx = task.cx;
        result.cz = task.cz;
        result.sectionIndex = task.sectionIndex;
        result.chunkPtr = chunk.get();
        result.meshData = std::move(meshData);
        result.requestedVersion = task.requestedVersion;
        result.version = chunk->getSectionVersion(task.sectionIndex);
        result.generation = task.generation;
        result.buildMs = std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();

        {
            std::lock_guard<std::mutex> rlock(m_resultMutex);
            m_resultQueue.push(std::move(result));
        }
    }
}

void WorldRenderer::rebuildSectionList() {
    ++m_meshGeneration;
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        while (!m_taskQueue.empty()) m_taskQueue.pop();
        m_inFlight.clear();
    }
    while (!m_pendingMeshQueue.empty()) m_pendingMeshQueue.pop();
    m_pendingMeshVersions.clear();
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        while (!m_resultQueue.empty()) m_resultQueue.pop();
    }

    for (auto& col : m_columns) {
        for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
            freeSectionMesh(col.sections[si]);
        }
    }
    m_columns.clear();
    m_columnIndex.clear();
    for (const auto& chunk : m_world.getAllChunks()) {
        addSectionsForChunk(chunk);
    }
}

void WorldRenderer::addSectionsForChunk(std::shared_ptr<Chunk> chunk) {
    if (!chunk || chunk->getState() == ChunkState::Empty) return;

    int cx = chunk->getX(), cz = chunk->getZ();
    ChunkColumn* col = findColumn(cx, cz);
    const bool chunkReplaced = col && col->chunk && col->chunk.get() != chunk.get();
    if (!col) {
        std::size_t idx = m_columns.size();
        m_columns.emplace_back();
        col = &m_columns.back();
        col->cx = cx;
        col->cz = cz;
        col->columnBounds = { glm::vec3(0.0f), glm::vec3(16.0f, 128.0f, 16.0f) };
        m_columnIndex[columnKey(cx, cz)] = idx;
    }

    col->chunk = chunk;
    col->needsCleanup = false;

    for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
        auto& entry = col->sections[si];
        entry.chunk = chunk;
        entry.sectionIndex = si;
        entry.bounds = { glm::vec3(0.0f), glm::vec3(16.0f) };

        const uint32_t newVersion = chunk->getSectionVersion(si);
        const bool missingMesh = !entry.hasMesh && chunk->isSectionNonEmpty(si);
        const bool versionMismatch = entry.hasMesh && entry.uploadedVersion != newVersion;
        const bool needsMesh = chunkReplaced || missingMesh || versionMismatch || chunk->isSectionDirty(si);

        if (chunkReplaced) {
            freeSectionMesh(entry);
        }

        if (needsMesh) {
            // Keep matching in-flight work. The enqueue pass removes stale records
            // by chunk pointer, generation, and section version.
            requestSectionMesh(entry);
            queueSectionMesh(chunk, si, 1);
        } else {
            entry.isBuilding = false;
            entry.buildingVersion = 0;
        }
    }
    m_stats.sectionCount = m_columns.size() * Chunk::SECTION_COUNT;
}

void WorldRenderer::processMeshResults(int maxResults) {
    OC_ZONE_SCOPED;
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        if (m_resultQueue.empty()) return;
    }
    // One fence wait for the whole upload/free batch instead of per section.
    if (m_batchedOpaque.isInitialized()) {
        m_batchedOpaque.beginMutationBatch();
    }
    int resultsProcessed = 0;
    while (resultsProcessed < maxResults) {
        MeshResult result;
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            if (m_resultQueue.empty()) break;
            result = std::move(m_resultQueue.front());
            m_resultQueue.pop();
        }

        ++m_stats.meshBuilds;
        m_stats.meshBuildMs += result.buildMs;
        resultsProcessed++;

        const SectionKey key = makeSectionKey(result.cx, result.cz, result.sectionIndex);

        // Always drop matching in-flight entry for THIS exact build only.
        auto clearMatchingInFlight = [&]() {
            auto it = m_inFlight.find(key);
            if (it == m_inFlight.end()) return;
            if (it->second.chunkPtr == result.chunkPtr
                && it->second.version == result.requestedVersion
                && it->second.generation == result.generation) {
                m_inFlight.erase(it);
            }
        };

        if (result.generation != m_meshGeneration) {
            clearMatchingInFlight();
            continue;
        }

        ChunkColumn* col = findColumn(result.cx, result.cz);
        if (!col || !col->chunk || col->chunk.get() != result.chunkPtr) {
            clearMatchingInFlight();
            continue;
        }

        auto& entry = col->sections[result.sectionIndex];
        if (!entry.chunk || entry.chunk.get() != result.chunkPtr) {
            clearMatchingInFlight();
            continue;
        }

        clearMatchingInFlight();

        if (entry.isBuilding
            && entry.buildingVersion == result.requestedVersion
            && entry.chunk.get() == result.chunkPtr) {
            entry.isBuilding = false;
            entry.buildingVersion = 0;
        }

        const uint32_t liveVersion = entry.chunk->getSectionVersion(result.sectionIndex);
        if (result.version != result.requestedVersion || result.requestedVersion != liveVersion) {
            // Stale mesh data — keep existing GPU mesh, force rebuild
            requestSectionMesh(entry);
            queueSectionMesh(entry.chunk, result.sectionIndex);
            continue;
        }

        if (!isSectionMeshReady(*entry.chunk)) {
            requestSectionMesh(entry);
            queueSectionMesh(entry.chunk, result.sectionIndex, 1);
            continue;
        }

        entry.bounds = result.meshData.bounds;
        if (entry.bounds.max.x <= entry.bounds.min.x) {
            entry.bounds = { glm::vec3(0.0f), glm::vec3(16.0f) };
        }

        if (m_batchedOpaque.isInitialized()) {
            OC_ZONE_SCOPED_N("UploadOpaqueMesh");
            BatchedMesh::Allocation oldAlloc = entry.batchedAlloc;
            if (!result.meshData.opaque.indices.empty()) {
                entry.batchedAlloc = m_batchedOpaque.upload(
                    result.meshData.opaque.vertices, result.meshData.opaque.indices);
                if (!entry.batchedAlloc.valid) {
                    // Allocator full — keep old mesh and retry
                    entry.batchedAlloc = oldAlloc;
                    requestSectionMesh(entry);
                    continue;
                }
            } else {
                entry.batchedAlloc = {};
            }
            if (oldAlloc.valid) {
                m_batchedOpaque.free(oldAlloc);
            }
        } else {
            entry.mesh.upload(result.meshData.opaque);
        }

        entry.translucentMesh.upload(result.meshData.translucent);
        entry.uploadedVersion = result.version;
        entry.hasMesh = true;
        entry.isBuilding = false;
        entry.buildingVersion = 0;
    }
    if (m_batchedOpaque.isInitialized()) {
        m_batchedOpaque.endMutationBatch();
    }
}

void WorldRenderer::enqueueDirtyMeshes(int limit) {
    OC_ZONE_SCOPED;
    if (limit <= 0) return;

    for (auto& event : m_world.popDirtySections()) {
        auto live = m_world.getChunk(event.chunk->getX(), event.chunk->getZ());
        if (live && live.get() == event.chunk.get()) {
            queueSectionMesh(event.chunk, event.sectionIndex, event.urgency);
        }
    }

    int buildsStarted = 0;
    const std::size_t maxLiveTasks = std::max<std::size_t>(1, m_meshWorkers.size());
    std::vector<MeshTask> deferred;
    while (buildsStarted < limit && m_inFlight.size() < maxLiveTasks && !m_pendingMeshQueue.empty()) {
        MeshTask task = m_pendingMeshQueue.top();
        m_pendingMeshQueue.pop();

        const SectionKey key = makeSectionKey(task.cx, task.cz, task.sectionIndex);
        auto wanted = m_pendingMeshVersions.find(key);
        if (wanted == m_pendingMeshVersions.end()
            || wanted->second.chunkPtr != task.chunk.get()
            || wanted->second.version != task.requestedVersion
            || wanted->second.priority != task.priority) continue;

        ChunkColumn* col = findColumn(task.cx, task.cz);
        if (!col || !col->chunk || col->chunk.get() != task.chunk.get()) {
            m_pendingMeshVersions.erase(wanted);
            continue;
        }
        if (!isSectionMeshReady(*task.chunk)) {
            deferred.push_back(std::move(task));
            continue;
        }
        if (m_inFlight.count(key) > 0) {
            deferred.push_back(std::move(task));
            continue;
        }

        auto& entry = col->sections[task.sectionIndex];
        if (!task.chunk->isSectionNonEmpty(task.sectionIndex) && !entry.hasMesh) {
            task.chunk->clearSectionDirty(task.sectionIndex);
            entry.uploadedVersion = task.requestedVersion;
            entry.hasMesh = true;
            m_pendingMeshVersions.erase(wanted);
            continue;
        }

        task.chunk->clearSectionDirty(task.sectionIndex);
        task.requestedVersion = task.chunk->getSectionVersion(task.sectionIndex);
        task.generation = m_meshGeneration;
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_taskQueue.push(task);
        }
        m_pendingMeshVersions.erase(wanted);
        m_inFlight[key] = InFlightInfo{entry.chunk.get(), task.requestedVersion, m_meshGeneration};
        entry.isBuilding = true;
        entry.buildingVersion = task.requestedVersion;
        ++buildsStarted;
    }

    for (auto& task : deferred) m_pendingMeshQueue.push(std::move(task));
    if (buildsStarted == 1) m_cv.notify_one();
    else if (buildsStarted > 1) m_cv.notify_all();
}

void WorldRenderer::reconcileMissingColumns() {
    OC_ZONE_SCOPED;
    // Ensure every loaded chunk near the player has a render column.
    // Fixes cases where popNewChunks was consumed elsewhere or a reload was missed.
    const int keepDist = m_renderDistanceChunks + 2;
    for (const auto& chunk : m_world.getAllChunks()) {
        if (!chunk) continue;
        const int cx = chunk->getX();
        const int cz = chunk->getZ();
        if (std::abs(cx - m_playerCX) > keepDist || std::abs(cz - m_playerCZ) > keepDist) continue;

        ChunkColumn* col = findColumn(cx, cz);
        if (!col || col->chunk.get() != chunk.get()) {
            addSectionsForChunk(chunk);
            continue;
        }

        // Existing column: ensure non-empty sections without meshes get dirtied
        if (!isSectionMeshReady(*chunk)) continue;
        for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
            auto& entry = col->sections[si];
            if (!entry.chunk) {
                entry.chunk = chunk;
                entry.sectionIndex = si;
            }
            if (chunk->isSectionNonEmpty(si) && !entry.hasMesh && !entry.isBuilding) {
                requestSectionMesh(entry);
                queueSectionMesh(chunk, si, 1);
            }
        }
    }
}

void WorldRenderer::cleanupRemovedColumns() {
    removeFarSections(m_playerCX, m_playerCZ, m_renderDistanceChunks + 2);

    const std::size_t sizeBefore = m_columns.size();
    auto newEnd = std::remove_if(m_columns.begin(), m_columns.end(),
        [this](ChunkColumn& col) {
            if (col.needsCleanup || !col.chunk || !m_world.isChunkLoaded(col.cx, col.cz)) {
                for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
                    freeSectionMesh(col.sections[si]);
                    m_inFlight.erase(makeSectionKey(col.cx, col.cz, si));
                }
                return true;
            }
            return false;
        });
    m_columns.erase(newEnd, m_columns.end());

    if (m_columns.size() != sizeBefore || m_columnIndex.size() != m_columns.size()) {
        m_columnIndex.clear();
        m_columnIndex.reserve(m_columns.size() * 2);
        for (std::size_t i = 0; i < m_columns.size(); ++i) {
            m_columnIndex[columnKey(m_columns[i].cx, m_columns[i].cz)] = i;
        }
        m_stats.sectionCount = m_columns.size() * Chunk::SECTION_COUNT;
    }
}

void WorldRenderer::updateDirtyMeshes(int limit) {
    OC_ZONE_SCOPED;
    processMeshResults(128);
    enqueueDirtyMeshes(limit);

    const bool playerChunkChanged = m_playerCX != m_maintainedPlayerCX || m_playerCZ != m_maintainedPlayerCZ;
    if (playerChunkChanged || (++m_maintenanceFrame % 120) == 0) {
        reconcileMissingColumns();
        cleanupRemovedColumns();
        m_maintainedPlayerCX = m_playerCX;
        m_maintainedPlayerCZ = m_playerCZ;
    }
    OC_PLOT("MeshInFlight", m_inFlight.size());
    OC_PLOT("MeshColumns", m_columns.size());
}

void WorldRenderer::updateVisibleSections(const Frustum& frustum, const glm::dvec3& cameraPos) {
    OC_ZONE_SCOPED;
    m_visibleOpaque.clear();
    m_visibleTranslucent.clear();
    m_stats.visibleSections = 0;

    const int rd = m_renderDistanceChunks;
    const int rdPlus1 = rd + 1;

    for (auto& col : m_columns) {
        if (!col.chunk) continue;

        int dcx = col.cx - m_playerCX;
        int dcz = col.cz - m_playerCZ;
        if (std::abs(dcx) > rdPlus1 || std::abs(dcz) > rdPlus1) continue;

        glm::vec3 colRelative = glm::vec3(
            glm::dvec3(col.cx * 16, 0, col.cz * 16) - cameraPos
        );
        if (!frustum.intersects(col.columnBounds, colRelative)) continue;

        for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
            auto& entry = col.sections[si];
            if (!entry.chunk) continue;
            if (!sectionHasRenderableMesh(entry)) continue;

            glm::vec3 relativePos = glm::vec3(
                glm::dvec3(col.cx * 16, si * 16, col.cz * 16) - cameraPos
            );

            // Use full section AABB if bounds are degenerate
            AABB bounds = entry.bounds;
            if (bounds.max.x <= bounds.min.x || bounds.max.y <= bounds.min.y || bounds.max.z <= bounds.min.z) {
                bounds = { glm::vec3(0.0f), glm::vec3(16.0f) };
            }
            if (!frustum.intersects(bounds, relativePos)) continue;

            bool hasOpaque = m_batchedOpaque.isInitialized()
                ? entry.batchedAlloc.valid
                : entry.mesh.hasGeometry();
            if (hasOpaque) {
                m_visibleOpaque.push_back(&entry);
            }
            if (entry.translucentMesh.hasGeometry()) {
                m_visibleTranslucent.push_back(&entry);
            }
            m_stats.visibleSections++;
        }
    }

    if (m_visibleTranslucent.size() > 1) {
        std::sort(m_visibleTranslucent.begin(), m_visibleTranslucent.end(),
            [&cameraPos](const SectionRenderEntry* a, const SectionRenderEntry* b) {
                double dxA = a->chunk->getX() * 16 + 8 - cameraPos.x;
                double dyA = a->sectionIndex * 16 + 8 - cameraPos.y;
                double dzA = a->chunk->getZ() * 16 + 8 - cameraPos.z;
                double dxB = b->chunk->getX() * 16 + 8 - cameraPos.x;
                double dyB = b->sectionIndex * 16 + 8 - cameraPos.y;
                double dzB = b->chunk->getZ() * 16 + 8 - cameraPos.z;
                double distSqA = dxA * dxA + dyA * dyA + dzA * dzA;
                double distSqB = dxB * dxB + dyB * dyB + dzB * dzB;
                return distSqA > distSqB;
            });
    }
}

void WorldRenderer::buildBatchedFrameData(const Frustum& frustum, const glm::dvec3& cameraPos) {
    (void)frustum;
    m_sectionGPUData.clear();
    m_opaqueCommands.clear();

    for (const auto* entry : m_visibleOpaque) {
        if (!entry->batchedAlloc.valid) continue;

        const glm::dvec3 sectionOriginD(entry->chunk->getX() * 16, entry->sectionIndex * 16, entry->chunk->getZ() * 16);
        glm::vec3 relativeCenter = glm::vec3(sectionOriginD - cameraPos);
        SectionGPUData gpuData;
        gpuData.model = glm::translate(glm::mat4(1.0f), relativeCenter);
        gpuData.aabbMin = glm::vec4(entry->bounds.min + relativeCenter, 1.0f);
        gpuData.aabbMax = glm::vec4(entry->bounds.max + relativeCenter, 1.0f);

        std::uint32_t ssboIndex = static_cast<std::uint32_t>(m_sectionGPUData.size());
        m_sectionGPUData.push_back(gpuData);

        BatchedMesh::DrawElementsIndirectCommand cmd;
        cmd.count = entry->batchedAlloc.indexCount;
        cmd.instanceCount = 1;
        cmd.firstIndex = entry->batchedAlloc.iboOffset / sizeof(std::uint32_t);
        cmd.baseVertex = static_cast<std::int32_t>(entry->batchedAlloc.vboOffset / sizeof(TerrainVertex));
        cmd.baseInstance = ssboIndex;
        m_opaqueCommands.push_back(cmd);
    }
}

void WorldRenderer::renderOpaque(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
    if (m_batchedOpaque.isInitialized()) {
        renderOpaqueBatched(frustum, shader, cameraPos);
        return;
    }

    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();
    glDisable(GL_BLEND);
    for (const auto* entry : m_visibleOpaque) {
        glm::vec3 relativePos = glm::vec3(
            glm::dvec3(entry->chunk->getX() * 16, entry->sectionIndex * 16, entry->chunk->getZ() * 16) - cameraPos
        );
        shader.setMat4("model", glm::translate(glm::mat4(1.0f), relativePos));
        entry->mesh.draw();
        m_stats.drawCalls++;
        m_stats.triangles += entry->mesh.getTriangleCount();
    }
}

void WorldRenderer::renderOpaqueBatched(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
    OC_ZONE_SCOPED;
    (void)shader;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    if (m_visibleOpaque.empty()) return;

    buildBatchedFrameData(frustum, cameraPos);

    if (m_opaqueCommands.empty()) return;

    m_batchedShader->use();
    glDisable(GL_BLEND);

    constexpr std::size_t maxSections = 100000;
    const std::size_t uploadCount = std::min(m_sectionGPUData.size(), maxSections);
    const std::size_t drawCount = std::min(m_opaqueCommands.size(), uploadCount);

    const std::size_t bufferIndex = m_sectionBufferIndex++ % kSectionBufferCount;
    GLsync& bufferFence = m_ssboFences[bufferIndex];
    if (bufferFence) {
        GLenum waitResult = GL_TIMEOUT_EXPIRED;
        while (waitResult != GL_ALREADY_SIGNALED && waitResult != GL_CONDITION_SATISFIED) {
            waitResult = glClientWaitSync(bufferFence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
            if (waitResult == GL_WAIT_FAILED) {
                glFinish();
                break;
            }
        }
        glDeleteSync(bufferFence);
        bufferFence = nullptr;
    }

    const std::size_t regionSize = maxSections * sizeof(SectionGPUData);
    const std::size_t regionOffset = bufferIndex * regionSize;
    if (m_sectionSSBOPtr && uploadCount > 0) {
        std::memcpy(static_cast<std::byte*>(m_sectionSSBOPtr) + regionOffset, m_sectionGPUData.data(),
                      uploadCount * sizeof(SectionGPUData));
    }
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_sectionSSBO,
                      static_cast<GLintptr>(regionOffset), static_cast<GLsizeiptr>(regionSize));

    m_batchedOpaque.bind();
    m_batchedOpaque.drawIndirect(m_opaqueCommands.data(), drawCount);

    bufferFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    m_stats.drawCalls = 1;
    for (const auto* entry : m_visibleOpaque) {
        if (entry->batchedAlloc.valid) {
            m_stats.triangles += entry->batchedAlloc.indexCount / 3;
        }
    }
}

void WorldRenderer::renderTranslucent(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
    (void)frustum;
    shader.use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    for (const auto* entry : m_visibleTranslucent) {
        glm::vec3 relativePos = glm::vec3(
            glm::dvec3(entry->chunk->getX() * 16, entry->sectionIndex * 16, entry->chunk->getZ() * 16) - cameraPos
        );
        shader.setMat4("model", glm::translate(glm::mat4(1.0f), relativePos));
        entry->translucentMesh.draw();
        m_stats.drawCalls++;
        m_stats.triangles += entry->translucentMesh.getTriangleCount();
    }
    glDepthMask(GL_TRUE);
}

void WorldRenderer::renderDebug(const Frustum& frustum, Shader& shader, bool showChunkBoundaries, const glm::dvec3& cameraPos) {
    if (!showChunkBoundaries) return;

    Tessellator* t = Tessellator::instance;
    shader.use();

    glEnable(GL_DEPTH_TEST);
    t->startDrawing(GL_LINES);
    t->setColorOpaque(255, 255, 0);

    for (const auto& col : m_columns) {
        if (!col.chunk) continue;

        float x = (float)(col.cx * 16);
        float z = (float)(col.cz * 16);

        glm::vec3 relativeChunkPos = glm::vec3(glm::dvec3(x, 0, z) - cameraPos);

        AABB columnBounds = { {0, 0, 0}, {16, 128, 16} };
        if (!frustum.intersects(columnBounds, relativeChunkPos)) continue;

        shader.setMat4("model", glm::mat4(1.0f));
        t->setTranslation(relativeChunkPos.x, relativeChunkPos.y, relativeChunkPos.z);

        for (int i = 0; i <= 16; i += 16) {
            for (int j = 0; j <= 16; j += 16) {
                t->addVertex(i, 0, j);
                t->addVertex(i, 128, j);
            }
        }
        for (int y = 0; y <= 128; y += 16) {
            t->addVertex(0, y, 0); t->addVertex(16, y, 0);
            t->addVertex(16, y, 0); t->addVertex(16, y, 16);
            t->addVertex(16, y, 16); t->addVertex(0, y, 16);
            t->addVertex(0, y, 16); t->addVertex(0, y, 0);
        }
    }
    t->setTranslation(0, 0, 0);
    t->draw();
}

void WorldRenderer::removeFarSections(int playerCX, int playerCZ, int keepDistance) {
    for (auto& col : m_columns) {
        if (std::abs(col.cx - playerCX) > keepDistance || std::abs(col.cz - playerCZ) > keepDistance) {
            col.needsCleanup = true;
        }
    }
}
