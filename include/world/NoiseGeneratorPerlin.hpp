#pragma once

#include "world/JavaRandom.hpp"
#include <vector>

class NoiseGeneratorPerlin {
public:
    NoiseGeneratorPerlin(JavaRandom& rand);
    
    double generateNoise(double x, double y, double z) const;
    void populateNoiseArray(double* noiseArray, int x, int y, int z, int xSize, int ySize, int zSize, 
                            double xScale, double yScale, double zScale, double amplitude) const;

private:
    static double lerp(double t, double a, double b);
    static double grad(int hash, double x, double y, double z);
    
    int m_permutations[512];
    double m_xCoord, m_yCoord, m_zCoord;
};
