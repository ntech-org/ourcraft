#include "world/NoiseGeneratorPerlin.hpp"
#include <cmath>
#include <algorithm>

NoiseGeneratorPerlin::NoiseGeneratorPerlin(JavaRandom& rand) {
    m_xCoord = rand.nextDouble() * 256.0;
    m_yCoord = rand.nextDouble() * 256.0;
    m_zCoord = rand.nextDouble() * 256.0;

    for (int i = 0; i < 256; ++i) {
        m_permutations[i] = i;
    }

    for (int i = 0; i < 256; ++i) {
        int j = rand.nextInt(256 - i) + i;
        int temp = m_permutations[i];
        m_permutations[i] = m_permutations[j];
        m_permutations[j] = temp;
        m_permutations[i + 256] = m_permutations[i];
    }
}

double NoiseGeneratorPerlin::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double NoiseGeneratorPerlin::grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Emulate Java's (int) double cast: truncate toward zero, keep low 32 bits.
int32_t NoiseGeneratorPerlin::javaIntCast(double d) {
    int64_t v = static_cast<int64_t>(d);
    uint32_t low = static_cast<uint32_t>(v);
    // Reinterpret bits as signed int32 (same as Java's wrap)
    union { uint32_t u; int32_t i; } c;
    c.u = low;
    return c.i;
}

double NoiseGeneratorPerlin::generateNoise(double x, double y, double z) const {
    double x_p = x + m_xCoord + m_farLandsOffset;
    double y_p = y + m_yCoord;
    double z_p = z + m_zCoord;
    
    int X = m_farLandsOffset ? javaIntCast(x_p) : (int)std::floor(x_p);
    int Y = (int)std::floor(y_p);
    int Z = (int)std::floor(z_p);

    int var16 = X & 255;
    int var17 = Y & 255;
    int var18 = Z & 255;
    
    double x_f = x_p - std::floor(x_p);
    double y_f = y_p - std::floor(y_p);
    double z_f = z_p - std::floor(z_p);

    double u = x_f * x_f * x_f * (x_f * (x_f * 6.0 - 15.0) + 10.0);
    double v = y_f * y_f * y_f * (y_f * (y_f * 6.0 - 15.0) + 10.0);
    double w = z_f * z_f * z_f * (z_f * (z_f * 6.0 - 15.0) + 10.0);

    int A = m_permutations[var16] + var17;
    int AA = m_permutations[A] + var18;
    int AB = m_permutations[A + 1] + var18;
    int B = m_permutations[var16 + 1] + var17;
    int BA = m_permutations[B] + var18;
    int BB = m_permutations[B + 1] + var18;

    return lerp(w, lerp(v, lerp(u, grad(m_permutations[AA], x_f, y_f, z_f), 
                                   grad(m_permutations[BA], x_f - 1.0, y_f, z_f)), 
                           lerp(u, grad(m_permutations[AB], x_f, y_f - 1.0, z_f), 
                                   grad(m_permutations[BB], x_f - 1.0, y_f - 1.0, z_f))), 
                   lerp(v, lerp(u, grad(m_permutations[AA + 1], x_f, y_f, z_f - 1.0), 
                                   grad(m_permutations[BA + 1], x_f - 1.0, y_f, z_f - 1.0)), 
                           lerp(u, grad(m_permutations[AB + 1], x_f, y_f - 1.0, z_f - 1.0), 
                                   grad(m_permutations[BB + 1], x_f - 1.0, y_f - 1.0, z_f - 1.0))));
}

void NoiseGeneratorPerlin::populateNoiseArray(double* noiseArray, int x, int y, int z, int xSize, int ySize, int zSize, 
                                             double xScale, double yScale, double zScale, double amplitude) const 
{
    int index = 0;
    double invAmplitude = 1.0 / amplitude;
    int lastY = -1;
    double lerp_x000 = 0, lerp_x010 = 0, lerp_x001 = 0, lerp_x011 = 0;

    for (int i = 0; i < xSize; ++i) {
        double worldX = (double)(x + i) * xScale + m_xCoord + m_farLandsOffset;
        int X = m_farLandsOffset ? javaIntCast(worldX) : (int)std::floor(worldX);
        int var38 = X & 255;
        double x_f = worldX - std::floor(worldX);
        double u = x_f * x_f * x_f * (x_f * (x_f * 6.0 - 15.0) + 10.0);

        for (int k = 0; k < zSize; ++k) {
            double worldZ = (double)(z + k) * zScale + m_zCoord + m_farLandsOffset;
            int Z = m_farLandsOffset ? javaIntCast(worldZ) : (int)std::floor(worldZ);
            int var45 = Z & 255;
            double z_f = worldZ - std::floor(worldZ);
            double w = z_f * z_f * z_f * (z_f * (z_f * 6.0 - 15.0) + 10.0);

            for (int j = 0; j < ySize; ++j) {
                double worldY = (double)(y + j) * yScale + m_yCoord;
                int Y = (int)std::floor(worldY);
                int var52 = Y & 255;
                double y_f = worldY - std::floor(worldY);
                double v = y_f * y_f * y_f * (y_f * (y_f * 6.0 - 15.0) + 10.0);

                if (j == 0 || var52 != lastY) {
                    lastY = var52;
                    int A = m_permutations[var38] + var52;
                    int AA = m_permutations[A] + var45;
                    int AB = m_permutations[A + 1] + var45;
                    int B = m_permutations[var38 + 1] + var52;
                    int BA = m_permutations[B] + var45;
                    int BB = m_permutations[B + 1] + var45;

                    lerp_x000 = lerp(u, grad(m_permutations[AA], x_f, y_f, z_f), grad(m_permutations[BA], x_f - 1.0, y_f, z_f));
                    lerp_x010 = lerp(u, grad(m_permutations[AB], x_f, y_f - 1.0, z_f), grad(m_permutations[BB], x_f - 1.0, y_f - 1.0, z_f));
                    lerp_x001 = lerp(u, grad(m_permutations[AA + 1], x_f, y_f, z_f - 1.0), grad(m_permutations[BA + 1], x_f - 1.0, y_f, z_f - 1.0));
                    lerp_x011 = lerp(u, grad(m_permutations[AB + 1], x_f, y_f - 1.0, z_f - 1.0), grad(m_permutations[BB + 1], x_f - 1.0, y_f - 1.0, z_f - 1.0));
                }

                double lerp_y0 = lerp(v, lerp_x000, lerp_x010);
                double lerp_y1 = lerp(v, lerp_x001, lerp_x011);
                double val = lerp(w, lerp_y0, lerp_y1);
                
                noiseArray[index++] += val * invAmplitude;
            }
        }
    }
}
