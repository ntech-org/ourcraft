#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include <cmath>

InfdevWorldGenerator::InfdevWorldGenerator(int seed) : m_seed(seed), m_noise(seed) {}

void InfdevWorldGenerator::generateChunk(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();

    for (int x = 0; x < Chunk::WIDTH; ++x) {
        for (int z = 0; z < Chunk::DEPTH; ++z) {
            float worldX = (float)(cx * Chunk::WIDTH + x);
            float worldZ = (float)(cz * Chunk::DEPTH + z);

            // Simple heightmap generation
            float n = m_noise.noise(worldX * 0.01f, 0.0f, worldZ * 0.01f);
            n += 0.5f * m_noise.noise(worldX * 0.02f, 0.0f, worldZ * 0.02f);
            
            int height = (int)(64.0f + n * 20.0f);
            height = std::clamp(height, 1, Chunk::HEIGHT - 1);

            for (int y = 0; y < Chunk::HEIGHT; ++y) {
                if (y == 0) {
                    chunk.setBlockID(x, y, z, 7); // Bedrock
                } else if (y < height - 4) {
                    chunk.setBlockID(x, y, z, 1); // Stone
                } else if (y < height) {
                    chunk.setBlockID(x, y, z, 3); // Dirt
                } else if (y == height) {
                    chunk.setBlockID(x, y, z, 2); // Grass
                } else {
                    chunk.setBlockID(x, y, z, 0); // Air
                }
            }
        }
    }
}
