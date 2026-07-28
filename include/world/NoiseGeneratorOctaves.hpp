#pragma once

#include "world/NoiseGeneratorPerlin.hpp"
#include <memory>
#include <vector>

class NoiseGeneratorOctaves {
public:
    NoiseGeneratorOctaves(JavaRandom& rand, int octaves);
    
    double generateNoise(double x, double y) const;
    double generateNoise(double x, double y, double z) const;
    
    void populateNoiseArray(double* noiseArray, int x, int y, int z, int xSize, int ySize, int zSize, 
                            double xScale, double yScale, double zScale) const;

    void setFarLandsOffset(int32_t offset) {
        for (auto& gen : m_generators) gen->setFarLandsOffset(offset);
    }

private:
    int m_octaves;
    std::vector<std::unique_ptr<NoiseGeneratorPerlin>> m_generators;
};
