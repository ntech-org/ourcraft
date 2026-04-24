#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"

InfdevWorldGenerator::InfdevWorldGenerator(int64_t seed) : m_seed(seed) {
    JavaRandom rand(seed);
    m_noiseGen1 = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    m_noiseGen2 = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    m_noiseGen3 = std::make_unique<NoiseGeneratorOctaves>(rand, 8);
    m_noiseGen4 = std::make_unique<NoiseGeneratorOctaves>(rand, 4);
    m_noiseGen5 = std::make_unique<NoiseGeneratorOctaves>(rand, 4);
    m_noiseGen6 = std::make_unique<NoiseGeneratorOctaves>(rand, 10);
    m_noiseGen7 = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    m_mobSpawnerNoise = std::make_unique<NoiseGeneratorOctaves>(rand, 8);
}

void InfdevWorldGenerator::generateChunk(Chunk& chunk) {
    generateTerrain(chunk);
    replaceSurface(chunk);
    generateCaves(chunk);
    for (int i = 0; i < Chunk::SECTION_COUNT; ++i) chunk.touchSection(i);
}
