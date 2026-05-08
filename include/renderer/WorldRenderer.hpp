#pragma once

#include "renderer/Bounds.hpp"
#include "renderer/ChunkMesh.hpp"
#include "renderer/Frustum.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

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

    struct MeshTask {
        std::uint64_t key;
        std::shared_ptr<Chunk> chunk;
        int sectionIndex;
        std::uint32_t requestedVersion = 0;
        int priority = 0;
    };

    struct MeshTaskPriority {
        bool operator()(const MeshTask& lhs, const MeshTask& rhs) const {
            return lhs.priority > rhs.priority;
        }
    };

    struct MeshResult {
        std::uint64_t key;
        ChunkMeshData meshData;
        std::uint32_t requestedVersion;
        std::uint32_t version;
        double buildMs;
    };

    World& m_world;
    std::map<std::uint64_t, SectionRenderEntry> m_sections;
    Stats m_stats;

    static std::uint64_t sectionKey(int cx, int cz, int sectionIndex);

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
