#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <algorithm>

void InfdevWorldGenerator::generateTerrain(Chunk& chunk) {
    int cx = chunk.getX(), cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());
    std::vector<double> noiseArray(5 * 17 * 5), n1, n2, n3, n6, n7;
    initializeNoiseField(noiseArray.data(), cx * 4, 0, cz * 4, 5, 17, 5, n1, n2, n3, n6, n7);

    for (int x = 0; x < 4; ++x) {
        for (int z = 0; z < 4; ++z) {
            for (int y = 0; y < 16; ++y) {
                double v14 = noiseArray[(x * 5 + z) * 17 + y], v16 = noiseArray[(x * 5 + z + 1) * 17 + y], v18 = noiseArray[((x + 1) * 5 + z) * 17 + y], v20 = noiseArray[((x + 1) * 5 + z + 1) * 17 + y];
                double v22 = (noiseArray[(x * 5 + z) * 17 + y + 1] - v14) * 0.125, v24 = (noiseArray[(x * 5 + z + 1) * 17 + y + 1] - v16) * 0.125;
                double v26 = (noiseArray[((x + 1) * 5 + z) * 17 + y + 1] - v18) * 0.125, v28 = (noiseArray[((x + 1) * 5 + z + 1) * 17 + y + 1] - v20) * 0.125;

                for (int dy = 0; dy < 8; ++dy) {
                    double v33 = v14, v35 = v16, v37 = (v18 - v14) * 0.25, v39 = (v20 - v16) * 0.25;
                    for (int dx = 0; dx < 4; ++dx) {
                        int idx = ((x * 4 + dx) << 11) | (z * 4 << 7) | (y * 8 + dy);
                        double v45 = v33, v47 = (v35 - v33) * 0.25;
                        for (int dz = 0; dz < 4; ++dz) {
                            blocks[idx] = (v45 > 0.0) ? (uint8_t)Block::stone->blockID : ((y * 8 + dy < 64) ? (uint8_t)Block::waterStill->blockID : 0);
                            idx += 128; v45 += v47;
                        }
                        v33 += v37; v35 += v39;
                    }
                    v14 += v22; v16 += v24; v18 += v26; v20 += v28;
                }
            }
        }
    }
}

double* InfdevWorldGenerator::initializeNoiseField(double* v1, int v2, int v3, int v4, int v5, int v6, int v7, std::vector<double>& n1, std::vector<double>& n2, std::vector<double>& n3, std::vector<double>& n6, std::vector<double>& n7) {
    if (m_farLands) { v2 += FARLANDS_OFFSET / 4; v4 += FARLANDS_OFFSET / 4; }
    n6.resize(v5 * v7); m_noiseGen6->populateNoiseArray(n6.data(), v2, 0, v4, v5, 1, v7, 1.0, 0.0, 1.0);
    n7.resize(v5 * v7); m_noiseGen7->populateNoiseArray(n7.data(), v2, 0, v4, v5, 1, v7, 100.0, 0.0, 100.0);
    n3.resize(v5 * v6 * v7); m_noiseGen3->populateNoiseArray(n3.data(), v2, v3, v4, v5, v6, v7, 8.555, 4.277, 8.555);
    n1.resize(v5 * v6 * v7); m_noiseGen1->populateNoiseArray(n1.data(), v2, v3, v4, v5, v6, v7, 684.412, 684.412, 684.412);
    n2.resize(v5 * v6 * v7); m_noiseGen2->populateNoiseArray(n2.data(), v2, v3, v4, v5, v6, v7, 684.412, 684.412, 684.412);
    
    int i12 = 0, i13 = 0;
    for (int x = 0; x < v5; ++x) {
        for (int z = 0; z < v7; ++z) {
            double v16 = std::min((n6[i13] + 256.0) / 512.0, 1.0), v20 = std::abs(n7[i13] / 8000.0) * 3.0 - 3.0;
            if (v20 < 0.0) { v20 = std::max(v20 / 2.8, -1.0) / 2.0; v16 = 0.0; } else { v20 = std::min(v20 / 6.0, 1.0); }
            double v22 = (double)v6 / 2.0 + (v20 * (double)v6 / 16.0) * 4.0; i13++;
            for (int y = 0; y < v6; ++y) {
                double v27 = ((double)y - v22) * 12.0 / (v16 + 0.5); if (v27 < 0.0) v27 *= 4.0;
                double v33 = (n3[i12] / 10.0 + 1.0) / 2.0, v25 = (v33 < 0.0) ? (n1[i12] / 512.0) : ((v33 > 1.0) ? (n2[i12] / 512.0) : (n1[i12] / 512.0 + (n2[i12] - n1[i12]) / 512.0 * v33));
                v25 -= v27;
                if (y > v6 - 4) { double v35 = (double)((float)(y - (v6 - 4)) / 3.0f); v25 = v25 * (1.0 - v35) - 10.0 * v35; }
                v1[i12++] = v25;
            }
        }
    }
    return v1;
}
