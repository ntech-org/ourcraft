#pragma once
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include "world/storage/RegionFile.hpp"
#include "world/Chunk.hpp"

class SaveHandler {
public:
    SaveHandler(const std::string& worldDir);
    ~SaveHandler() = default;

    bool loadChunk(Chunk& chunk);
    void saveChunk(const Chunk& chunk);

private:
    std::string m_worldDir;
    std::map<std::pair<int, int>, std::unique_ptr<RegionFile>> m_regions;
    std::mutex m_regionMutex;

    RegionFile* getRegionFile(int chunkX, int chunkZ);
    std::string getLegacyChunkPath(int chunkX, int chunkZ);
    bool loadLegacyChunk(Chunk& chunk);
};
