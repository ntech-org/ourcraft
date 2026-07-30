#pragma once

#include "renderer/ChunkMesh.hpp"
#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"
#include "world/Material.hpp"
#include "world/World.hpp"
#include <array>

class World;

enum class FaceDirection : uint8_t { Down = 0, Up = 1, North = 2, South = 3, West = 4, East = 5 };

constexpr std::uint32_t kWhiteColor = 0xFFFFFFFFu;

inline void appendIndices(ChunkMeshData::Pass& p) {
    std::uint32_t b = (std::uint32_t)p.vertices.size() - 4;
    p.indices.insert(p.indices.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
    p.quadCount++;
}

// Scans upward from y+1, skipping solid blocks, to find water surface depth.
// Correctly handles blocks nested under other solid blocks underwater.
inline float getUnderwaterDepth(const IBlockAccess& n, int x, int y, int z) {
    for (int i = y + 1; i < y + 65 && i < Chunk::HEIGHT; ++i) {
        uint8_t bid = n.getBlockID(x, i, z);
        if (bid == 0) continue;
        if (bid == 8 || bid == 9) return 1.0f;
    }
    return 0.0f;
}

// Precompute water surface Y per (lx,lz) column once per section for O(1) lookup.
inline float computeUnderwaterDepth(const World& world, int baseX, int baseZ, int lx, int lz, int si) {
    float d = 0;
    bool foundWater = false;
    for (int y = si * 16 + 15; y >= si * 16; --y) {
        uint8_t bid = world.getBlockID(baseX + lx, y, baseZ + lz);
        if (bid == 0) continue;
        if (world.getBlockMaterial(baseX + lx, y, baseZ + lz).isLiquid()) {
            if (foundWater) d += 1.0f;
            foundWater = true;
        } else if (foundWater) {
            break;
        }
    }
    return foundWater ? d : 0.0f;
}

// Scans in a direction from (x,y,z) to count continuous water blocks.
// Used for side face water depth.
inline float getWaterDepth(const IBlockAccess& n, int x, int y, int z) {
    float d = 0;
    for (int i = 0; i < 64 && (y + i) < Chunk::HEIGHT; ++i)
        if (n.getBlockMaterial(x, y + i, z).isLiquid()) d += 1.0f;
    return d;
}

class MeshingSnapshot final : public IBlockAccess {
public:
    static constexpr int HALO_SIZE = 18;
    static constexpr int HALO_HEIGHT = Chunk::SECTION_HEIGHT + 2;
    static constexpr int CELL_COUNT = HALO_SIZE * HALO_SIZE * HALO_HEIGHT;

    MeshingSnapshot(const World& world, int chunkX, int chunkZ, int sectionIndex);

    uint8_t getBlockID(int x, int y, int z) const override {
        if (x < -1 || x > 16 || y < m_minY || y >= m_minY + HALO_HEIGHT || z < -1 || z > 16) return 0;
        return m_blocks[localIndex(x, y, z)];
    }

    uint8_t getBlockMetadata(int x, int y, int z) const override {
        if (x < -1 || x > 16 || y < m_minY || y >= m_minY + HALO_HEIGHT || z < -1 || z > 16) return 0;
        return m_metadata[localIndex(x, y, z)];
    }

    const Material& getBlockMaterial(int x, int y, int z) const override;
    float getWaterLevel(int x, int z) const { return m_waterLevels[x * 16 + z]; }

    std::pair<int, int> getLightPair(int x, int y, int z) const override {
        if (y >= Chunk::HEIGHT) return {15, 0};
        if (y < 0) return {0, 0};
        x -= m_baseX;
        z -= m_baseZ;
        if (x < -1 || x > 16 || y < m_minY || y >= m_minY + HALO_HEIGHT || z < -1 || z > 16) {
            return {0, 0};
        }
        const int i = localIndex(x, y, z);
        return {m_skylight[i], m_blocklight[i]};
    }

private:
    static int index(int x, int y, int z) {
        return (y * HALO_SIZE + (z + 1)) * HALO_SIZE + (x + 1);
    }

    int localIndex(int x, int y, int z) const { return index(x, y - m_minY, z); }

    int m_baseX;
    int m_baseZ;
    int m_minY;
    std::array<uint8_t, CELL_COUNT> m_blocks {};
    std::array<uint8_t, CELL_COUNT> m_metadata {};
    std::array<uint8_t, CELL_COUNT> m_skylight {};
    std::array<uint8_t, CELL_COUNT> m_blocklight {};
    std::array<float, Chunk::WIDTH * Chunk::DEPTH> m_waterLevels {};
};

class ChunkMesher {
public:
    static ChunkMeshData buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex);

private:
    static void greedyMeshTopBottom(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool up, const float* waterLevels = nullptr);
    static void greedyMeshNorthSouth(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool south, const float* waterLevels = nullptr);
    static void greedyMeshWestEast(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, bool east, const float* waterLevels = nullptr);
    static void fluidMeshPass(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz, const float* waterLevels = nullptr);
    static void crossMeshPass(ChunkMeshData& md, const MeshingSnapshot& n, int si, int cx, int cz);
};
