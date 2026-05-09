#include "world/storage/SaveHandler.hpp"
#include "world/nbt/NBT.hpp"
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

SaveHandler::SaveHandler(const std::string& worldDir) : m_worldDir(worldDir) {
    if (!fs::exists(worldDir)) fs::create_directories(worldDir);
    if (!fs::exists(worldDir + "/region")) fs::create_directories(worldDir + "/region");
}

RegionFile* SaveHandler::getRegionFile(int chunkX, int chunkZ) {
    int rx = chunkX >> 5;
    int rz = chunkZ >> 5;
    auto key = std::make_pair(rx, rz);
    
    std::lock_guard<std::mutex> lock(m_regionMutex);
    if (m_regions.find(key) == m_regions.end()) {
        std::string path = m_worldDir + "/region/r." + std::to_string(rx) + "." + std::to_string(rz) + ".mca";
        m_regions[key] = std::make_unique<RegionFile>(path);
    }
    return m_regions[key].get();
}

bool SaveHandler::loadChunk(Chunk& chunk) {
    int cx = chunk.getX();
    int cz = chunk.getZ();
    RegionFile* region = getRegionFile(cx, cz);
    
    if (region->hasChunk(cx, cz)) {
        std::vector<uint8_t> data = region->readChunk(cx, cz);
        if (data.size() >= Chunk::SIZE + Chunk::SIZE / 2 * 3) {
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocks()), data.data(), Chunk::SIZE);
            std::memcpy(const_cast<uint8_t*>(chunk.getMetadata()), data.data() + Chunk::SIZE, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getSkylight()), data.data() + Chunk::SIZE + Chunk::SIZE / 2, Chunk::SIZE / 2);
            std::memcpy(const_cast<uint8_t*>(chunk.getBlocklight()), data.data() + Chunk::SIZE + Chunk::SIZE, Chunk::SIZE / 2);
            chunk.setState(ChunkState::Complete);
            chunk.setLightWipeComplete(true);
            chunk.generateHeightMap();
            return true;
        }
    }
    
    // Try legacy NBT if not in region
    return loadLegacyChunk(chunk);
}

void SaveHandler::saveChunk(const Chunk& chunk) {
    if (chunk.getState() != ChunkState::Complete) return;

    int cx = chunk.getX();    int cz = chunk.getZ();
    RegionFile* region = getRegionFile(cx, cz);
    
    std::vector<uint8_t> data(Chunk::SIZE + Chunk::SIZE / 2 * 3);
    std::memcpy(data.data(), chunk.getBlocks(), Chunk::SIZE);
    std::memcpy(data.data() + Chunk::SIZE, chunk.getMetadata(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE / 2, chunk.getSkylight(), Chunk::SIZE / 2);
    std::memcpy(data.data() + Chunk::SIZE + Chunk::SIZE, chunk.getBlocklight(), Chunk::SIZE / 2);
    
    region->writeChunk(cx, cz, data.data(), data.size());
}

std::string SaveHandler::getLegacyChunkPath(int chunkX, int chunkZ) {
    // Legacy Infdev path: world/c/xx/zz/c.x.z.dat
    // xx and zz are base-36 of (x & 63) and (z & 63)? No, it's simpler in early Infdev.
    // Let's use a common one: world/x/z/c.x.z.dat
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
    
    // Save to region immediately for conversion
    saveChunk(chunk);
    return true;
}
