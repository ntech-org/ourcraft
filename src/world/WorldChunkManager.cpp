#include "world/World.hpp"
#include "world/Block.hpp"
#include <cmath>
#include <algorithm>

void World::addChunk(std::shared_ptr<Chunk> chunk) {
    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        uint64_t key = chunkKey(chunk->getX(), chunk->getZ());
        auto it = m_chunkLookup.find(key);
        if (it != m_chunkLookup.end()) {
            // Remove old chunk from m_chunks list
            auto& oldChunk = it->second;
            m_chunks.erase(std::remove(m_chunks.begin(), m_chunks.end(), oldChunk), m_chunks.end());
        }
        m_chunkLookup[key] = chunk;
        m_chunks.push_back(chunk);
    }
    {
        std::lock_guard<std::mutex> lock(m_newChunksMutex);
        m_newChunks.push_back(chunk);
    }
}

void World::removeChunk(int chunkX, int chunkZ) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    uint64_t key = chunkKey(chunkX, chunkZ);
    auto it = m_chunkLookup.find(key);
    if (it != m_chunkLookup.end()) {
        auto& chunk = it->second;
        saveChunk(chunk);
        m_chunks.erase(std::remove(m_chunks.begin(), m_chunks.end(), chunk), m_chunks.end());
        m_chunkLookup.erase(it);
    }
}

void World::requestChunk(int chunkX, int chunkZ) {
    std::uint64_t key = chunkKey(chunkX, chunkZ);
    
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        if (isChunkLoaded(chunkX, chunkZ) || m_pendingChunks.count(key) > 0 || m_pendingRequests.count(key) > 0) return;

        if (isRemote) {
            m_pendingRequests.insert(key);
            return;
        }
        m_pendingChunks.insert(key);
    }
    m_loader->requestChunk(chunkX, chunkZ);
}

void World::saveChunk(std::shared_ptr<Chunk> chunk) {
    if (!isRemote && m_loader) {
        m_loader->requestSave(chunk);
    }
}

void World::saveAllChunks() {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    for (auto& chunk : m_chunks) {
        saveChunk(chunk);
    }
}

bool World::pollGeneratedChunks() {
    if (isRemote || !m_loader) return false;
    bool worldChanged = false;
    std::shared_ptr<Chunk> chunk;
    int processed = 0;
    while (processed < 128 && m_loader->tryPopResult(chunk)) {
        processed++;
        int cx = chunk->getX(), cz = chunk->getZ();
        ChunkState state = chunk->getState();

        if (state == ChunkState::Generated) {
            {
                std::lock_guard<std::mutex> lock(m_pendingMutex);
                m_pendingChunks.erase(chunkKey(cx, cz));
            }
            chunk->generateBitmask();
            addChunk(chunk);
            worldChanged = true;
            m_loader->requestLighting(chunk);
        } else if (state == ChunkState::Complete) {
            // This chunk was likely loaded from disk already complete
            {
                std::lock_guard<std::mutex> lock(m_pendingMutex);
                m_pendingChunks.erase(chunkKey(cx, cz));
            }
            addChunk(chunk);
            worldChanged = true;
            
            // Wake up fluids and neighbors
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    for (int y = 0; y < Chunk::HEIGHT; ++y) {
                        uint8_t id = chunk->getBlockID(x, y, z);
                        if (id >= 8 && id <= 11) { // Any fluid
                            scheduleBlockUpdate(cx * 16 + x, y, cz * 16 + z, id, Block::blocksList[id]->tickRate());
                        }
                    }
                }
            }

            // Fully finished! Touch all neighbors to fix boundaries
            chunk->generateBitmask();
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
                if (auto n = getChunk(cx - 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx + 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx, cz - 1)) n->touchSection(i);
                if (auto n = getChunk(cx, cz + 1)) n->touchSection(i);
                chunk->touchSection(i);
            }
            {
                std::lock_guard<std::mutex> lock(m_completeChunksMutex);
                m_completeChunks.push_back(chunk);
            }
            checkChunkProgression(cx, cz);
        } else if (state == ChunkState::Lighted) {
            checkChunkProgression(cx, cz);
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) chunk->touchSection(i);
        } else if (state == ChunkState::Decorated) {
            checkChunkProgression(cx, cz);
        }
    }
    return worldChanged;
}

void World::checkChunkProgression(int cx, int cz) {
    // Check for decoration (2x2 area)
    for (int dx = -1; dx <= 0; ++dx) {
        for (int dz = -1; dz <= 0; ++dz) {
            auto c00 = getChunk(cx + dx, cz + dz), c10 = getChunk(cx + dx + 1, cz + dz);
            auto c01 = getChunk(cx + dx, cz + dz + 1), c11 = getChunk(cx + dx + 1, cz + dz + 1);
            if (c00 && c10 && c01 && c11 && 
                c00->getState() == ChunkState::Lighted && 
                c10->getState() >= ChunkState::Lighted && 
                c01->getState() >= ChunkState::Lighted && 
                c11->getState() >= ChunkState::Lighted) {
                c00->setState(ChunkState::Decorating);
                m_loader->requestDecoration(c00, c10, c01, c11);
            }
        }
    }
    
    // Check for final lighting (2x2 area)
    for (int dx = 0; dx <= 1; ++dx) {
        for (int dz = 0; dz <= 1; ++dz) {
            int tx = cx + dx, tz = cz + dz;
            auto target = getChunk(tx, tz);
            if (target && target->getState() == ChunkState::Decorated) {
                auto c00 = getChunk(tx, tz), c_10 = getChunk(tx - 1, tz);
                auto c0_1 = getChunk(tx, tz - 1), c_1_1 = getChunk(tx - 1, tz - 1);
                if (c00 && c_10 && c0_1 && c_1_1 &&
                    c00->getState() >= ChunkState::Decorated &&
                    c_10->getState() >= ChunkState::Decorated &&
                    c0_1->getState() >= ChunkState::Decorated &&
                    c_1_1->getState() >= ChunkState::Decorated) {
                    target->setState(ChunkState::LightingFinal);
                    m_loader->requestLighting(target);
                }
            }
        }
    }
}

void World::unloadFarChunks(int playerCX, int playerCZ, int keepDistance) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunks.begin();
    while (it != m_chunks.end()) {
        int cx = (*it)->getX(), cz = (*it)->getZ();
        if (std::abs(cx - playerCX) > keepDistance || std::abs(cz - playerCZ) > keepDistance) {
            auto chunk = *it;
            saveChunk(chunk);
            m_chunkLookup.erase(chunkKey(cx, cz)); it = m_chunks.erase(it);
        } else ++it;
    }
}

std::vector<std::shared_ptr<Chunk>> World::getAllChunks() const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return m_chunks;
}

std::shared_ptr<Chunk> World::getChunk(int chunkX, int chunkZ) {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunkLookup.find(chunkKey(chunkX, chunkZ));
    if (it != m_chunkLookup.end()) return it->second;
    return nullptr;
}

std::shared_ptr<const Chunk> World::getChunk(int chunkX, int chunkZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunkLookup.find(chunkKey(chunkX, chunkZ));
    if (it != m_chunkLookup.end()) return it->second;
    return nullptr;
}

bool World::isChunkLoaded(int chunkX, int chunkZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return m_chunkLookup.count(chunkKey(chunkX, chunkZ)) > 0;
}

bool World::isChunkPending(int chunkX, int chunkZ) const {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pendingChunks.count(chunkKey(chunkX, chunkZ)) > 0 || m_pendingRequests.count(chunkKey(chunkX, chunkZ)) > 0;
}

std::vector<std::shared_ptr<Chunk>> World::popNewChunks() {
    std::lock_guard<std::mutex> lock(m_newChunksMutex);
    auto res = std::move(m_newChunks);
    m_newChunks.clear();
    return res;
}

std::vector<std::shared_ptr<Chunk>> World::popCompleteChunks() {
    std::lock_guard<std::mutex> lock(m_completeChunksMutex);
    auto res = std::move(m_completeChunks);
    m_completeChunks.clear();
    return res;
}

std::vector<int32_t> World::popRemovedEntities() {
    std::lock_guard<std::mutex> lock(m_removedEntitiesMutex);
    auto res = std::move(m_removedEntities);
    m_removedEntities.clear();
    return res;
}

uint64_t World::chunkKey(int chunkX, int chunkZ) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(chunkX)) << 32) | static_cast<uint32_t>(chunkZ);
}
