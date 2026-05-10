#include "world/storage/SaveHandler.hpp"
#include "world/nbt/NBT.hpp"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

SaveHandler::SaveHandler(const std::string& worldDir) : m_worldDir(worldDir) {
    if (!fs::exists(worldDir)) fs::create_directories(worldDir);
    
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kZSTD;
    
    // Silence logs and keep them minimal
    options.info_log_level = rocksdb::ERROR_LEVEL;
    options.keep_log_file_num = 1; // Must be > 0 in some versions
    options.max_log_file_size = 1024 * 1024; // 1MB limit for the single log file
    options.stats_dump_period_sec = 0;
    options.recycle_log_file_num = 0;
    
    // Aggressive space optimization
    options.optimize_filters_for_hits = true;
    options.level_compaction_dynamic_level_bytes = true;
    
    std::string dbPath = worldDir + "/chunks_db";
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &m_db);
    if (!status.ok()) {
        std::cerr << "SaveHandler: Failed to open RocksDB at " << dbPath << ": " << status.ToString() << std::endl;
        m_db.reset();
    }
}

SaveHandler::~SaveHandler() = default;

bool SaveHandler::loadChunk(Chunk& chunk) {
    if (!m_db) return loadLegacyChunk(chunk);

    int cx = chunk.getX();
    int cz = chunk.getZ();
    
    uint64_t keyVal = (uint64_t(cx) << 32) | (uint32_t(cz));
    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), rocksdb::Slice((char*)&keyVal, 8), &value);
    
    if (status.ok()) {
        if (value.size() >= Chunk::SIZE + Chunk::SIZE / 2 * 3) {
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocks()), value.data(), Chunk::SIZE);
            std::memcpy(const_cast<uint8_t*>(chunk.getMetadata()), value.data() + Chunk::SIZE, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getSkylight()), value.data() + Chunk::SIZE + Chunk::SIZE / 2, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocklight()), value.data() + Chunk::SIZE + Chunk::SIZE, Chunk::SIZE / 2);
            chunk.setState(ChunkState::Complete);
            chunk.setLightWipeComplete(true);
            chunk.generateHeightMap();
            return true;
        }
    }
    
    return loadLegacyChunk(chunk);
}

void SaveHandler::saveChunk(const Chunk& chunk) {
    if (!m_db || chunk.getState() < ChunkState::Lighted) return;

    int cx = chunk.getX();
    int cz = chunk.getZ();
    
    std::vector<uint8_t> data(Chunk::SIZE + Chunk::SIZE / 2 * 3);
    std::memcpy(data.data(), chunk.getBlocks(), Chunk::SIZE);
    std::memcpy(data.data() + Chunk::SIZE, chunk.getMetadata(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE / 2, chunk.getSkylight(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE, chunk.getBlocklight(), Chunk::SIZE / 2);
    
    uint64_t keyVal = (uint64_t(cx) << 32) | (uint32_t(cz));
    rocksdb::Status status = m_db->Put(rocksdb::WriteOptions(), rocksdb::Slice((char*)&keyVal, 8), rocksdb::Slice((char*)data.data(), data.size()));
    
    if (!status.ok()) {
        std::cerr << "SaveHandler: Failed to save chunk (" << cx << "," << cz << "): " << status.ToString() << std::endl;
    }
}

std::string SaveHandler::getLegacyChunkPath(int chunkX, int chunkZ) {
    return m_worldDir + "/" + std::to_string(chunkX & 63) + "/" + std::to_string(chunkZ & 63) + "/c." + std::to_string(chunkX) + "." + std::to_string(chunkZ) + ".dat";
}

bool SaveHandler::loadLegacyChunk(Chunk& chunk) {
    std::string path = getLegacyChunkPath(chunk.getX(), chunk.getZ());
    if (!fs::exists(path)) return false;
    
    auto tag = nbt::readCompressed(path);
    if (!tag || tag->type != nbt::TagType::Compound) return false;
    
    auto& level = std::get<nbt::Compound>(std::get<nbt::Compound>(tag->value)["Level"]->value);
    auto& blocks = std::get<std::vector<int8_t>>(level["Blocks"]->value);
    auto& data = std::get<std::vector<int8_t>>(level["Data"]->value);
    auto& skyLight = std::get<std::vector<int8_t>>(level["SkyLight"]->value);
    auto& blockLight = std::get<std::vector<int8_t>>(level["BlockLight"]->value);
    
    std::memcpy(const_cast<uint8_t*>(chunk.getBlocks()), blocks.data(), Chunk::SIZE);
    std::memcpy(const_cast<uint8_t*>(chunk.getMetadata()), data.data(), Chunk::SIZE / 2);
    std::memcpy(const_cast<uint8_t*>(chunk.getSkylight()), skyLight.data(), Chunk::SIZE / 2);
    std::memcpy(const_cast<uint8_t*>(chunk.getBlocklight()), blockLight.data(), Chunk::SIZE / 2);
    
    chunk.setState(ChunkState::Complete);
    chunk.setLightWipeComplete(true);
    chunk.generateHeightMap();
    
    saveChunk(chunk); // Save to RocksDB immediately
    return true;
}

bool SaveHandler::loadLevelData(LevelData& data) {
    if (!m_db) return false;

    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), "__level_metadata__", &value);
    if (!status.ok()) return false;

    std::istringstream iss(value, std::ios::binary);
    auto tag = nbt::readTag(iss);
    if (!tag || tag->type != nbt::TagType::Compound) return false;

    auto& root = std::get<nbt::Compound>(tag->value);
    if (root.find("Data") == root.end() || root["Data"]->type != nbt::TagType::Compound) return false;
    auto& level = std::get<nbt::Compound>(root["Data"]->value);

    auto getLong = [&](const std::string& name, int64_t& val) {
        if (level.count(name) && level[name]->type == nbt::TagType::Long) val = std::get<int64_t>(level[name]->value);
    };
    auto getInt = [&](const std::string& name, int32_t& val) {
        if (level.count(name) && level[name]->type == nbt::TagType::Int) val = std::get<int32_t>(level[name]->value);
    };

    getLong("RandomSeed", data.seed);
    getInt("SpawnX", data.spawnX);
    getInt("SpawnY", data.spawnY);
    getInt("SpawnZ", data.spawnZ);
    getLong("Time", data.time);

    return true;
}

void SaveHandler::saveLevelData(const LevelData& data) {
    if (!m_db) return;

    auto level = std::make_shared<nbt::Tag>(nbt::TagType::Compound, "Data");
    level->value = nbt::Compound();
    auto& levelMap = std::get<nbt::Compound>(level->value);

    levelMap["RandomSeed"] = std::make_shared<nbt::Tag>(nbt::TagType::Long, "RandomSeed");
    levelMap["RandomSeed"]->value = data.seed;
    levelMap["SpawnX"] = std::make_shared<nbt::Tag>(nbt::TagType::Int, "SpawnX");
    levelMap["SpawnX"]->value = data.spawnX;
    levelMap["SpawnY"] = std::make_shared<nbt::Tag>(nbt::TagType::Int, "SpawnY");
    levelMap["SpawnY"]->value = data.spawnY;
    levelMap["SpawnZ"] = std::make_shared<nbt::Tag>(nbt::TagType::Int, "SpawnZ");
    levelMap["SpawnZ"]->value = data.spawnZ;
    levelMap["Time"] = std::make_shared<nbt::Tag>(nbt::TagType::Long, "Time");
    levelMap["Time"]->value = data.time;

    auto root = std::make_shared<nbt::Tag>(nbt::TagType::Compound, "");
    root->value = nbt::Compound();
    std::get<nbt::Compound>(root->value)["Data"] = level;

    std::ostringstream oss(std::ios::binary);
    nbt::writeTag(oss, *root);
    std::string blob = oss.str();

    rocksdb::Status status = m_db->Put(rocksdb::WriteOptions(), "__level_metadata__", blob);
    if (!status.ok()) {
        std::cerr << "SaveHandler: Failed to save level metadata: " << status.ToString() << std::endl;
    }
}

bool SaveHandler::loadPlayerData(const std::string& name, PlayerSaveData& data) {
    std::string path = m_worldDir + "/players/" + name + ".dat";
    if (!fs::exists(path)) return false;

    auto tag = nbt::readCompressed(path);
    if (!tag || tag->type != nbt::TagType::Compound) return false;

    auto& root = std::get<nbt::Compound>(tag->value);
    
    if (root.count("Pos") && root["Pos"]->type == nbt::TagType::List) {
        auto& pos = std::get<nbt::List>(root["Pos"]->value);
        if (pos.type == nbt::TagType::Double && pos.elements.size() == 3) {
            data.x = std::get<double>(pos.elements[0]->value);
            data.y = std::get<double>(pos.elements[1]->value);
            data.z = std::get<double>(pos.elements[2]->value);
        }
    }
    if (root.count("Rotation") && root["Rotation"]->type == nbt::TagType::List) {
        auto& rot = std::get<nbt::List>(root["Rotation"]->value);
        if (rot.type == nbt::TagType::Float && rot.elements.size() == 2) {
            data.yaw = std::get<float>(rot.elements[0]->value);
            data.pitch = std::get<float>(rot.elements[1]->value);
        }
    }
    if (root.count("Health") && root["Health"]->type == nbt::TagType::Int) {
        data.health = std::get<int32_t>(root["Health"]->value);
    }
    
    if (root.count("Inventory") && root["Inventory"]->type == nbt::TagType::List) {
        auto& invList = std::get<nbt::List>(root["Inventory"]->value);
        for (const auto& itemTag : invList.elements) {
            if (itemTag->type != nbt::TagType::Compound) continue;
            auto& itemMap = std::get<nbt::Compound>(itemTag->value);
            
            auto has = [&](const std::string& k, nbt::TagType t) {
                return itemMap.count(k) && itemMap[k]->type == t;
            };

            if (has("Slot", nbt::TagType::Byte) && has("id", nbt::TagType::Short) && 
                has("Count", nbt::TagType::Byte) && has("Damage", nbt::TagType::Short)) {
                int slot = std::get<int8_t>(itemMap["Slot"]->value);
                if (slot >= 0 && slot < 45) {
                    data.inventory[slot].itemID = std::get<int16_t>(itemMap["id"]->value);
                    data.inventory[slot].count = std::get<int8_t>(itemMap["Count"]->value);
                    data.inventory[slot].metadata = std::get<int16_t>(itemMap["Damage"]->value);
                }
            }
        }
    }

    data.name = name;
    return true;
}

void SaveHandler::savePlayerData(const PlayerSaveData& data) {
    std::string dir = m_worldDir + "/players";
    if (!fs::exists(dir)) fs::create_directories(dir);

    auto root = std::make_shared<nbt::Tag>(nbt::TagType::Compound, "");
    root->value = nbt::Compound();
    auto& map = std::get<nbt::Compound>(root->value);

    auto pos = std::make_shared<nbt::Tag>(nbt::TagType::List, "Pos");
    nbt::List posList; posList.type = nbt::TagType::Double;
    for (double v : {data.x, data.y, data.z}) {
        auto t = std::make_shared<nbt::Tag>(nbt::TagType::Double, "");
        t->value = v;
        posList.elements.push_back(t);
    }
    pos->value = posList;
    map["Pos"] = pos;

    auto rot = std::make_shared<nbt::Tag>(nbt::TagType::List, "Rotation");
    nbt::List rotList; rotList.type = nbt::TagType::Float;
    for (float v : {data.yaw, data.pitch}) {
        auto t = std::make_shared<nbt::Tag>(nbt::TagType::Float, "");
        t->value = v;
        rotList.elements.push_back(t);
    }
    rot->value = rotList;
    map["Rotation"] = rot;

    map["Health"] = std::make_shared<nbt::Tag>(nbt::TagType::Int, "Health");
    map["Health"]->value = (int32_t)data.health;

    auto invTag = std::make_shared<nbt::Tag>(nbt::TagType::List, "Inventory");
    nbt::List invList; invList.type = nbt::TagType::Compound;
    for (int i = 0; i < 45; ++i) {
        if (!data.inventory[i].isEmpty()) {
            auto itemMapTag = std::make_shared<nbt::Tag>(nbt::TagType::Compound, "");
            itemMapTag->value = nbt::Compound();
            auto& itemMap = std::get<nbt::Compound>(itemMapTag->value);
            
            auto slotTag = std::make_shared<nbt::Tag>(nbt::TagType::Byte, "Slot");
            slotTag->value = (int8_t)i;
            itemMap["Slot"] = slotTag;

            auto idTag = std::make_shared<nbt::Tag>(nbt::TagType::Short, "id");
            idTag->value = (int16_t)data.inventory[i].itemID;
            itemMap["id"] = idTag;

            auto countTag = std::make_shared<nbt::Tag>(nbt::TagType::Byte, "Count");
            countTag->value = (int8_t)data.inventory[i].count;
            itemMap["Count"] = countTag;

            auto damageTag = std::make_shared<nbt::Tag>(nbt::TagType::Short, "Damage");
            damageTag->value = (int16_t)data.inventory[i].metadata;
            itemMap["Damage"] = damageTag;

            invList.elements.push_back(itemMapTag);
        }
    }
    invTag->value = invList;
    map["Inventory"] = invTag;

    nbt::writeCompressed(dir + "/" + data.name + ".dat", *root);
}

void SaveHandler::flush() {
    if (m_db) {
        m_db->Flush(rocksdb::FlushOptions());
    }
}
