#include "renderer/ChunkRenderer.hpp"
#include "renderer/BlockRenderer.hpp"
#include "world/Block.hpp"

bool ChunkRenderer::isOpaque(const Chunk& chunk, int x, int y, int z) {
    uint8_t id = chunk.getBlockID(x, y, z);
    if (id == 0) return false;
    return Block::opaqueCubeLookup[id];
}

void ChunkRenderer::generateMesh(const Chunk& chunk) {
    Tessellator* t = Tessellator::instance;
    int chunkX = chunk.getX() * Chunk::WIDTH;
    int chunkZ = chunk.getZ() * Chunk::DEPTH;

    for (int x = 0; x < Chunk::WIDTH; ++x) {
        for (int z = 0; z < Chunk::DEPTH; ++z) {
            for (int y = 0; y < Chunk::HEIGHT; ++y) {
                uint8_t id = chunk.getBlockID(x, y, z);
                if (id == 0) continue;

                Block* block = Block::blocksList[id];
                if (!block) continue;

                double worldX = chunkX + x;
                double worldY = y;
                double worldZ = chunkZ + z;

                // Check each face
                // Bottom
                if (y == 0 || !isOpaque(chunk, x, y - 1, z)) {
                    BlockRenderer::renderFace(block->getTexture(0), worldX, worldY, worldZ, 0);
                }
                // Top
                if (y == Chunk::HEIGHT - 1 || !isOpaque(chunk, x, y + 1, z)) {
                    BlockRenderer::renderFace(block->getTexture(1), worldX, worldY, worldZ, 1);
                }
                // East (-Z)
                if (z == 0 || !isOpaque(chunk, x, y, z - 1)) {
                    BlockRenderer::renderFace(block->getTexture(2), worldX, worldY, worldZ, 2);
                }
                // West (+Z)
                if (z == Chunk::DEPTH - 1 || !isOpaque(chunk, x, y, z + 1)) {
                    BlockRenderer::renderFace(block->getTexture(3), worldX, worldY, worldZ, 3);
                }
                // North (-X)
                if (x == 0 || !isOpaque(chunk, x - 1, y, z)) {
                    BlockRenderer::renderFace(block->getTexture(4), worldX, worldY, worldZ, 4);
                }
                // South (+X)
                if (x == Chunk::WIDTH - 1 || !isOpaque(chunk, x + 1, y, z)) {
                    BlockRenderer::renderFace(block->getTexture(5), worldX, worldY, worldZ, 5);
                }
            }
        }
    }
}
