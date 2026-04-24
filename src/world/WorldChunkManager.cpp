#include "world/World.hpp"
#include <cmath>

void World::addChunk(std::shared_ptr<Chunk> chunk) {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    m_chunkLookup[chunkKey(chunk->getX(), chunk->getZ())] = chunk;
    m_chunks.push_back(std::move(chunk));
}

void World::requestChunk(int chunkX, int chunkZ) {
    if (isChunkLoaded(chunkX, chunkZ) || isChunkPending(chunkX, chunkZ)) return;
    m_pendingChunks.insert(chunkKey(chunkX, chunkZ));
    m_loader->requestChunk(chunkX, chunkZ);
}

bool World::pollGeneratedChunks() {
    bool changed = false;
    std::shared_ptr<Chunk> chunk;
    while (m_loader->tryPopResult(chunk)) {
        int cx = chunk->getX(), cz = chunk->getZ();
        if (chunk->getState() == ChunkState::Generated) {
            m_pendingChunks.erase(chunkKey(cx, cz));
            addChunk(chunk);
            changed = true;
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
                if (auto n = getChunk(cx - 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx + 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx, cz - 1)) n->touchSection(i);
                if (auto n = getChunk(cx, cz + 1)) n->touchSection(i);
            }
            for (int dx = -1; dx <= 0; ++dx) {
                for (int dz = -1; dz <= 0; ++dz) {
                    auto c00 = getChunk(cx + dx, cz + dz), c10 = getChunk(cx + dx + 1, cz + dz);
                    auto c01 = getChunk(cx + dx, cz + dz + 1), c11 = getChunk(cx + dx + 1, cz + dz + 1);
                    if (c00 && c10 && c01 && c11 && c00->getState() == ChunkState::Generated && c10->getState() == ChunkState::Generated && c01->getState() == ChunkState::Generated && c11->getState() == ChunkState::Generated) {
                        c00->setState(ChunkState::Decorating); m_loader->requestDecoration(c00, c10, c01, c11);
                    }
                }
            }
        } else if (chunk->getState() == ChunkState::Decorated) {
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
                if (auto n = getChunk(cx - 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx + 1, cz)) n->touchSection(i);
                if (auto n = getChunk(cx, cz - 1)) n->touchSection(i);
                if (auto n = getChunk(cx, cz + 1)) n->touchSection(i);
                chunk->touchSection(i);
            }
        }
    }
    return changed;
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
