#pragma once

#include "world/Chunk.hpp"
#include "world/IBlockAccess.hpp"
#include "world/WorldGenerator.hpp"
#include "world/ChunkLoader.hpp"
#include "world/storage/SaveHandler.hpp"
#include "physics/AxisAlignedBB.hpp"
#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <set>
#include <mutex>
#include <functional>
#include <deque>

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

enum class HitType {
    NONE,
    BLOCK,
    ENTITY
};

struct ChunkHasher {
    std::size_t operator()(std::uint64_t key) const {
        key = (key ^ (key >> 30)) * 0xbf58476d1ce4e5b9ULL;
        key = (key ^ (key >> 27)) * 0x94d049bb133111ebULL;
        key = key ^ (key >> 31);
        return static_cast<std::size_t>(key);
    }
};

struct HitResult {
    HitType type = HitType::NONE;
    int x, y, z;
    int sideHit;
    glm::dvec3 hitVec;
    class Entity* entity = nullptr;
};

class World : public IBlockAccess {
public:
    World();
    ~World();

    void setGenerator(std::unique_ptr<WorldGenerator> generator);
    void initSaveHandler(const std::string& worldDir);

    void addChunk(std::shared_ptr<Chunk> chunk);
    void removeChunk(int chunkX, int chunkZ);
    void requestChunk(int chunkX, int chunkZ);
    void saveChunk(std::shared_ptr<Chunk> chunk);
    void saveAllChunks();
    bool pollGeneratedChunks();
    void checkChunkProgression(int cx, int cz);
    void unloadFarChunks(int playerCX, int playerCZ, int keepDistance);

    std::shared_ptr<Chunk> getChunk(int chunkX, int chunkZ);
    std::shared_ptr<const Chunk> getChunk(int chunkX, int chunkZ) const;

    bool isChunkLoaded(int chunkX, int chunkZ) const;
    bool isChunkPending(int chunkX, int chunkZ) const;
    
    // Fast bulk check
    void getLoadedAndPendingChunks(int playerCX, int playerCZ, int radius, 
                                   std::vector<std::pair<int, int>>& outToRequest);

    uint8_t getBlockID(int worldX, int worldY, int worldZ) const;
    bool setBlockID(int worldX, int worldY, int worldZ, uint8_t id);
    bool setBlockIDAndMetadata(int x, int y, int z, uint8_t id, uint8_t meta);
    void setBlockWithNotify(int worldX, int worldY, int worldZ, uint8_t id);
    void setBlockAndMetadataWithNotify(int worldX, int worldY, int worldZ, uint8_t id, uint8_t meta);

    uint8_t getBlockMetadata(int worldX, int worldY, int worldZ) const;
    void setBlockMetadata(int x, int y, int z, uint8_t meta);
    void setBlockMetadataWithNotify(int worldX, int worldY, int worldZ, uint8_t meta);

    const class Material& getBlockMaterial(int worldX, int worldY, int worldZ) const;

    int getSavedLightValue(LightType type, int x, int y, int z) const;
    void setLightValue(LightType type, int x, int y, int z, int val);
    
    struct LightNode { int x, y, z; };
    struct LightRemovalNode { int x, y, z, val; };
    
    void propagateLight(LightType type, std::vector<LightNode>& queue);
    void unpropagateLight(LightType type, std::vector<LightRemovalNode>& removeQueue, std::vector<LightNode>& addQueue);

    void updateLightForBlockChange(int x, int y, int z, int oldOpacity, int newOpacity, int oldBlockLight, int newBlockLight, int oldSkyLight);

    void calculateInitialSkylight(Chunk& chunk);
    void predictLighting(Chunk& chunk);

    void scheduleBlockUpdate(int worldX, int worldY, int worldZ, int blockID, int delay);
    void notifyBlocksOfNeighborChange(int worldX, int worldY, int worldZ, int blockID);
    void notifyBlockChange(int x, int y, int z, int blockID);

    std::vector<AxisAlignedBB> getCollidingBoundingBoxes(const AxisAlignedBB& bb);
    bool handleMaterialAcceleration(const AxisAlignedBB& bb, const class Material& mat, Entity* entity);
    bool getIsAnyLiquid(const AxisAlignedBB& bb);

    HitResult rayTraceBlocks(glm::dvec3 start, glm::dvec3 end, bool ignoreLiquids = false);

    std::vector<Entity*> getEntitiesWithinAABB(const AxisAlignedBB& bb);

    void update(float deltaTime);
    float getCelestialAngle(float partialTick = 0.0f) const;
    glm::vec3 getSkyColor(float partialTick = 0.0f) const;
    glm::vec3 getFogColor(float partialTick = 0.0f) const;
    std::pair<int, int> getLightPair(int x, int y, int z) const override;
    float getStarBrightness(float partialTick = 0.0f) const;
    float getDaylightStrength(float partialTick = 0.0f) const;
    glm::vec3 getSunDirection(float partialTick = 0.0f) const;
    float getHorizon() const { return 64.0f; }
    double getWorldTime() const { return m_worldTime; }
    void setWorldTime(double time) { m_worldTime = time; }

    std::vector<std::shared_ptr<Chunk>> getAllChunks() const;
    std::vector<std::shared_ptr<Chunk>> popNewChunks();
    std::vector<std::shared_ptr<Chunk>> popCompleteChunks();
    std::vector<int32_t> popRemovedEntities();

    void spawnEntity(std::unique_ptr<Entity> entity);
    void removeEntity(int32_t id, bool notify = true);
    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return m_entities; }

    SaveHandler* getSaveHandler() const { return m_saveHandler.get(); }

    bool isRemote = false;
    bool m_editingBlocks = false;
    std::function<void(int, int, int, uint8_t, uint8_t)> onBlockChanged;

private:
    struct BlockUpdate { int x, y, z; int id; };
    std::deque<BlockUpdate> m_notificationQueue;
    bool m_processingNotifications = false;

    void notifyBlockOfNeighborChange(int x, int y, int z, int neighborID);    static int floorDiv(int value, int divisor);
    static int floorMod(int value, int divisor);
    static std::uint64_t chunkKey(int chunkX, int chunkZ);
    static glm::vec3 unpackColor(std::uint32_t rgb);

    std::vector<std::shared_ptr<Chunk>> m_chunks;
    std::vector<std::shared_ptr<Chunk>> m_newChunks;
    std::mutex m_newChunksMutex;
    std::vector<std::shared_ptr<Chunk>> m_completeChunks;
    std::mutex m_completeChunksMutex;
    std::vector<int32_t> m_removedEntities;
    std::mutex m_removedEntitiesMutex;
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>, ChunkHasher> m_chunkLookup;
    std::vector<std::unique_ptr<Entity>> m_entities;
    int32_t m_nextEntityID = 0;

    double m_worldTime = 6000.0;
    std::uint32_t m_skyColor = 8961023u;
    std::uint32_t m_fogColor = 12638463u;
    std::uint32_t m_cloudColor = 16777215u;
    std::unique_ptr<WorldGenerator> m_generator;
    std::unique_ptr<SaveHandler> m_saveHandler;
    std::unique_ptr<ChunkLoader> m_loader;
    mutable std::shared_mutex m_chunkMutex;
    mutable std::mutex m_pendingMutex;

    std::set<NextTickListEntry> m_scheduledTickSet;
    uint64_t m_tickCount = 0;

public:
    std::unordered_set<std::uint64_t, ChunkHasher> m_pendingChunks;
    std::unordered_set<std::uint64_t, ChunkHasher> m_pendingRequests;
};
