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
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <algorithm>
#include <array>
#include <memory>

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
        std::uint32_t buildingVersion = 0;
        ChunkMesh mesh;
        ChunkMesh translucentMesh;
        BatchedMesh::Allocation batchedAlloc;
        AABB bounds { glm::vec3(0.0f), glm::vec3(16.0f) };
        bool isBuilding = false;
        bool hasMesh = false;
    };

    struct ChunkColumn {
        std::shared_ptr<Chunk> chunk;
        int cx = 0;
        int cz = 0;
        std::array<SectionRenderEntry, Chunk::SECTION_COUNT> sections;
        AABB columnBounds { glm::vec3(0.0f), glm::vec3(16.0f, 128.0f, 16.0f) };
        bool needsCleanup = false;
    };

    struct MeshTask {
        std::shared_ptr<Chunk> chunk;
        int cx = 0;
        int cz = 0;
        int sectionIndex = 0;
        std::uint32_t requestedVersion = 0;
        std::uint32_t generation = 0;
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
        Chunk* chunkPtr = nullptr;
        ChunkMeshData meshData;
        std::uint32_t requestedVersion = 0;
        std::uint32_t version = 0;
        std::uint32_t generation = 0;
        double buildMs = 0.0;
    };

    // Tracks the single live in-flight build per section. Stale worker results
    // must not erase this unless they match exactly.
    struct InFlightInfo {
        Chunk* chunkPtr = nullptr;
        std::uint32_t version = 0;
        std::uint32_t generation = 0;
    };

    struct PendingMeshInfo {
        Chunk* chunkPtr = nullptr;
        std::uint32_t version = 0;
        int priority = 0;
    };

    World& m_world;
    std::vector<ChunkColumn> m_columns;
    std::unordered_map<std::uint64_t, std::size_t> m_columnIndex;
    Stats m_stats;
    int m_playerCX = 0;
    int m_playerCZ = 0;
    int m_maintainedPlayerCX = 0;
    int m_maintainedPlayerCZ = 0;
    int m_renderDistanceChunks = 12;
    std::uint32_t m_maintenanceFrame = 0;
    std::vector<SectionRenderEntry*> m_visibleOpaque;
    std::vector<SectionRenderEntry*> m_visibleTranslucent;
    std::uint32_t m_meshGeneration = 1;

    struct SectionKey {
        int cx = 0;
        int cz = 0;
        int si = 0;
        bool operator==(const SectionKey& o) const {
            return cx == o.cx && cz == o.cz && si == o.si;
        }
    };
    struct SectionKeyHash {
        std::size_t operator()(const SectionKey& k) const {
            std::size_t h = static_cast<std::size_t>(static_cast<std::uint32_t>(k.cx));
            h ^= static_cast<std::size_t>(static_cast<std::uint32_t>(k.cz)) + 0x9e3779b9u + (h << 6) + (h >> 2);
            h ^= static_cast<std::size_t>(static_cast<std::uint32_t>(k.si)) + 0x9e3779b9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    ChunkColumn* findColumn(int cx, int cz);
    const ChunkColumn* findColumn(int cx, int cz) const;
    static std::uint64_t columnKey(int cx, int cz);
    static SectionKey makeSectionKey(int cx, int cz, int si);
    void freeSectionMesh(SectionRenderEntry& entry);
    bool isSectionMeshReady(const Chunk& chunk) const;
    bool sectionHasRenderableMesh(const SectionRenderEntry& entry) const;
    void requestSectionMesh(SectionRenderEntry& entry);
    void queueSectionMesh(const std::shared_ptr<Chunk>& chunk, int sectionIndex, int urgency = 0);
    void processMeshResults(int maxResults);
    void enqueueDirtyMeshes(int limit);
    void cleanupRemovedColumns();
    void reconcileMissingColumns();

    void meshWorkerLoop();

    std::vector<std::thread> m_meshWorkers;
    std::priority_queue<MeshTask, std::vector<MeshTask>, MeshTaskPriority> m_taskQueue;
    std::priority_queue<MeshTask, std::vector<MeshTask>, MeshTaskPriority> m_pendingMeshQueue;
    std::unordered_map<SectionKey, PendingMeshInfo, SectionKeyHash> m_pendingMeshVersions;
    std::unordered_map<SectionKey, InFlightInfo, SectionKeyHash> m_inFlight;
    std::queue<MeshResult> m_resultQueue;

    std::mutex m_taskMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;

    BatchedMesh m_batchedOpaque;
    Shader* m_batchedShader = nullptr;
    GLuint m_sectionSSBO = 0;
    void* m_sectionSSBOPtr = nullptr;
    static constexpr std::size_t kSectionBufferCount = 3;
    std::array<GLsync, kSectionBufferCount> m_ssboFences {};
    std::size_t m_sectionBufferIndex = 0;
    std::vector<SectionGPUData> m_sectionGPUData;
    std::vector<BatchedMesh::DrawElementsIndirectCommand> m_opaqueCommands;

    void buildBatchedFrameData(const Frustum& frustum, const glm::dvec3& cameraPos);
};
