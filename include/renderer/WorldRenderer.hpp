#pragma once

#include "renderer/BatchedMesh.hpp"
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

    struct SectionGPUData {
        glm::mat4 model;
        glm::vec4 aabbMin;
        glm::vec4 aabbMax;
    };

    explicit WorldRenderer(World& world);
    ~WorldRenderer();

    void rebuildSectionList();
    void addSectionsForChunk(std::shared_ptr<Chunk> chunk);
    void updateDirtyMeshes(int limit = 4);
    void renderOpaque(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos);
    void renderOpaqueBatched(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos);
    void renderTranslucent(const Frustum& frustum, Shader& shader, const glm::dvec3& cameraPos);
    void renderDebug(const Frustum& frustum, Shader& shader, bool showChunkBoundaries, const glm::dvec3& cameraPos);

    void removeFarSections(int playerCX, int playerCZ, int keepDistance);
    const Stats& getStats() const { return m_stats; }

    void setPlayerChunkPosition(int playerCX, int playerCZ) {
        m_playerCX = playerCX;
        m_playerCZ = playerCZ;
    }
    void setRenderDistanceChunks(int chunks) { m_renderDistanceChunks = chunks; }
    void updateVisibleSections(const Frustum& frustum, const glm::dvec3& cameraPos);

    void initBatchedRendering();
    void setBatchedShader(Shader* shader) { m_batchedShader = shader; }

    bool useBatchedRendering() const { return m_batchedOpaque.isInitialized(); }

private:
    struct SectionRenderEntry {
        std::shared_ptr<Chunk> chunk;
        int sectionIndex = 0;
        std::uint32_t uploadedVersion = 0;
        ChunkMesh mesh;
        ChunkMesh translucentMesh;
        BatchedMesh::Allocation batchedAlloc;
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
        std::shared_ptr<Chunk> chunk;
        int cx = 0;
        int cz = 0;
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
    static std::uint64_t sectionKey(int cx, int cz, int si);

    void meshWorkerLoop();

    std::vector<std::thread> m_meshWorkers;
    std::priority_queue<MeshTask, std::vector<MeshTask>, MeshTaskPriority> m_taskQueue;
    std::unordered_set<std::uint64_t> m_queuedTasks;
    std::queue<MeshResult> m_resultQueue;

    std::mutex m_taskMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;

// Batched rendering
    BatchedMesh m_batchedOpaque;
    Shader* m_batchedShader = nullptr;
    GLuint m_sectionSSBO = 0;
    void* m_sectionSSBOPtr = nullptr;
    GLsync m_ssboFence = nullptr;
    std::vector<SectionGPUData> m_sectionGPUData;
    std::vector<BatchedMesh::DrawElementsIndirectCommand> m_opaqueCommands;

    static constexpr std::size_t kMaxQueuedTasks = 512;
    static constexpr std::size_t kMaxResultQueue = 1024;

    void buildBatchedFrameData(const Frustum& frustum, const glm::dvec3& cameraPos);
};
