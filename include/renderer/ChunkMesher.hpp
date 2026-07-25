#pragma once

#include "renderer/ChunkMesh.hpp"
#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"
#include "world/Material.hpp"
#include "world/World.hpp"

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

class ChunkMesher {
public:
    static ChunkMeshData buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex);

private:
    static void greedyMeshTopBottom(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool up, const float* waterLevels = nullptr);
    static void greedyMeshNorthSouth(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool south, const float* waterLevels = nullptr);
    static void greedyMeshWestEast(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool east, const float* waterLevels = nullptr);
    static void fluidMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, const float* waterLevels = nullptr);
    static void crossMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz);
};
