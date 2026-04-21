#pragma once

#include "world/Chunk.hpp"
#include "renderer/Tessellator.hpp"

class ChunkRenderer {
public:
    static void generateMesh(const Chunk& chunk);

private:
    static bool isOpaque(const Chunk& chunk, int x, int y, int z);
};
