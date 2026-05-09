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
    if (!m_db || chunk.getState() != ChunkState::Complete) return;

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
