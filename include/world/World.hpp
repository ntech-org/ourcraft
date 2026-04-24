#pragma once

#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"
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
#include <set>

class Entity;

struct NextTickListEntry {
    int x, y, z;
    int blockID;
    uint64_t scheduledTime;

    bool operator<(const NextTickListEntry& other) const {
        if (scheduledTime != other.scheduledTime) {
            return scheduledTime < other.scheduledTime;
        }
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        if (z != other.z) return z < other.z;
        return blockID < other.blockID;
    }
};

class World : public IBlockAccess {
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
    void setBlockWithNotify(int worldX, int worldY, int worldZ, uint8_t id);
    void setBlockAndMetadataWithNotify(int worldX, int worldY, int worldZ, uint8_t id, uint8_t meta);

    uint8_t getBlockMetadata(int worldX, int worldY, int worldZ) const;
    void setBlockMetadataWithNotify(int worldX, int worldY, int worldZ, uint8_t meta);

    const class Material& getBlockMaterial(int worldX, int worldY, int worldZ) const;

    void scheduleBlockUpdate(int worldX, int worldY, int worldZ, int blockID, int delay);
    void notifyBlocksOfNeighborChange(int worldX, int worldY, int worldZ, int blockID);
    void notifyBlockChange(int x, int y, int z, int blockID);

    std::vector<AxisAlignedBB> getCollidingBoundingBoxes(const AxisAlignedBB& bb);
    bool handleMaterialAcceleration(const AxisAlignedBB& bb, const class Material& mat, Entity* entity);
    bool getIsAnyLiquid(const AxisAlignedBB& bb);

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

    void spawnEntity(std::unique_ptr<Entity> entity);
    void removeEntity(int32_t id);
    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return m_entities; }

    bool isRemote = false;

private:
    static int floorDiv(int value, int divisor);
    static int floorMod(int value, int divisor);
    static std::uint64_t chunkKey(int chunkX, int chunkZ);
    static glm::vec3 unpackColor(std::uint32_t rgb);

    std::vector<std::shared_ptr<Chunk>> m_chunks;
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>> m_chunkLookup;
    std::vector<std::unique_ptr<Entity>> m_entities;
    int32_t m_nextEntityID = 0;

    double m_worldTime = 6000.0;
    std::uint32_t m_skyColor = 8961023u;
    std::uint32_t m_fogColor = 12638463u;
    std::uint32_t m_cloudColor = 16777215u;
    std::unique_ptr<WorldGenerator> m_generator;
    std::unique_ptr<ChunkLoader> m_loader;
    mutable std::shared_mutex m_chunkMutex;

    std::set<NextTickListEntry> m_scheduledTickSet;
    uint64_t m_tickCount = 0;

public:
    std::unordered_set<std::uint64_t> m_pendingChunks;
};
