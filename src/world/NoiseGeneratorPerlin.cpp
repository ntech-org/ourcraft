#include "world/NoiseGeneratorPerlin.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <limits>

// Java narrowing (int) cast for doubles: saturate to INT_MIN/INT_MAX (not wrap).
static int32_t javaDoubleToInt(double d) {
    if (std::isnan(d)) return 0;
    if (d >= static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return std::numeric_limits<int32_t>::max();
    }
    if (d <= static_cast<double>(std::numeric_limits<int32_t>::min())) {
        return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(d); // truncate toward zero
}

// Java Perlin lattice selection: (int)d, then if (d < X) X-- with 32-bit wrap.
// Fraction is d - X (unbounded when Far Lands overflow saturates X).
static void selectLattice(double d, bool farLands, int32_t& outLattice, double& outFrac) {
    if (farLands) {
        int32_t X = javaDoubleToInt(d);
        if (d < static_cast<double>(X)) {
            // Java int overflow wraps; emulate with unsigned wrap.
            X = static_cast<int32_t>(static_cast<uint32_t>(X) - 1u);
        }
        outLattice = X;
        outFrac = d - static_cast<double>(X);
    } else {
        double fl = std::floor(d);
        outLattice = static_cast<int32_t>(static_cast<int64_t>(fl));
        outFrac = d - fl;
    }
}

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

double NoiseGeneratorPerlin::generateNoise(double x, double y, double z) const {
    double x_p = x + m_xCoord;
    double y_p = y + m_yCoord;
    double z_p = z + m_zCoord;

    const bool farLands = InfdevWorldGenerator::isFarLandsEnabledStatic();
    int32_t X, Y, Z;
    double x_f, y_f, z_f;
    selectLattice(x_p, farLands, X, x_f);
    selectLattice(y_p, farLands, Y, y_f);
    selectLattice(z_p, farLands, Z, z_f);

    int var16 = X & 255;
    int var17 = Y & 255;
    int var18 = Z & 255;

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
    bool farLands = InfdevWorldGenerator::isFarLandsEnabledStatic();
    int index = 0;
    double invAmplitude = 1.0 / amplitude;
    int lastY = -1;
    double lerp_x000 = 0, lerp_x010 = 0, lerp_x001 = 0, lerp_x011 = 0;

    for (int i = 0; i < xSize; ++i) {
        double worldX = (double)(x + i) * xScale + m_xCoord;
        int32_t X;
        double x_f;
        selectLattice(worldX, farLands, X, x_f);
        int var38 = X & 255;
        double u = x_f * x_f * x_f * (x_f * (x_f * 6.0 - 15.0) + 10.0);

        for (int k = 0; k < zSize; ++k) {
            double worldZ = (double)(z + k) * zScale + m_zCoord;
            int32_t Z;
            double z_f;
            selectLattice(worldZ, farLands, Z, z_f);
            int var45 = Z & 255;
            double w = z_f * z_f * z_f * (z_f * (z_f * 6.0 - 15.0) + 10.0);

            for (int j = 0; j < ySize; ++j) {
                double worldY = (double)(y + j) * yScale + m_yCoord;
                int32_t Y;
                double y_f;
                // Y never overflows in normal terrain generation; still use same path when enabled.
                selectLattice(worldY, farLands, Y, y_f);
                int var52 = Y & 255;
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
