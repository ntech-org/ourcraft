#include "world/NoiseGeneratorOctaves.hpp"

NoiseGeneratorOctaves::NoiseGeneratorOctaves(JavaRandom& rand, int octaves) : m_octaves(octaves) {
    for (int i = 0; i < octaves; ++i) {
        m_generators.push_back(std::make_unique<NoiseGeneratorPerlin>(rand));
    }
}

double NoiseGeneratorOctaves::generateNoise(double x, double y) const {
    double total = 0;
    double amplitude = 1.0;
    for (int i = 0; i < m_octaves; ++i) {
        total += m_generators[i]->generateNoise(x * amplitude, y * amplitude, 0.0) / amplitude;
        amplitude /= 2.0;
    }
    return total;
}

double NoiseGeneratorOctaves::generateNoise(double x, double y, double z) const {
    double total = 0;
    double amplitude = 1.0;
    for (int i = 0; i < m_octaves; ++i) {
        total += m_generators[i]->generateNoise(x * amplitude, y * amplitude, z * amplitude) / amplitude;
        amplitude /= 2.0;
    }
    return total;
}

void NoiseGeneratorOctaves::populateNoiseArray(double* noiseArray, int x, int y, int z, int xSize, int ySize, int zSize, 
                                              double xScale, double yScale, double zScale) const 
{
    // Clear array (matching Java reset)
    for (int i = 0; i < xSize * ySize * zSize; ++i) {
        noiseArray[i] = 0.0;
    }

    double amplitude = 1.0;
    for (int i = 0; i < m_octaves; ++i) {
        m_generators[i]->populateNoiseArray(noiseArray, x, y, z, xSize, ySize, zSize, xScale * amplitude, yScale * amplitude, zScale * amplitude, amplitude);
        amplitude /= 2.0;
    }
}
