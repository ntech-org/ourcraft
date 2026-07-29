#pragma once

#include "world/WorldGenerator.hpp"
#include "world/NoiseGeneratorOctaves.hpp"
#include "world/JavaRandom.hpp"
#include <memory>
#include <vector>
#include <cstdio>
#include <cstdint>

class InfdevWorldGenerator : public WorldGenerator {
public:
    InfdevWorldGenerator(int64_t seed);
    void generateChunk(Chunk& chunk) override;
    
    static bool isFarLandsEnabledStatic() { return s_farLands; }
    
    void decorateChunk(Chunk& chunk, Chunk* chunkE, Chunk* chunkS, Chunk* chunkSE) override;

    void setFarLands(bool enabled) override {
        m_farLands = enabled;
        s_farLands = enabled;
        printf("[FARLANDS] setFarLands(%d)\n", enabled);
    }

private:
    void generateTerrain(Chunk& chunk);
    void replaceSurface(Chunk& chunk);
    void generateCaves(Chunk& chunk);
    
    void generateLargeCaveNode(int cx, int cz, uint8_t* blocks, double x, double y, double z, JavaRandom& rand);
    void generateCaveNode(int cx, int cz, uint8_t* blocks, double x, double y, double z, float scale, float yaw, float pitch, int start, int end, double var15, JavaRandom& rand);

    double* initializeNoiseField(double* noiseArray, int x, int y, int z, int xSize, int ySize, int zSize, 
                                 std::vector<double>& n1, std::vector<double>& n2, std::vector<double>& n3, 
                                 std::vector<double>& n6, std::vector<double>& n7);

    int64_t m_seed;
    bool m_farLands = false;
    static bool s_farLands;
    static constexpr int32_t FARLANDS_OFFSET = 12550824;
    
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen1;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen2;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen3;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen4;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen5;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen6;
    std::unique_ptr<NoiseGeneratorOctaves> m_noiseGen7;
    std::unique_ptr<NoiseGeneratorOctaves> m_mobSpawnerNoise;
};
