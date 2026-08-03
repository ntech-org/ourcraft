#include "world/storage/SaveHandler.hpp"
#include "world/storage/NbtHelpers.hpp"
#include "world/nbt/NBT.hpp"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

namespace {
constexpr std::size_t CHUNK_DATA_SIZE = Chunk::SIZE + Chunk::SIZE / 2 * 3;
constexpr uint8_t CHUNK_LIGHTING_VERSION = 1;

std::string chestKey(int x, int y, int z) {
    return "__chest__:" + std::to_string(x) + ":" + std::to_string(y) + ":" + std::to_string(z);
}
}

SaveHandler::SaveHandler(const std::string& worldDir) : m_worldDir(worldDir) {
    if (!fs::exists(worldDir)) fs::create_directories(worldDir);

    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kZSTD;
    options.info_log_level = rocksdb::ERROR_LEVEL;
    options.keep_log_file_num = 1;
    options.max_log_file_size = 1024 * 1024;
    options.stats_dump_period_sec = 0;
    options.recycle_log_file_num = 0;
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

bool SaveHandler::loadChest(int x, int y, int z, ItemStack* contents, int size) {
    if (!m_db || !contents || size <= 0) return false;
    std::string value;
    if (!m_db->Get(rocksdb::ReadOptions(), chestKey(x, y, z), &value).ok()) return false;
    constexpr std::size_t stackSize = sizeof(int32_t) * 3 + sizeof(uint8_t);
    if (value.size() != (std::size_t)size * stackSize) return false;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
    for (int i = 0; i < size; ++i) {
        int32_t itemID, count, damage;
        std::memcpy(&itemID, data, 4); data += 4;
        std::memcpy(&count, data, 4); data += 4;
        uint8_t metadata = *data++;
        std::memcpy(&damage, data, 4); data += 4;
        contents[i] = {itemID, count, metadata, damage};
    }
    return true;
}

void SaveHandler::saveChest(int x, int y, int z, const ItemStack* contents, int size) {
    if (!m_db || !contents || size <= 0) return;
    std::string value;
    value.reserve((std::size_t)size * 13);
    for (int i = 0; i < size; ++i) {
        const ItemStack& stack = contents[i];
        value.append(reinterpret_cast<const char*>(&stack.itemID), 4);
        value.append(reinterpret_cast<const char*>(&stack.count), 4);
        value.push_back((char)stack.metadata);
        value.append(reinterpret_cast<const char*>(&stack.damage), 4);
    }
    m_db->Put(rocksdb::WriteOptions(), chestKey(x, y, z), value);
}

void SaveHandler::removeChest(int x, int y, int z) {
    if (m_db) m_db->Delete(rocksdb::WriteOptions(), chestKey(x, y, z));
}

bool SaveHandler::loadChunk(Chunk& chunk) {
    if (!m_db) return loadLegacyChunk(chunk);

    int cx = chunk.getX();
    int cz = chunk.getZ();

    uint64_t keyVal = (uint64_t(cx) << 32) | (uint32_t(cz));
    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), rocksdb::Slice((char*)&keyVal, 8), &value);

    if (status.ok()) {
        if (value.size() >= CHUNK_DATA_SIZE) {
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocks()), value.data(), Chunk::SIZE);
            std::memcpy(const_cast<uint8_t*>(chunk.getMetadata()), value.data() + Chunk::SIZE, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getSkylight()), value.data() + Chunk::SIZE + Chunk::SIZE / 2, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocklight()), value.data() + Chunk::SIZE + Chunk::SIZE, Chunk::SIZE / 2);
            const bool lightingCurrent = value.size() > CHUNK_DATA_SIZE &&
                static_cast<uint8_t>(value[CHUNK_DATA_SIZE]) == CHUNK_LIGHTING_VERSION;
            chunk.setState(lightingCurrent ? ChunkState::Complete : ChunkState::Decorated);
            chunk.setLightWipeComplete(true);
            chunk.generateHeightMap();
            return true;
        }
    }

    return loadLegacyChunk(chunk);
}

void SaveHandler::saveChunk(const Chunk& chunk) {
    if (!m_db || chunk.getState() != ChunkState::Complete) return;

    int cx = chunk.getX();
    int cz = chunk.getZ();

    std::vector<uint8_t> data(CHUNK_DATA_SIZE + 1);
    std::memcpy(data.data(), chunk.getBlocks(), Chunk::SIZE);
    std::memcpy(data.data() + Chunk::SIZE, chunk.getMetadata(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE / 2, chunk.getSkylight(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE, chunk.getBlocklight(), Chunk::SIZE / 2);
    data[CHUNK_DATA_SIZE] = CHUNK_LIGHTING_VERSION;

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

    chunk.setState(ChunkState::Decorated);
    chunk.setLightWipeComplete(true);
    chunk.generateHeightMap();

    return true;
}

bool SaveHandler::loadLevelData(LevelData& data) {
    if (!m_db) return false;

    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), "__level_metadata__", &value);
    if (!status.ok()) return false;

    std::shared_ptr<nbt::Tag> tag;
    try {
        tag = stringToNbt(value);
    } catch (const std::exception& e) {
        std::cerr << "SaveHandler: Failed to parse level metadata: " << e.what() << std::endl;
        m_db->Delete(rocksdb::WriteOptions(), "__level_metadata__");
        return false;
    }
    if (!tag || tag->type != nbt::TagType::Compound) return false;

    auto& root = std::get<nbt::Compound>(tag->value);
    if (root.find("Data") == root.end() || root["Data"]->type != nbt::TagType::Compound) return false;
    auto& level = std::get<nbt::Compound>(root["Data"]->value);

    getNbtLong(level, "RandomSeed", data.seed);
    getNbtInt(level, "SpawnX", data.spawnX);
    getNbtInt(level, "SpawnY", data.spawnY);
    getNbtInt(level, "SpawnZ", data.spawnZ);
    getNbtLong(level, "Time", data.time);
    int8_t farLandsByte = 0;
    if (getNbtByte(level, "FarLands", farLandsByte)) data.farLands = (farLandsByte != 0);

    return true;
}

void SaveHandler::saveLevelData(const LevelData& data) {
    if (!m_db) return;

    auto level = makeNbtCompound("Data");
    auto& levelMap = std::get<nbt::Compound>(level->value);

    setNbtLong(levelMap, "RandomSeed", data.seed);
    setNbtInt(levelMap, "SpawnX", data.spawnX);
    setNbtInt(levelMap, "SpawnY", data.spawnY);
    setNbtInt(levelMap, "SpawnZ", data.spawnZ);
    setNbtLong(levelMap, "Time", data.time);
    setNbtByte(levelMap, "FarLands", data.farLands ? 1 : 0);

    auto root = makeNbtCompound();
    std::get<nbt::Compound>(root->value)["Data"] = level;

    std::string blob = nbtToString(*root);
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

    if (root.count("GameType") && root["GameType"]->type == nbt::TagType::Int) {
        data.gameMode = std::get<int32_t>(root["GameType"]->value);
    }

    if (root.count("Inventory") && root["Inventory"]->type == nbt::TagType::List) {
        auto& invList = std::get<nbt::List>(root["Inventory"]->value);
        for (const auto& itemTag : invList.elements) {
            if (itemTag->type != nbt::TagType::Compound) continue;
            auto& itemMap = std::get<nbt::Compound>(itemTag->value);

            int8_t slot = 0;
            int16_t id = 0, damage = 0;
            int8_t count = 0;
            if (getNbtByte(itemMap, "Slot", slot) && getNbtShort(itemMap, "id", id) &&
                getNbtByte(itemMap, "Count", count) && getNbtShort(itemMap, "Damage", damage)) {
                if (slot >= 0 && slot < 45) {
                    data.inventory[slot].itemID = id;
                    data.inventory[slot].count = count;
                    data.inventory[slot].metadata = damage;
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

    auto root = makeNbtCompound();
    auto& map = std::get<nbt::Compound>(root->value);

    auto pos = makeNbtList("Pos", nbt::TagType::Double);
    auto& posList = std::get<nbt::List>(pos->value);
    addNbtDouble(posList, data.x);
    addNbtDouble(posList, data.y);
    addNbtDouble(posList, data.z);
    map["Pos"] = pos;

    auto rot = makeNbtList("Rotation", nbt::TagType::Float);
    auto& rotList = std::get<nbt::List>(rot->value);
    addNbtFloat(rotList, data.yaw);
    addNbtFloat(rotList, data.pitch);
    map["Rotation"] = rot;

    setNbtInt(map, "Health", (int32_t)data.health);
    setNbtInt(map, "GameType", (int32_t)data.gameMode);

    auto invTag = makeNbtList("Inventory", nbt::TagType::Compound);
    auto& invList = std::get<nbt::List>(invTag->value);
    for (int i = 0; i < 45; ++i) {
        if (!data.inventory[i].isEmpty()) {
            auto itemMapTag = makeNbtCompound();
            auto& itemMap = std::get<nbt::Compound>(itemMapTag->value);

            setNbtByte(itemMap, "Slot", (int8_t)i);
            setNbtShort(itemMap, "id", (int16_t)data.inventory[i].itemID);
            setNbtByte(itemMap, "Count", (int8_t)data.inventory[i].count);
            setNbtShort(itemMap, "Damage", (int16_t)data.inventory[i].metadata);

            invList.elements.push_back(itemMapTag);
        }
    }
    map["Inventory"] = invTag;

    nbt::writeCompressed(dir + "/" + data.name + ".dat", *root);
}

void SaveHandler::flush() {
    if (m_db) {
        m_db->Flush(rocksdb::FlushOptions());
    }
}
