#include "world/World.hpp"
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
        m_chunks.erase(std::remove(m_chunks.begin(), m_chunks.end(), chunk), m_chunks.end());
        m_chunkLookup.erase(it);
    }
}

void World::requestChunk(int chunkX, int chunkZ) {
    std::uint64_t key = chunkKey(chunkX, chunkZ);
    if (isChunkLoaded(chunkX, chunkZ) || isChunkPending(chunkX, chunkZ)) return;

    if (isRemote) {
        if (m_pendingRequests.find(key) == m_pendingRequests.end()) {
            m_pendingRequests.insert(key);
            // This is a bit of a hack since World doesn't know about NetworkHandler,
            // but we can assume someone else will poll m_pendingRequests or 
            // we'll find a better way. Wait, we have the Minecraft instance 
            // but World doesn't. 
            // Let's use a callback or just check m_pendingRequests in GameRenderer.
        }
        return;
    }
    m_pendingChunks.insert(key);
    m_loader->requestChunk(chunkX, chunkZ);
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
            m_pendingChunks.erase(chunkKey(cx, cz));
            chunk->generateBitmask();
            addChunk(chunk);
            worldChanged = true;
            m_loader->requestLighting(chunk);
        } else if (state == ChunkState::Lighted) {
            // Initial lighting done, now check for decoration
            for (int dx = -1; dx <= 0; ++dx) {
                for (int dz = -1; dz <= 0; ++dz) {
                    auto c00 = getChunk(cx + dx, cz + dz), c10 = getChunk(cx + dx + 1, cz + dz);
                    auto c01 = getChunk(cx + dx, cz + dz + 1), c11 = getChunk(cx + dx + 1, cz + dz + 1);
                    if (c00 && c10 && c01 && c11 && 
                        c00->getState() == ChunkState::Lighted && 
                        (c10->getState() >= ChunkState::Lighted) && 
                        (c01->getState() >= ChunkState::Lighted) && 
                        (c11->getState() >= ChunkState::Lighted)) {
                        c00->setState(ChunkState::Decorating);
                        m_loader->requestDecoration(c00, c10, c01, c11);
                    }
                }
            }
            // Trigger first mesh build
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) chunk->touchSection(i);
        } else if (state == ChunkState::Decorated) {
            // Decoration done, now final lighting pass to fix shadows from trees etc.
            m_loader->requestLighting(chunk);
        } else if (state == ChunkState::Complete) {
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
        }
    }
    return worldChanged;
}

void World::unloadFarChunks(int playerCX, int playerCZ, int keepDistance) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunks.begin();
    while (it != m_chunks.end()) {
        int cx = (*it)->getX(), cz = (*it)->getZ();
        if (std::abs(cx - playerCX) > keepDistance || std::abs(cz - playerCZ) > keepDistance) {
            m_chunkLookup.erase(chunkKey(cx, cz)); it = m_chunks.erase(it);
        } else ++it;
    }
}

void World::getLoadedAndPendingChunks(int playerCX, int playerCZ, int radius, std::vector<std::pair<int, int>>& outToRequest) {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            int cx = playerCX + dx;
            int cz = playerCZ + dz;
            uint64_t key = chunkKey(cx, cz);
            if (m_chunkLookup.find(key) == m_chunkLookup.end() && m_pendingChunks.find(key) == m_pendingChunks.end()) {
                outToRequest.push_back({cx, cz});
            }
        }
    }
    
    std::sort(outToRequest.begin(), outToRequest.end(), [playerCX, playerCZ](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        int dxa = a.first - playerCX;
        int dza = a.second - playerCZ;
        int dxb = b.first - playerCX;
        int dzb = b.second - playerCZ;
        return (dxa * dxa + dza * dza) < (dxb * dxb + dzb * dzb);
    });
}

bool World::isChunkLoaded(int cx, int cz) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return m_chunkLookup.find(chunkKey(cx, cz)) != m_chunkLookup.end();
}

bool World::isChunkPending(int cx, int cz) const {
    return m_pendingChunks.find(chunkKey(cx, cz)) != m_pendingChunks.end();
}

std::shared_ptr<Chunk> World::getChunk(int cx, int cz) {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunkLookup.find(chunkKey(cx, cz));
    return it == m_chunkLookup.end() ? nullptr : it->second;
}

std::shared_ptr<const Chunk> World::getChunk(int cx, int cz) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    auto it = m_chunkLookup.find(chunkKey(cx, cz));
    return it == m_chunkLookup.end() ? nullptr : it->second;
}

std::vector<std::shared_ptr<Chunk>> World::popNewChunks() {
    std::lock_guard<std::mutex> lock(m_newChunksMutex);
    std::vector<std::shared_ptr<Chunk>> n = std::move(m_newChunks);
    m_newChunks.clear();
    return n;
}

std::vector<std::shared_ptr<Chunk>> World::popCompleteChunks() {
    std::lock_guard<std::mutex> lock(m_completeChunksMutex);
    std::vector<std::shared_ptr<Chunk>> n = std::move(m_completeChunks);
    m_completeChunks.clear();
    return n;
}
