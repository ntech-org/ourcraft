#pragma once

#include "renderer/Bounds.hpp"
#include "renderer/ChunkMesh.hpp"
#include "renderer/Frustum.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

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

    void rebuildSectionList();
    void updateDirtyMeshes();
    void render(const Frustum& frustum, Shader& shader);

    const Stats& getStats() const { return m_stats; }

private:
    struct SectionRenderEntry {
        Chunk* chunk = nullptr;
        int sectionIndex = 0;
        std::uint32_t uploadedVersion = 0;
        ChunkMesh mesh;
        AABB bounds {};
    };

    World& m_world;
    std::vector<SectionRenderEntry> m_sections;
    Stats m_stats;
};
