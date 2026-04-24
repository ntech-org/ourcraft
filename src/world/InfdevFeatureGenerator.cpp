#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void InfdevWorldGenerator::replaceSurface(Chunk& chunk) {
    int cx = chunk.getX(), cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());
    JavaRandom rand(m_seed);
    int64_t v5 = rand.nextLong() / 2 * 2 + 1, v7 = rand.nextLong() / 2 * 2 + 1;
    rand.setSeed((int64_t)cx * v5 + (int64_t)cz * v7 ^ m_seed);

    std::vector<double> sn(256), gn(256), sun(256);
    double v13 = 0.03125;
    m_noiseGen4->populateNoiseArray(sn.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, v13, v13, 1.0);
    m_noiseGen4->populateNoiseArray(gn.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, v13 * 2.0, v13 * 2.0, 1.0);
    m_noiseGen5->populateNoiseArray(sun.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, v13 * 8.0, v13 * 8.0, 1.0);

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            bool sand = sn[x * 16 + z] + rand.nextDouble() * 0.2 > 0.0, gravel = gn[x * 16 + z] + rand.nextDouble() * 0.2 > 3.0;
            int thres = (int)(sun[x * 16 + z] / 3.0 + 3.0 + rand.nextDouble() * 0.25), cnt = -1;
            uint8_t top = (uint8_t)Block::grass->blockID, fill = (uint8_t)Block::dirt->blockID;

            for (int y = 127; y >= 0; --y) {
                int idx = (x << 11) | (z << 7) | y;
                if (y <= rand.nextInt(6)) blocks[idx] = (uint8_t)Block::bedrock->blockID;
                else {
                    uint8_t b = blocks[idx];
                    if (b == 0) cnt = -1;
                    else if (b == Block::stone->blockID) {
                        if (cnt == -1) {
                            if (thres <= 0) { top = 0; fill = (uint8_t)Block::stone->blockID; }
                            else if (y >= 60 && y <= 65) {
                                top = (uint8_t)Block::grass->blockID; fill = (uint8_t)Block::dirt->blockID;
                                if (gravel) { top = 0; fill = (uint8_t)Block::gravel->blockID; }
                                if (sand) { top = (uint8_t)Block::sand->blockID; fill = (uint8_t)Block::sand->blockID; }
                            }
                            if (y < 64 && top == 0) top = (uint8_t)Block::waterStill->blockID;
                            cnt = thres; blocks[idx] = (y >= 63) ? top : fill;
                        } else if (cnt > 0) { --cnt; blocks[idx] = fill; }
                    }
                }
            }
        }
    }
}

void InfdevWorldGenerator::decorateChunk(Chunk& chunk, Chunk* cE, Chunk* cS, Chunk* cSE) {
    int cx = chunk.getX(), cz = chunk.getZ();
    JavaRandom rand(m_seed);
    int64_t v6 = rand.nextLong() / 2 * 2 + 1, v8 = rand.nextLong() / 2 * 2 + 1;
    JavaRandom dr((int64_t)cx * v6 + (int64_t)cz * v8 ^ m_seed);

    auto get = [&](int x, int y, int z) -> uint8_t {
        if (y < 0 || y >= 128) return 0;
        int lx = x - cx * 16, lz = z - cz * 16;
        Chunk* t = (lx >= 0 && lx < 16 && lz >= 0 && lz < 16) ? &chunk : ((lx >= 16 && lz < 16) ? cE : ((lx < 16 && lz >= 16) ? cS : cSE));
        return t ? t->getBlockID(lx & 15, y, lz & 15) : 0;
    };
    auto set = [&](int x, int y, int z, uint8_t id) {
        if (y < 0 || y >= 128) return;
        int lx = x - cx * 16, lz = z - cz * 16;
        Chunk* t = (lx >= 0 && lx < 16 && lz >= 0 && lz < 16) ? &chunk : ((lx >= 16 && lz < 16) ? cE : ((lx < 16 && lz >= 16) ? cS : cSE));
        if (t) t->setBlockIDSafe(lx & 15, y, lz & 15, id);
    };

    auto generateOre = [&](uint8_t id, int count, int size, int minY, int maxY) {
        for (int i = 0; i < count; ++i) {
            double wx = cx * 16 + dr.nextInt(16), wy = minY + dr.nextInt(maxY - minY), wz = cz * 16 + dr.nextInt(16);
            float angle = (float)(dr.nextFloat() * M_PI);
            double x1 = wx + std::sin(angle) * size / 8.0, x2 = wx - std::sin(angle) * size / 8.0;
            double z1 = wz + std::cos(angle) * size / 8.0, z2 = wz - std::cos(angle) * size / 8.0;
            double y1 = wy + dr.nextInt(3) - 2, y2 = wy + dr.nextInt(3) - 2;
            for (int j = 0; j <= size; ++j) {
                double cx = x1 + (x2 - x1) * j / size, cy = y1 + (y2 - y1) * j / size, cz = z1 + (z2 - z1) * j / size;
                double cs = (std::sin(j * M_PI / size) + 1.0) * size / 16.0 + 1.0;
                for (int vx = std::floor(cx - cs); vx <= std::floor(cx + cs); ++vx) {
                    double dx = (vx + 0.5 - cx) / cs; if (dx * dx >= 1.0) continue;
                    for (int vy = std::floor(cy - cs); vy <= std::floor(cy + cs); ++vy) {
                        double dy = (vy + 0.5 - cy) / cs; if (dx * dx + dy * dy >= 1.0) continue;
                        for (int vz = std::floor(cz - cs); vz <= std::floor(cz + cs); ++vz) {
                            double dz = (vz + 0.5 - cz) / cs;
                            if (dx * dx + dy * dy + dz * dz < 1.0 && get(vx, vy, vz) == Block::stone->blockID) set(vx, vy, vz, id);
                        }
                    }
                }
            }
        }
    };

    generateOre(Block::dirt->blockID, 20, 32, 0, 128); generateOre(Block::gravel->blockID, 10, 32, 0, 128);
    generateOre(Block::oreCoal->blockID, 20, 16, 0, 128); generateOre(Block::oreIron->blockID, 20, 8, 0, 64);
    generateOre(Block::oreGold->blockID, 2, 8, 0, 32); generateOre(Block::oreDiamond->blockID, 1, 8, 0, 16);

    auto getHeight = [&](int x, int z) -> int {
        for (int y = 127; y >= 0; --y) {
            uint8_t b = get(x, y, z);
            if (b != 0 && b != Block::waterStill->blockID && b != Block::waterMoving->blockID) return y + 1;
        }
        return 0;
    };

    int treeCount = (int)((m_mobSpawnerNoise->generateNoise(cx * 16 * 0.1, cz * 16 * 0.1) * 3.0 + dr.nextDouble() * 2.0 + 2.0));
    if (dr.nextInt(10) == 0) treeCount = 1; else if (dr.nextInt(10) > 8) treeCount = 0; // Adjust density

    for (int i = 0; i < treeCount; ++i) {
        int x = cx * 16 + dr.nextInt(16) + 8;
        int z = cz * 16 + dr.nextInt(16) + 8;
        int y = getHeight(x, z);
        
        if (y <= 0) continue;

        int h = dr.nextInt(3) + 4;
        bool grow = (y >= 1 && y + h + 1 <= 128);
        if (grow) {
            for (int vy = y; vy <= y + 1 + h && grow; ++vy) {
                int r = (vy == y) ? 0 : (vy >= y + 1 + h - 2 ? 2 : 1);
                for (int vx = x - r; vx <= x + r && grow; ++vx) for (int vz = z - r; vz <= z + r && grow; ++vz) {
                    uint8_t b = get(vx, vy, vz); if (b != 0 && b != Block::leaves->blockID) grow = false;
                }
            }
        }
        if (grow) {
            uint8_t ground = get(x, y - 1, z);
            if (ground == Block::grass->blockID || ground == Block::dirt->blockID) {
                set(x, y - 1, z, Block::dirt->blockID);
                for (int vy = y - 3 + h; vy <= y + h; ++vy) {
                    int relY = vy - (y + h), r = 1 - relY / 2;
                    for (int vx = x - r; vx <= x + r; ++vx) for (int vz = z - r; vz <= z + r; ++vz) {
                        if (std::abs(vx - x) != r || std::abs(vz - z) != r || (dr.nextInt(2) != 0 && relY != 0)) {
                            if (get(vx, vy, vz) == 0) set(vx, vy, vz, Block::leaves->blockID);
                        }
                    }
                }
                for (int vy = 0; vy < h; ++vy) { uint8_t b = get(x, y + vy, z); if (b == 0 || b == Block::leaves->blockID) set(x, y + vy, z, Block::wood->blockID); }
            }
        }
    }

    for (int i = 0; i < 2; ++i) {
        int x = cx * 16 + dr.nextInt(16) + 8, z = cz * 16 + dr.nextInt(16) + 8, y = dr.nextInt(128);
        if (get(x, y, z) == 0 && get(x, y - 1, z) == Block::grass->blockID) set(x, y, z, Block::flowerYellow->blockID);
    }
    if (dr.nextInt(2) == 0) {
        int x = cx * 16 + dr.nextInt(16) + 8, z = cz * 16 + dr.nextInt(16) + 8, y = dr.nextInt(128);
        if (get(x, y, z) == 0 && get(x, y - 1, z) == Block::grass->blockID) set(x, y, z, Block::flowerRed->blockID);
    }
    if (dr.nextInt(4) == 0) {
        int x = cx * 16 + dr.nextInt(16) + 8, z = cz * 16 + dr.nextInt(16) + 8, y = dr.nextInt(128);
        if (get(x, y, z) == 0 && get(x, y - 1, z) == Block::grass->blockID) set(x, y, z, Block::mushroomBrown->blockID);
    }
    if (dr.nextInt(8) == 0) {
        int x = cx * 16 + dr.nextInt(16) + 8, z = cz * 16 + dr.nextInt(16) + 8, y = dr.nextInt(128);
        if (get(x, y, z) == 0 && get(x, y - 1, z) == Block::grass->blockID) set(x, y, z, Block::mushroomRed->blockID);
    }
}
