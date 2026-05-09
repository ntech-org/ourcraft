#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "world/Chunk.hpp"

namespace rocksdb {
    class DB;
}

class SaveHandler {
public:
    SaveHandler(const std::string& worldDir);
    ~SaveHandler();

    bool loadChunk(Chunk& chunk);
    void saveChunk(const Chunk& chunk);

private:
    std::string m_worldDir;
    std::unique_ptr<rocksdb::DB> m_db;

    std::string getLegacyChunkPath(int chunkX, int chunkZ);
    bool loadLegacyChunk(Chunk& chunk);
};
