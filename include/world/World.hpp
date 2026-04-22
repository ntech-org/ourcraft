#pragma once

#include "world/Chunk.hpp"
#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

class World {
public:
    World() = default;

    void addChunk(std::unique_ptr<Chunk> chunk);

    Chunk* getChunk(int chunkX, int chunkZ);
    const Chunk* getChunk(int chunkX, int chunkZ) const;

    uint8_t getBlockID(int worldX, int worldY, int worldZ) const;
    void setBlockID(int worldX, int worldY, int worldZ, uint8_t id);

    void update(float deltaTime);
    float getCelestialAngle(float partialTick = 0.0f) const;
    glm::vec3 getSkyColor(float partialTick = 0.0f) const;
    glm::vec3 getFogColor(float partialTick = 0.0f) const;
    float getBrightness(int x, int y, int z) const;
    float getStarBrightness(float partialTick = 0.0f) const;
    float getDaylightStrength(float partialTick = 0.0f) const;
    glm::vec3 getSunDirection(float partialTick = 0.0f) const;
    float getHorizon() const { return 64.0f; }
    double getWorldTime() const { return m_worldTime; }

    const std::vector<std::unique_ptr<Chunk>>& getChunks() const { return m_chunks; }

private:
    static int floorDiv(int value, int divisor);
    static int floorMod(int value, int divisor);
    static std::uint64_t chunkKey(int chunkX, int chunkZ);
    static glm::vec3 unpackColor(std::uint32_t rgb);

    std::vector<std::unique_ptr<Chunk>> m_chunks;
    std::unordered_map<std::uint64_t, Chunk*> m_chunkLookup;
    double m_worldTime = 6000.0;
    std::uint32_t m_skyColor = 8961023u;
    std::uint32_t m_fogColor = 12638463u;
    std::uint32_t m_cloudColor = 16777215u;
};
