#pragma once

#include "renderer/ChunkMesh.hpp"
#include "world/Chunk.hpp"

class World;

class ChunkMesher {
public:
    static ChunkMeshData buildSectionMesh(const World& world, const Chunk& chunk, int sectionIndex);
};
