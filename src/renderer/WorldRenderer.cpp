#include "renderer/WorldRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Shader.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
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

std::uint64_t WorldRenderer::columnKey(int cx, int cz) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz)) & 0xFFFFFFFFu);
}

WorldRenderer::ChunkColumn* WorldRenderer::findColumn(int cx, int cz) {
    auto it = m_columnIndex.find(columnKey(cx, cz));
    return it != m_columnIndex.end() ? &m_columns[it->second] : nullptr;
}

const WorldRenderer::ChunkColumn* WorldRenderer::findColumn(int cx, int cz) const {
    auto it = m_columnIndex.find(columnKey(cx, cz));
    return it != m_columnIndex.end() ? &m_columns[it->second] : nullptr;
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
        }

        ChunkColumn* col = task.column;
        const auto buildStart = clock::now();
        ChunkMeshData meshData = ChunkMesher::buildSectionMesh(m_world, *col->chunk, task.sectionIndex);
        const auto buildEnd = clock::now();

        MeshResult result;
        result.cx = col->cx;
        result.cz = col->cz;
        result.sectionIndex = task.sectionIndex;
        result.meshData = std::move(meshData);
        result.requestedVersion = task.requestedVersion;
        result.version = col->chunk->getSectionVersion(task.sectionIndex);
        result.buildMs = std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();

        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(result));
        }
    }
}

void WorldRenderer::rebuildSectionList() {
    m_columns.clear();
    m_columnIndex.clear();
    for (const auto& chunk : m_world.getAllChunks()) {
        addSectionsForChunk(chunk);
    }
}

void WorldRenderer::addSectionsForChunk(std::shared_ptr<Chunk> chunk) {
    if (!chunk || chunk->getState() == ChunkState::Empty) return;

    chunk->computeWaterLevels();

    int cx = chunk->getX(), cz = chunk->getZ();
    ChunkColumn* col = findColumn(cx, cz);
    if (!col) {
        std::size_t idx = m_columns.size();
        m_columns.emplace_back();
        col = &m_columns.back();
        col->cx = cx;
        col->cz = cz;
        col->columnBounds.min = {0.0f, 0.0f, 0.0f};
        col->columnBounds.max = {16.0f, 128.0f, 16.0f};
        m_columnIndex[columnKey(cx, cz)] = idx;
    }

    col->chunk = chunk;
    for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
        auto& entry = col->sections[si];
        entry.chunk = chunk;
        if (entry.uploadedVersion == 0) {
            entry.sectionIndex = si;
            entry.uploadedVersion = chunk->getSectionVersion(si);
            entry.bounds.min = {0.0f, 0.0f, 0.0f};
            entry.bounds.max = {16.0f, 16.0f, 16.0f};
        }
    }
    m_stats.sectionCount = m_columns.size() * Chunk::SECTION_COUNT;
}

void WorldRenderer::updateDirtyMeshes(int limit) {
    int resultsProcessed = 0;
    while (resultsProcessed < 64) {
        MeshResult result;
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            if (m_resultQueue.empty()) break;
            result = std::move(m_resultQueue.front());
            m_resultQueue.pop();
        }

        ChunkColumn* col = findColumn(result.cx, result.cz);
        if (col) {
            auto& entry = col->sections[result.sectionIndex];
            if (result.version != result.requestedVersion) {
                entry.isBuilding = false;
                entry.chunk->touchSection(entry.sectionIndex);
                ++m_stats.meshBuilds;
                m_stats.meshBuildMs += result.buildMs;
                resultsProcessed++;
                continue;
            }
            entry.bounds = result.meshData.bounds;
            entry.mesh.upload(result.meshData.opaque);
            entry.translucentMesh.upload(result.meshData.translucent);
            entry.uploadedVersion = result.version;
            entry.isBuilding = false;
        }

        ++m_stats.meshBuilds;
        m_stats.meshBuildMs += result.buildMs;
        resultsProcessed++;
    }

    if (limit <= 0) return;

    int buildsStarted = 0;
    for (auto& col : m_columns) {
        if (!col.chunk) continue;

        if (!m_world.isChunkLoaded(col.cx, col.cz)) {
            col.needsCleanup = true;
            continue;
        }

        if (std::abs(col.cx - m_playerCX) > m_renderDistanceChunks + 2 ||
            std::abs(col.cz - m_playerCZ) > m_renderDistanceChunks + 2) {
            continue;
        }

        for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
            auto& entry = col.sections[si];
            if (!entry.chunk) continue;
            if (!entry.chunk->isSectionDirty(si) || entry.isBuilding) continue;

            uint32_t currentVersion = entry.chunk->getSectionVersion(si);
            ChunkState state = entry.chunk->getState();
            if (state != ChunkState::Complete && state != ChunkState::Decorated && state != ChunkState::Lighted) {
                entry.chunk->clearSectionDirty(si);
                continue;
            }

            entry.isBuilding = true;
            bool enqueued = false;
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                if (m_queuedTasks.size() < 512) {
                    MeshTask task;
                    task.column = &col;
                    task.sectionIndex = si;
                    task.requestedVersion = currentVersion;
                    int distX = std::abs(col.cx - m_playerCX);
                    int distZ = std::abs(col.cz - m_playerCZ);
                    task.priority = std::max(distX, distZ);
                    m_taskQueue.push(std::move(task));
                    enqueued = true;
                }
            }
            if (!enqueued) {
                entry.isBuilding = false;
                continue;
            }
            entry.chunk->clearSectionDirty(si);
            m_cv.notify_one();
            if (++buildsStarted >= limit) break;
        }
        if (buildsStarted >= limit) break;
    }

    // Cleanup columns whose chunks were unloaded
    m_columns.erase(
        std::remove_if(m_columns.begin(), m_columns.end(),
            [this](const ChunkColumn& col) {
                if (col.needsCleanup) return true;
                if (!col.chunk) return false;
                return !m_world.isChunkLoaded(col.cx, col.cz);
            }),
        m_columns.end());
    // Rebuild index after erase
    m_columnIndex.clear();
    for (std::size_t i = 0; i < m_columns.size(); ++i) {
        m_columnIndex[columnKey(m_columns[i].cx, m_columns[i].cz)] = i;
    }
}

void WorldRenderer::updateVisibleSections(const Frustum& frustum, const glm::dvec3& cameraPos) {
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

        // Column-level frustum test (1 test per 8 sections)
        glm::vec3 colRelative = glm::vec3(
            glm::dvec3(col.cx * 16, 0, col.cz * 16) - cameraPos
        );
        if (!frustum.intersects(col.columnBounds, colRelative)) continue;

        for (int si = 0; si < Chunk::SECTION_COUNT; ++si) {
            auto& entry = col.sections[si];
            if (!entry.chunk) continue;

            glm::vec3 relativePos = glm::vec3(
                glm::dvec3(col.cx * 16, si * 16, col.cz * 16) - cameraPos
            );

            // Section-level frustum test only if column passed
            if (!frustum.intersects(entry.bounds, relativePos)) continue;

            if (entry.mesh.hasGeometry()) {
                m_visibleOpaque.push_back(&entry);
            }
            if (entry.translucentMesh.hasGeometry()) {
                m_visibleTranslucent.push_back(&entry);
            }
            m_stats.visibleSections++;
        }
    }

    std::sort(m_visibleTranslucent.begin(), m_visibleTranslucent.end(),
        [&cameraPos](const SectionRenderEntry* a, const SectionRenderEntry* b) {
            double distA = glm::length(glm::dvec3(
                a->chunk->getX() * 16 + 8, a->sectionIndex * 16 + 8, a->chunk->getZ() * 16 + 8) - cameraPos);
            double distB = glm::length(glm::dvec3(
                b->chunk->getX() * 16 + 8, b->sectionIndex * 16 + 8, b->chunk->getZ() * 16 + 8) - cameraPos);
            return distA > distB;
        });
}

void WorldRenderer::renderOpaque(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
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

void WorldRenderer::renderTranslucent(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos) {
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
