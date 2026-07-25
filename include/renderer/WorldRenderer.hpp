#pragma once

#include "renderer/Bounds.hpp"
#include "renderer/ChunkMesh.hpp"
#include "renderer/Frustum.hpp"
#include "world/Chunk.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <algorithm>

class Shader;
class World;
class Chunk;

class WorldRenderer {
public:
    struct Stats {
        std::size_t sectionCount = 0;
        std::size_t visibleSections = 0;
        std::size_t drawCalls = 0;
        std::size_t triangles = 0;
        std::size_t meshBuilds = 0;
        double meshBuildMs = 0.0;
    };

    explicit WorldRenderer(World& world);
    ~WorldRenderer();

    void rebuildSectionList();
    void addSectionsForChunk(std::shared_ptr<Chunk> chunk);
    void updateDirtyMeshes(int limit = 4);
    void renderOpaque(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos);
    void renderTranslucent(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos);
    void renderDebug(const Frustum& frustum, Shader& shader, bool showChunkBoundaries, const glm::dvec3& cameraPos);

    void removeFarSections(int playerCX, int playerCZ, int keepDistance);
    void setPlayerChunkPosition(int playerCX, int playerCZ) {
        m_playerCX = playerCX;
        m_playerCZ = playerCZ;
    }
    void setRenderDistanceChunks(int chunks) { m_renderDistanceChunks = chunks; }
    void updateVisibleSections(const Frustum& frustum, const glm::dvec3& cameraPos);

    const Stats& getStats() const { return m_stats; }

private:
    struct SectionRenderEntry {
        std::shared_ptr<Chunk> chunk;
        int sectionIndex = 0;
        std::uint32_t uploadedVersion = 0;
        ChunkMesh mesh;
        ChunkMesh translucentMesh;
        AABB bounds {};
        bool isBuilding = false;
    };

    struct ChunkColumn {
        std::shared_ptr<Chunk> chunk;
        int cx = 0;
        int cz = 0;
        std::array<SectionRenderEntry, Chunk::SECTION_COUNT> sections;
        AABB columnBounds {};
        bool needsCleanup = false;
    };

    struct MeshTask {
        ChunkColumn* column = nullptr;
        int sectionIndex = 0;
        std::uint32_t requestedVersion = 0;
        int priority = 0;
    };

    struct MeshTaskPriority {
        bool operator()(const MeshTask& lhs, const MeshTask& rhs) const {
            return lhs.priority > rhs.priority;
        }
    };

    struct MeshResult {
        int cx = 0;
        int cz = 0;
        int sectionIndex = 0;
        ChunkMeshData meshData;
        std::uint32_t requestedVersion;
        std::uint32_t version;
        double buildMs;
    };

    World& m_world;
    std::vector<ChunkColumn> m_columns;
    std::unordered_map<std::uint64_t, std::size_t> m_columnIndex;
    Stats m_stats;
    int m_playerCX = 0;
    int m_playerCZ = 0;
    int m_renderDistanceChunks = 12;
    std::vector<SectionRenderEntry*> m_visibleOpaque;
    std::vector<SectionRenderEntry*> m_visibleTranslucent;

    ChunkColumn* findColumn(int cx, int cz);
    const ChunkColumn* findColumn(int cx, int cz) const;
    static std::uint64_t columnKey(int cx, int cz);

    void meshWorkerLoop();

    std::vector<std::thread> m_meshWorkers;
    std::priority_queue<MeshTask, std::vector<MeshTask>, MeshTaskPriority> m_taskQueue;
    std::unordered_set<std::uint64_t> m_queuedTasks;
    std::queue<MeshResult> m_resultQueue;

    std::mutex m_taskMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
};
