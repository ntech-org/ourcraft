#pragma once

#include "world/WorldGenerator.hpp"
#include "world/PerlinNoise.hpp"
#include <memory>

class InfdevWorldGenerator : public WorldGenerator {
public:
    InfdevWorldGenerator(int seed);
    void generateChunk(Chunk& chunk) override;

private:
    int m_seed;
    PerlinNoise m_noise;
};
