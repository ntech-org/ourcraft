#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include <cmath>

InfdevWorldGenerator::InfdevWorldGenerator(int seed) : m_seed(seed), m_noise(seed) {}

void InfdevWorldGenerator::generateChunk(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());

    for (int x = 0; x < Chunk::WIDTH; ++x) {
        for (int z = 0; z < Chunk::DEPTH; ++z) {
            float worldX = (float)(cx * Chunk::WIDTH + x);
            float worldZ = (float)(cz * Chunk::DEPTH + z);

            float n = m_noise.noise(worldX * 0.01f, 0.0f, worldZ * 0.01f);
            n += 0.5f * m_noise.noise(worldX * 0.02f, 0.0f, worldZ * 0.02f);
            
            int height = (int)(64.0f + n * 20.0f);
            height = std::clamp(height, 1, Chunk::HEIGHT - 1);

            // Access column directly
            int baseIndex = (x << 11) | (z << 7);
            
            // Bedrock at 0
            blocks[baseIndex] = 7;
            
            // Stone
            for (int y = 1; y < height - 4; ++y) {
                blocks[baseIndex + y] = 1;
            }
            // Dirt
            for (int y = std::max(1, height - 4); y < height; ++y) {
                blocks[baseIndex + y] = 3;
            }
            // Grass
            blocks[baseIndex + height] = 2;
            
            // Air (rest)
            for (int y = height + 1; y < Chunk::HEIGHT; ++y) {
                blocks[baseIndex + y] = 0;
            }
        }
    }
    
    // Mark sections dirty once
    for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
        chunk.touchSection(i);
    }
}
