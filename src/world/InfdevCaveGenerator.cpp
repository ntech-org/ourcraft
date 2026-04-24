#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void InfdevWorldGenerator::generateCaves(Chunk& chunk) {
    int cx = chunk.getX(), cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());
    JavaRandom rand(m_seed);
    int64_t s1 = rand.nextLong() / 2 * 2 + 1, s2 = rand.nextLong() / 2 * 2 + 1;

    for (int x = cx - 8; x <= cx + 8; ++x) {
        for (int z = cz - 8; z <= cz + 8; ++z) {
            JavaRandom cr((int64_t)x * s1 + (int64_t)z * s2 ^ m_seed);
            int caveCount = cr.nextInt(cr.nextInt(cr.nextInt(40) + 1) + 1);
            if (cr.nextInt(15) != 0) caveCount = 0;

            for (int i = 0; i < caveCount; ++i) {
                double wx = (double)(x * 16 + cr.nextInt(16)), wy = (double)(cr.nextInt(cr.nextInt(120) + 8)), wz = (double)(z * 16 + cr.nextInt(16));
                int nodeCount = 1;
                if (cr.nextInt(4) == 0) {
                    generateCaveNode(cx, cz, blocks, wx, wy, wz, 1.0f + (float)(cr.nextFloat() * 6.0f), 0.0f, 0.0f, -1, -1, 0.5, cr);
                    nodeCount += cr.nextInt(4);
                }
                for (int j = 0; j < nodeCount; ++j) {
                    float yaw = (float)(cr.nextFloat() * M_PI * 2.0), pitch = (float)(cr.nextFloat() - 0.5f) * 2.0f / 8.0f, scale = (float)(cr.nextFloat() * 2.0f + cr.nextFloat());
                    generateCaveNode(cx, cz, blocks, wx, wy, wz, scale, yaw, pitch, 0, 0, 1.0, cr);
                }
            }
        }
    }
}

void InfdevWorldGenerator::generateCaveNode(int cx, int cz, uint8_t* blocks, double x, double y, double z, float scale, float yaw, float pitch, int start, int end, double var15, JavaRandom& rand) {
    double centerX = (double)(cx * 16 + 8), centerZ = (double)(cz * 16 + 8);
    float v21 = 0.0f, v22 = 0.0f;
    JavaRandom nr(rand.nextLong());
    if (end <= 0) end = 112 - nr.nextInt(28);
    bool v52 = false; if (start == -1) { start = end / 2; v52 = true; }
    int splitIdx = nr.nextInt(end / 2) + end / 4;
    bool steep = nr.nextInt(6) == 0;

    for (; start < end; ++start) {
        double hs = 1.5 + (double)(std::sin((float)start * (float)M_PI / (float)end) * scale), vs = hs * var15;
        float cp = std::cos(pitch), sp = std::sin(pitch);
        x += (double)(std::cos(yaw) * cp); y += (double)sp; z += (double)(std::sin(yaw) * cp);
        pitch *= (steep ? 0.92f : 0.7f); pitch += v22 * 0.1f; yaw += v21 * 0.1f;
        v22 = v22 * 0.9f + (nr.nextFloat() - nr.nextFloat()) * nr.nextFloat() * 2.0f;
        v21 = v21 * 0.75f + (nr.nextFloat() - nr.nextFloat()) * nr.nextFloat() * 4.0f;
        
        if (!v52 && start == splitIdx && scale > 1.0f) {
            generateCaveNode(cx, cz, blocks, x, y, z, (nr.nextFloat() * 0.5f + 0.5f), yaw - (float)M_PI * 0.5f, pitch / 3.0f, start, end, 1.0, nr);
            generateCaveNode(cx, cz, blocks, x, y, z, (nr.nextFloat() * 0.5f + 0.5f), yaw + (float)M_PI * 0.5f, pitch / 3.0f, start, end, 1.0, nr);
            return;
        }

        if (v52 || nr.nextInt(4) != 0) {
            double dx = x - centerX, dz = z - centerZ, rem = (double)(end - start), md = (double)(scale + 18.0f);
            if (dx * dx + dz * dz - rem * rem > md * md) return;
            if (x >= centerX - 16.0 - hs * 2.0 && z >= centerZ - 16.0 - hs * 2.0 && x <= centerX + 16.0 + hs * 2.0 && z <= centerZ + 16.0 + hs * 2.0) {
                int xMin = std::max((int)std::floor(x - hs) - cx * 16 - 1, 0), xMax = std::min((int)std::floor(x + hs) - cx * 16 + 1, 16);
                int yMin = std::max((int)std::floor(y - vs) - 1, 1), yMax = std::min((int)std::floor(y + vs) + 1, 120);
                int zMin = std::max((int)std::floor(z - hs) - cz * 16 - 1, 0), zMax = std::min((int)std::floor(z + hs) - cz * 16 + 1, 16);

                bool water = false;
                for (int vx = xMin; !water && vx < xMax; ++vx) {
                    for (int vz = zMin; !water && vz < zMax; ++vz) {
                        for (int vy = yMax + 1; !water && vy >= yMin - 1; --vy) {
                            if (vy >= 0 && vy < 128) {
                                uint8_t b = blocks[(vx << 11) | (vz << 7) | vy];
                                if (b == Block::waterMoving->blockID || b == Block::waterStill->blockID) water = true;
                                if (vy != yMin - 1 && vx != xMin && vx != xMax - 1 && vz != zMin && vz != zMax - 1) vy = yMin;
                            }
                        }
                    }
                }
                if (!water) {
                    for (int vx = xMin; vx < xMax; ++vx) {
                        double drx = ((double)(vx + cx * 16) + 0.5 - x) / hs;
                        for (int vz = zMin; vz < zMax; ++vz) {
                            double drz = ((double)(vz + cz * 16) + 0.5 - z) / hs;
                            bool grass = false;
                            for (int vy = yMax - 1; vy >= yMin; --vy) {
                                double dry = ((double)vy + 0.5 - y) / vs;
                                if (dry > -0.7 && drx * drx + dry * dry + drz * drz < 1.0) {
                                    uint8_t b = blocks[(vx << 11) | (vz << 7) | vy];
                                    if (b == Block::grass->blockID) grass = true;
                                    if (b == Block::stone->blockID || b == Block::dirt->blockID || b == Block::grass->blockID) {
                                        blocks[(vx << 11) | (vz << 7) | vy] = (vy < 10) ? (uint8_t)Block::lavaStill->blockID : 0;
                                        if (grass && vy > 0 && blocks[(vx << 11) | (vz << 7) | (vy - 1)] == Block::dirt->blockID) blocks[(vx << 11) | (vz << 7) | (vy - 1)] = (uint8_t)Block::grass->blockID;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
