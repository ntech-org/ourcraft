#pragma once

#include "renderer/ChunkMesh.hpp"
#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"
#include "world/Material.hpp"

class World;

enum class FaceDirection : uint8_t { Down = 0, Up = 1, North = 2, South = 3, West = 4, East = 5 };

constexpr std::uint32_t kWhiteColor = 0xFFFFFFFFu;

inline void appendIndices(ChunkMeshData::Pass& p) {
    std::uint32_t b = (std::uint32_t)p.vertices.size() - 4;
    p.indices.insert(p.indices.end(), {b, b + 1, b + 2, b, b + 2, b + 3}); p.quadCount++;
}

inline float getWaterDepth(const IBlockAccess& n, int x, int y, int z) {
    float d = 0;
    for (int i = 0; (y + i + 1) < Chunk::HEIGHT; ++i) {
        if (n.getBlockMaterial(x, y + i + 1, z).isLiquid()) {
            d += 1.0f;
        } else {
            break;
        }
    }
    return d;
}

class ChunkMesher {
public:
    static ChunkMeshData buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex);

private:
    static void greedyMeshTopBottom(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool up);
    static void greedyMeshNorthSouth(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool south);
    static void greedyMeshWestEast(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool east);
    static void fluidMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz);
    static void crossMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz);
};
