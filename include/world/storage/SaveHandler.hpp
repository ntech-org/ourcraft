#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "world/Chunk.hpp"
#include "entities/InventoryPlayer.hpp"

namespace rocksdb {
    class DB;
}

struct LevelData {
    int64_t seed;
    int32_t spawnX, spawnY, spawnZ;
    int64_t time;
};

struct PlayerSaveData {
    std::string name;
    double x, y, z;
    float yaw, pitch;
    int health;
    ItemStack inventory[45]; // TOTAL_SIZE
};

class SaveHandler {
public:
    SaveHandler(const std::string& worldDir);
    ~SaveHandler();

    bool loadChunk(Chunk& chunk);
    void saveChunk(const Chunk& chunk);

    bool loadLevelData(LevelData& data);
    void saveLevelData(const LevelData& data);

    void flush();

    bool loadPlayerData(const std::string& name, PlayerSaveData& data);
    void savePlayerData(const PlayerSaveData& data);
    
    std::string getWorldDir() const { return m_worldDir; }

private:
    std::string m_worldDir;
    std::unique_ptr<rocksdb::DB> m_db;

    std::string getLegacyChunkPath(int chunkX, int chunkZ);
    bool loadLegacyChunk(Chunk& chunk);
};
