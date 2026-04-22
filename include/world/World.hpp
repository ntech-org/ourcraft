#pragma once

#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include "world/ChunkLoader.hpp"
#include "physics/AxisAlignedBB.hpp"
#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>

class World {
public:
    World();

    void setGenerator(std::unique_ptr<WorldGenerator> generator);

    void addChunk(std::shared_ptr<Chunk> chunk);
    void requestChunk(int chunkX, int chunkZ);
    bool pollGeneratedChunks();
    void unloadFarChunks(int playerCX, int playerCZ, int keepDistance);

    std::shared_ptr<Chunk> getChunk(int chunkX, int chunkZ);
    std::shared_ptr<const Chunk> getChunk(int chunkX, int chunkZ) const;

    bool isChunkLoaded(int chunkX, int chunkZ) const;
    bool isChunkPending(int chunkX, int chunkZ) const;


    uint8_t getBlockID(int worldX, int worldY, int worldZ) const;
    void setBlockID(int worldX, int worldY, int worldZ, uint8_t id);

    std::vector<AxisAlignedBB> getCollidingBoundingBoxes(const AxisAlignedBB& bb);

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

    const std::vector<std::shared_ptr<Chunk>>& getChunks() const { return m_chunks; }

private:
    static int floorDiv(int value, int divisor);
    static int floorMod(int value, int divisor);
    static std::uint64_t chunkKey(int chunkX, int chunkZ);
    static glm::vec3 unpackColor(std::uint32_t rgb);

    std::vector<std::shared_ptr<Chunk>> m_chunks;
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>> m_chunkLookup;
    double m_worldTime = 6000.0;
    std::uint32_t m_skyColor = 8961023u;
    std::uint32_t m_fogColor = 12638463u;
    std::uint32_t m_cloudColor = 16777215u;
    std::unique_ptr<WorldGenerator> m_generator;
    std::unique_ptr<ChunkLoader> m_loader;
    mutable std::shared_mutex m_chunkMutex;

public:
    std::unordered_set<std::uint64_t> m_pendingChunks;
};
