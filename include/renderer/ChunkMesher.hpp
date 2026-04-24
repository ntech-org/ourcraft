#pragma once

#include "renderer/ChunkMesh.hpp"
#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"

class World;

class ChunkMesher {
public:
    static ChunkMeshData buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex);

private:
    static void greedyMeshTopBottom(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool up);
    static void greedyMeshNorthSouth(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool south);
    static void greedyMeshWestEast(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz, bool east);
    static void fluidMeshPass(ChunkMeshData& md, const IBlockAccess& n, int si, int cx, int cz);
};
