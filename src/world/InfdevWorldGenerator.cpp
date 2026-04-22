#include "world/InfdevWorldGenerator.hpp"
#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
        chunk.touchSection(i);
    }
}

void InfdevWorldGenerator::generateTerrain(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());

    int var4 = 4;
    int var6 = var4 + 1;
    int var7 = 17;
    int var8 = var4 + 1;
    
    std::vector<double> noiseArray(var6 * var7 * var8);
    std::vector<double> n1, n2, n3, n6, n7;
    initializeNoiseField(noiseArray.data(), cx * var4, 0, cz * var4, var6, var7, var8, n1, n2, n3, n6, n7);

    for (int var9 = 0; var9 < var4; ++var9) {
        for (int var10 = 0; var10 < var4; ++var10) {
            for (int var11 = 0; var11 < 16; ++var11) {
                double var12 = 0.125;
                double var14 = noiseArray[((var9 + 0) * var8 + var10 + 0) * var7 + var11 + 0];
                double var16 = noiseArray[((var9 + 0) * var8 + var10 + 1) * var7 + var11 + 0];
                double var18 = noiseArray[((var9 + 1) * var8 + var10 + 0) * var7 + var11 + 0];
                double var20 = noiseArray[((var9 + 1) * var8 + var10 + 1) * var7 + var11 + 0];
                double var22 = (noiseArray[((var9 + 0) * var8 + var10 + 0) * var7 + var11 + 1] - var14) * var12;
                double var24 = (noiseArray[((var9 + 0) * var8 + var10 + 1) * var7 + var11 + 1] - var16) * var12;
                double var26 = (noiseArray[((var9 + 1) * var8 + var10 + 0) * var7 + var11 + 1] - var18) * var12;
                double var28 = (noiseArray[((var9 + 1) * var8 + var10 + 1) * var7 + var11 + 1] - var20) * var12;

                for (int var30 = 0; var30 < 8; ++var30) {
                    double var31 = 0.25;
                    double var33 = var14;
                    double var35 = var16;
                    double var37 = (var18 - var14) * var31;
                    double var39 = (var20 - var16) * var31;

                    for (int var41 = 0; var41 < 4; ++var41) {
                        int index = ((var9 * 4 + var41) << 11) | ((var10 * 4 + 0) << 7) | (var11 * 8 + var30);
                        double var43 = 0.25;
                        double var45 = var33;
                        double var47 = (var35 - var33) * var43;

                        for (int var49 = 0; var49 < 4; ++var49) {
                            int blockID = 0;
                            if (var11 * 8 + var30 < 64) {
                                blockID = Block::waterStill->blockID;
                            }

                            if (var45 > 0.0) {
                                blockID = Block::stone->blockID;
                            }

                            blocks[index] = (uint8_t)blockID;
                            index += 128;
                            var45 += var47;
                        }

                        var33 += var37;
                        var35 += var39;
                    }

                    var14 += var22;
                    var16 += var24;
                    var18 += var26;
                    var20 += var28;
                }
            }
        }
    }
}

void InfdevWorldGenerator::replaceSurface(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());
    
    JavaRandom rand(m_seed);
    int64_t var5 = rand.nextLong() / 2 * 2 + 1;
    int64_t var7 = rand.nextLong() / 2 * 2 + 1;
    rand.setSeed((int64_t)cx * var5 + (int64_t)cz * var7 ^ m_seed);

    std::vector<double> sandNoise(256);
    std::vector<double> gravelNoise(256);
    std::vector<double> surfaceNoise(256);
    double var13 = 0.03125;
    m_noiseGen4->populateNoiseArray(sandNoise.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, var13, var13, 1.0);
    m_noiseGen4->populateNoiseArray(gravelNoise.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, var13 * 2.0, var13 * 2.0, 1.0);
    m_noiseGen5->populateNoiseArray(surfaceNoise.data(), cx * 16, cz * 16, 0.0, 16, 16, 1, var13 * 8.0, var13 * 8.0, 1.0);

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            bool sand = sandNoise[x * 16 + z] + rand.nextDouble() * 0.2 > 0.0;
            bool gravel = gravelNoise[x * 16 + z] + rand.nextDouble() * 0.2 > 3.0;
            int threshold = (int)(surfaceNoise[x * 16 + z] / 3.0 + 3.0 + rand.nextDouble() * 0.25);
            
            int counter = -1;
            uint8_t topBlock = (uint8_t)Block::grass->blockID;
            uint8_t fillerBlock = (uint8_t)Block::dirt->blockID;

            int baseIndex = (x << 11) | (z << 7);
            for (int y = 127; y >= 0; --y) {
                int index = baseIndex + y;
                if (y <= 0 + rand.nextInt(6) - 1) {
                    blocks[index] = (uint8_t)Block::bedrock->blockID;
                } else {
                    uint8_t b = blocks[index];
                    if (b == 0) {
                        counter = -1;
                    } else if (b == Block::stone->blockID) {
                        if (counter == -1) {
                            if (threshold <= 0) {
                                topBlock = 0;
                                fillerBlock = (uint8_t)Block::stone->blockID;
                            } else if (y >= 64 - 4 && y <= 64 + 1) {
                                topBlock = (uint8_t)Block::grass->blockID;
                                fillerBlock = (uint8_t)Block::dirt->blockID;
                                if (gravel) {
                                    topBlock = 0;
                                    fillerBlock = (uint8_t)Block::gravel->blockID;
                                }
                                if (sand) {
                                    topBlock = (uint8_t)Block::sand->blockID;
                                    fillerBlock = (uint8_t)Block::sand->blockID;
                                }
                            }

                            if (y < 64 && topBlock == 0) {
                                topBlock = (uint8_t)Block::waterStill->blockID;
                            }

                            counter = threshold;
                            if (y >= 64 - 1) {
                                blocks[index] = topBlock;
                            } else {
                                blocks[index] = fillerBlock;
                            }
                        } else if (counter > 0) {
                            --counter;
                            blocks[index] = fillerBlock;
                        }
                    }
                }
            }
        }
    }
}

void InfdevWorldGenerator::generateCaves(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    uint8_t* blocks = const_cast<uint8_t*>(chunk.getBlocks());
    
    int radius = 8;
    JavaRandom rand(m_seed);
    int64_t seed1 = rand.nextLong() / 2 * 2 + 1;
    int64_t seed2 = rand.nextLong() / 2 * 2 + 1;

    for (int x = cx - radius; x <= cx + radius; ++x) {
        for (int z = cz - radius; z <= cz + radius; ++z) {
            JavaRandom chunkRand((int64_t)x * seed1 + (int64_t)z * seed2 ^ m_seed);
            
            int caveCount = chunkRand.nextInt(chunkRand.nextInt(chunkRand.nextInt(40) + 1) + 1);
            if (chunkRand.nextInt(15) != 0) caveCount = 0;

            for (int i = 0; i < caveCount; ++i) {
                double worldX = (double)(x * 16 + chunkRand.nextInt(16));
                double worldY = (double)(chunkRand.nextInt(chunkRand.nextInt(120) + 8));
                double worldZ = (double)(z * 16 + chunkRand.nextInt(16));
                
                int nodeCount = 1;
                if (chunkRand.nextInt(4) == 0) {
                    generateLargeCaveNode(cx, cz, blocks, worldX, worldY, worldZ, chunkRand);
                    nodeCount += chunkRand.nextInt(4);
                }

                for (int j = 0; j < nodeCount; ++j) {
                    float yaw = (float)(chunkRand.nextFloat() * M_PI * 2.0);
                    float pitch = (float)(chunkRand.nextFloat() - 0.5f) * 2.0f / 8.0f;
                    float scale = (float)(chunkRand.nextFloat() * 2.0f + chunkRand.nextFloat());
                    generateCaveNode(cx, cz, blocks, worldX, worldY, worldZ, scale, yaw, pitch, 0, 0, 1.0, chunkRand);
                }
            }
        }
    }
}

void InfdevWorldGenerator::generateLargeCaveNode(int cx, int cz, uint8_t* blocks, double x, double y, double z, JavaRandom& rand) {
    generateCaveNode(cx, cz, blocks, x, y, z, 1.0f + (float)(rand.nextFloat() * 6.0f), 0.0f, 0.0f, -1, -1, 0.5, rand);
}

void InfdevWorldGenerator::generateCaveNode(int cx, int cz, uint8_t* blocks, double x, double y, double z, float scale, float yaw, float pitch, int start, int end, double var15, JavaRandom& rand) {
    double centerX = (double)(cx * 16 + 8);
    double centerZ = (double)(cz * 16 + 8);
    float var21 = 0.0f;
    float var22 = 0.0f;
    
    JavaRandom nodeRand(rand.nextLong());
    
    if (end <= 0) {
        int var24 = 112;
        end = var24 - nodeRand.nextInt(var24 / 4);
    }

    bool var52 = false;
    if (start == -1) {
        start = end / 2;
        var52 = true;
    }

    int splitIndex = nodeRand.nextInt(end / 2) + end / 4;
    bool steep = nodeRand.nextInt(6) == 0;

    for (; start < end; ++start) {
        double horizontalScale = 1.5 + (double)(std::sin((float)start * (float)M_PI / (float)end) * scale);
        double verticalScale = horizontalScale * var15;
        float cosPitch = std::cos(pitch);
        float sinPitch = std::sin(pitch);
        
        x += (double)(std::cos(yaw) * cosPitch);
        y += (double)sinPitch;
        z += (double)(std::sin(yaw) * cosPitch);
        
        if (steep) {
            pitch *= 0.92f;
        } else {
            pitch *= 0.7f;
        }

        pitch += var22 * 0.1f;
        yaw += var21 * 0.1f;
        var22 *= 0.9f;
        var21 *= 0.75f;
        var22 += (nodeRand.nextFloat() - nodeRand.nextFloat()) * nodeRand.nextFloat() * 2.0f;
        var21 += (nodeRand.nextFloat() - nodeRand.nextFloat()) * nodeRand.nextFloat() * 4.0f;
        
        if (!var52 && start == splitIndex && scale > 1.0f) {
            generateCaveNode(cx, cz, blocks, x, y, z, (nodeRand.nextFloat() * 0.5f + 0.5f), yaw - (float)M_PI * 0.5f, pitch / 3.0f, start, end, 1.0, nodeRand);
            generateCaveNode(cx, cz, blocks, x, y, z, (nodeRand.nextFloat() * 0.5f + 0.5f), yaw + (float)M_PI * 0.5f, pitch / 3.0f, start, end, 1.0, nodeRand);
            return;
        }

        if (var52 || nodeRand.nextInt(4) != 0) {
            double dx = x - centerX;
            double dz = z - centerZ;
            double remainingSteps = (double)(end - start);
            double maxDist = (double)(scale + 2.0f + 16.0f);
            
            if (dx * dx + dz * dz - remainingSteps * remainingSteps > maxDist * maxDist) {
                return;
            }

            if (x >= centerX - 16.0 - horizontalScale * 2.0 && z >= centerZ - 16.0 - horizontalScale * 2.0 && x <= centerX + 16.0 + horizontalScale * 2.0 && z <= centerZ + 16.0 + horizontalScale * 2.0) {
                int xMin = std::floor(x - horizontalScale) - cx * 16 - 1;
                int xMax = std::floor(x + horizontalScale) - cx * 16 + 1;
                int yMin = std::floor(y - verticalScale) - 1;
                int yMax = std::floor(y + verticalScale) + 1;
                int zMin = std::floor(z - horizontalScale) - cz * 16 - 1;
                int zMax = std::floor(z + horizontalScale) - cz * 16 + 1;

                xMin = std::max(xMin, 0);
                xMax = std::min(xMax, 16);
                yMin = std::max(yMin, 1);
                yMax = std::min(yMax, 120);
                zMin = std::max(zMin, 0);
                zMax = std::min(zMax, 16);

                bool waterFound = false;
                for (int vx = xMin; !waterFound && vx < xMax; ++vx) {
                    for (int vz = zMin; !waterFound && vz < zMax; ++vz) {
                        for (int vy = yMax + 1; !waterFound && vy >= yMin - 1; --vy) {
                            int blockIdx = (vx << 11) | (vz << 7) | vy;
                            if (vy >= 0 && vy < 128) {
                                uint8_t b = blocks[blockIdx];
                                if (b == Block::waterMoving->blockID || b == Block::waterStill->blockID) {
                                    waterFound = true;
                                }
                                if (vy != yMin - 1 && vx != xMin && vx != xMax - 1 && vz != zMin && vz != zMax - 1) {
                                    vy = yMin;
                                }
                            }
                        }
                    }
                }

                if (!waterFound) {
                    for (int vx = xMin; vx < xMax; ++vx) {
                        double distRelX = ((double)(vx + cx * 16) + 0.5 - x) / horizontalScale;
                        for (int vz = zMin; vz < zMax; ++vz) {
                            double distRelZ = ((double)(vz + cz * 16) + 0.5 - z) / horizontalScale;
                            int columnIdx = (vx << 11) | (vz << 7);
                            bool grassAbove = false;
                            
                            for (int vy = yMax - 1; vy >= yMin; --vy) {
                                double distRelY = ((double)vy + 0.5 - y) / verticalScale;
                                if (distRelY > -0.7 && distRelX * distRelX + distRelY * distRelY + distRelZ * distRelZ < 1.0) {
                                    uint8_t b = blocks[columnIdx | vy];
                                    if (b == Block::grass->blockID) {
                                        grassAbove = true;
                                    }
                                    if (b == Block::stone->blockID || b == Block::dirt->blockID || b == Block::grass->blockID) {
                                        if (vy < 10) {
                                            blocks[columnIdx | vy] = (uint8_t)Block::lavaStill->blockID;
                                        } else {
                                            blocks[columnIdx | vy] = 0;
                                            if (grassAbove && vy > 0 && blocks[columnIdx | (vy - 1)] == Block::dirt->blockID) {
                                                blocks[columnIdx | (vy - 1)] = (uint8_t)Block::grass->blockID;
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
    }
}

double* InfdevWorldGenerator::initializeNoiseField(double* var1, int var2, int var3, int var4, int var5, int var6, int var7,
                                                   std::vector<double>& n1, std::vector<double>& n2, std::vector<double>& n3, 
                                                   std::vector<double>& n6, std::vector<double>& n7) {
    double var8 = 684.412;
    double var10 = 684.412;
    
    n6.resize(var5 * 1 * var7);
    m_noiseGen6->populateNoiseArray(n6.data(), var2, 0, var4, var5, 1, var7, 1.0, 0.0, 1.0);
    
    n7.resize(var5 * 1 * var7);
    m_noiseGen7->populateNoiseArray(n7.data(), var2, 0, var4, var5, 1, var7, 100.0, 0.0, 100.0);
    
    n3.resize(var5 * var6 * var7);
    m_noiseGen3->populateNoiseArray(n3.data(), var2, var3, var4, var5, var6, var7, var8 / 80.0, var10 / 160.0, var8 / 80.0);
    
    n1.resize(var5 * var6 * var7);
    m_noiseGen1->populateNoiseArray(n1.data(), var2, var3, var4, var5, var6, var7, var8, var10, var8);
    
    n2.resize(var5 * var6 * var7);
    m_noiseGen2->populateNoiseArray(n2.data(), var2, var3, var4, var5, var6, var7, var8, var10, var8);
    
    int var12 = 0;
    int var13 = 0;

    for (int var14 = 0; var14 < var5; ++var14) {
        for (int var15 = 0; var15 < var7; ++var15) {
            double var16 = (n6[var13] + 256.0) / 512.0;
            if (var16 > 1.0) var16 = 1.0;

            double var20 = n7[var13] / 8000.0;
            if (var20 < 0.0) var20 = -var20;

            var20 = var20 * 3.0 - 3.0;
            if (var20 < 0.0) {
                var20 /= 2.0;
                if (var20 < -1.0) var20 = -1.0;
                var20 /= 1.4;
                var20 /= 2.0;
                var16 = 0.0;
            } else {
                if (var20 > 1.0) var20 = 1.0;
                var20 /= 6.0;
            }

            var16 += 0.5;
            var20 = var20 * (double)var6 / 16.0;
            double var22 = (double)var6 / 2.0 + var20 * 4.0;
            ++var13;

            for (int var24 = 0; var24 < var6; ++var24) {
                double var25 = 0.0;
                double var27 = ((double)var24 - var22) * 12.0 / var16;
                if (var27 < 0.0) var27 *= 4.0;

                double var29 = n1[var12] / 512.0;
                double var31 = n2[var12] / 512.0;
                double var33 = (n3[var12] / 10.0 + 1.0) / 2.0;
                if (var33 < 0.0) {
                    var25 = var29;
                } else if (var33 > 1.0) {
                    var25 = var31;
                } else {
                    var25 = var29 + (var31 - var29) * var33;
                }

                var25 -= var27;
                double var35;
                if (var24 > var6 - 4) {
                    var35 = (double)((float)(var24 - (var6 - 4)) / 3.0f);
                    var25 = var25 * (1.0 - var35) + -10.0 * var35;
                }

                if ((double)var24 < 0.0) {
                    var35 = (0.0 - (double)var24) / 4.0;
                    if (var35 < 0.0) var35 = 0.0;
                    if (var35 > 1.0) var35 = 1.0;
                    var25 = var25 * (1.0 - var35) + -10.0 * var35;
                }

                var1[var12] = var25;
                ++var12;
            }
        }
    }

    return var1;
}

void InfdevWorldGenerator::decorateChunk(Chunk& chunk, Chunk* chunkE, Chunk* chunkS, Chunk* chunkSE) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    
    JavaRandom rand(m_seed);
    int64_t var6 = rand.nextLong() / 2 * 2 + 1;
    int64_t var8 = rand.nextLong() / 2 * 2 + 1;
    JavaRandom decoRand((int64_t)cx * var6 + (int64_t)cz * var8 ^ m_seed);

    auto getBlock = [&](int x, int y, int z) -> uint8_t {
        if (y < 0 || y >= 128) return 0;
        int lx = x - cx * 16;
        int lz = z - cz * 16;
        Chunk* target = nullptr;
        if (lx >= 0 && lx < 16 && lz >= 0 && lz < 16) target = &chunk;
        else if (lx >= 16 && lx < 32 && lz >= 0 && lz < 16) target = chunkE;
        else if (lx >= 0 && lx < 16 && lz >= 16 && lz < 32) target = chunkS;
        else if (lx >= 16 && lx < 32 && lz >= 16 && lz < 32) target = chunkSE;
        if (target) return target->getBlockID(lx & 15, y, lz & 15);
        return 0;
    };

    auto setBlock = [&](int x, int y, int z, uint8_t id) {
        if (y < 0 || y >= 128) return;
        int lx = x - cx * 16;
        int lz = z - cz * 16;
        Chunk* target = nullptr;
        if (lx >= 0 && lx < 16 && lz >= 0 && lz < 16) target = &chunk;
        else if (lx >= 16 && lx < 32 && lz >= 0 && lz < 16) target = chunkE;
        else if (lx >= 0 && lx < 16 && lz >= 16 && lz < 32) target = chunkS;
        else if (lx >= 16 && lx < 32 && lz >= 16 && lz < 32) target = chunkSE;
        if (target) target->setBlockIDSafe(lx & 15, y, lz & 15, id);
    };

    auto getHeight = [&](int x, int z) -> int {
        for (int y = 127; y >= 0; --y) {
            uint8_t b = getBlock(x, y, z);
            if (b != 0 && b != Block::waterStill->blockID && b != Block::waterMoving->blockID) return y + 1;
        }
        return 0;
    };

    // Ores
    auto generateOre = [&](uint8_t blockID, int count, int size, int minY, int maxY) {
        for (int i = 0; i < count; ++i) {
            int x = cx * 16 + decoRand.nextInt(16);
            int y = minY + decoRand.nextInt(maxY - minY);
            int z = cz * 16 + decoRand.nextInt(16);
            
            float angle = (float)(decoRand.nextFloat() * M_PI);
            double x1 = x + std::sin(angle) * size / 8.0;
            double x2 = x - std::sin(angle) * size / 8.0;
            double z1 = z + std::cos(angle) * size / 8.0;
            double z2 = z - std::cos(angle) * size / 8.0;
            double y1 = y + decoRand.nextInt(3) - 2;
            double y2 = y + decoRand.nextInt(3) - 2;

            for (int j = 0; j <= size; ++j) {
                double currX = x1 + (x2 - x1) * j / size;
                double currY = y1 + (y2 - y1) * j / size;
                double currZ = z1 + (z2 - z1) * j / size;
                double currSize = (std::sin(j * M_PI / size) + 1.0) * size / 16.0 + 1.0;
                
                int xStart = std::floor(currX - currSize);
                int xEnd = std::floor(currX + currSize);
                int yStart = std::floor(currY - currSize);
                int yEnd = std::floor(currY + currSize);
                int zStart = std::floor(currZ - currSize);
                int zEnd = std::floor(currZ + currSize);

                for (int vx = xStart; vx <= xEnd; ++vx) {
                    double dx = (vx + 0.5 - currX) / currSize;
                    if (dx * dx >= 1.0) continue;
                    for (int vy = yStart; vy <= yEnd; ++vy) {
                        double dy = (vy + 0.5 - currY) / currSize;
                        if (dx * dx + dy * dy >= 1.0) continue;
                        for (int vz = zStart; vz <= zEnd; ++vz) {
                            double dz = (vz + 0.5 - currZ) / currSize;
                            if (dx * dx + dy * dy + dz * dz < 1.0) {
                                if (getBlock(vx, vy, vz) == Block::stone->blockID) {
                                    setBlock(vx, vy, vz, blockID);
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    generateOre(Block::dirt->blockID, 20, 32, 0, 128);
    generateOre(Block::gravel->blockID, 10, 32, 0, 128);
    generateOre(Block::oreCoal->blockID, 20, 16, 0, 128);
    generateOre(Block::oreIron->blockID, 20, 8, 0, 64);
    generateOre(Block::oreGold->blockID, 2, 8, 0, 32);
    generateOre(Block::oreDiamond->blockID, 1, 8, 0, 16);

    // Trees
    int treeCount = (int)((m_mobSpawnerNoise->generateNoise(cx * 16 * 0.5, cz * 16 * 0.5) / 8.0 + decoRand.nextDouble() * 4.0 + 4.0) / 3.0);
    if (decoRand.nextInt(10) == 0) treeCount++;

    for (int i = 0; i < treeCount; ++i) {
        int x = cx * 16 + decoRand.nextInt(16) + 8;
        int z = cz * 16 + decoRand.nextInt(16) + 8;
        int y = getHeight(x, z);
        
        int height = decoRand.nextInt(3) + 4;
        bool canGrow = true;
        if (y >= 1 && y + height + 1 <= 128) {
            for (int vy = y; vy <= y + 1 + height; ++vy) {
                int radius = 1;
                if (vy == y) radius = 0;
                if (vy >= y + 1 + height - 2) radius = 2;
                for (int vx = x - radius; vx <= x + radius && canGrow; ++vx) {
                    for (int vz = z - radius; vz <= z + radius && canGrow; ++vz) {
                        uint8_t b = getBlock(vx, vy, vz);
                        if (b != 0 && b != Block::leaves->blockID) canGrow = false;
                    }
                }
            }

            if (canGrow) {
                uint8_t ground = getBlock(x, y - 1, z);
                if (ground == Block::grass->blockID || ground == Block::dirt->blockID) {
                    setBlock(x, y - 1, z, Block::dirt->blockID);
                    for (int vy = y - 3 + height; vy <= y + height; ++vy) {
                        int relY = vy - (y + height);
                        int radius = 1 - relY / 2;
                        for (int vx = x - radius; vx <= x + radius; ++vx) {
                            int relX = vx - x;
                            for (int vz = z - radius; vz <= z + radius; ++vz) {
                                int relZ = vz - z;
                                if (std::abs(relX) != radius || std::abs(relZ) != radius || (decoRand.nextInt(2) != 0 && relY != 0)) {
                                    if (getBlock(vx, vy, vz) == 0) setBlock(vx, vy, vz, Block::leaves->blockID);
                                }
                            }
                        }
                    }
                    for (int vy = 0; vy < height; ++vy) {
                        uint8_t b = getBlock(x, y + vy, z);
                        if (b == 0 || b == Block::leaves->blockID) setBlock(x, y + vy, z, Block::wood->blockID);
                    }
                }
            }
        }
    }

    // Flora
    for (int i = 0; i < 2; ++i) {
        int x = cx * 16 + decoRand.nextInt(16) + 8;
        int z = cz * 16 + decoRand.nextInt(16) + 8;
        int y = decoRand.nextInt(128);
        if (getBlock(x, y, z) == 0 && getBlock(x, y - 1, z) == Block::grass->blockID) setBlock(x, y, z, Block::flowerYellow->blockID);
    }
    
    if (decoRand.nextInt(2) == 0) {
        int x = cx * 16 + decoRand.nextInt(16) + 8;
        int z = cz * 16 + decoRand.nextInt(16) + 8;
        int y = decoRand.nextInt(128);
        if (getBlock(x, y, z) == 0 && getBlock(x, y - 1, z) == Block::grass->blockID) setBlock(x, y, z, Block::flowerRed->blockID);
    }

    if (decoRand.nextInt(4) == 0) {
        int x = cx * 16 + decoRand.nextInt(16) + 8;
        int z = cz * 16 + decoRand.nextInt(16) + 8;
        int y = decoRand.nextInt(128);
        if (getBlock(x, y, z) == 0 && getBlock(x, y - 1, z) == Block::grass->blockID) setBlock(x, y, z, Block::mushroomBrown->blockID);
    }

    if (decoRand.nextInt(8) == 0) {
        int x = cx * 16 + decoRand.nextInt(16) + 8;
        int z = cz * 16 + decoRand.nextInt(16) + 8;
        int y = decoRand.nextInt(128);
        if (getBlock(x, y, z) == 0 && getBlock(x, y - 1, z) == Block::grass->blockID) setBlock(x, y, z, Block::mushroomRed->blockID);
    }
}
