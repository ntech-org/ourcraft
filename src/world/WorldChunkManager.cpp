#include "world/World.hpp"
#include "world/Block.hpp"
#include "util/Profiler.hpp"
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
    if (m_trackSectionChanges) {
        std::weak_ptr<Chunk> weakChunk = chunk;
        chunk->setSectionDirtyCallback([this, weakChunk](int sectionIndex) {
            auto dirtyChunk = weakChunk.lock();
            if (!dirtyChunk) return;
            queueSectionRemesh(dirtyChunk, sectionIndex, 2);
        });
    }
    notifyChunkUpdated(chunk);
}

void World::enableSectionChangeTracking() {
    m_trackSectionChanges = true;
    for (const auto& chunk : getAllChunks()) {
        if (!chunk) continue;
        std::weak_ptr<Chunk> weakChunk = chunk;
        chunk->setSectionDirtyCallback([this, weakChunk](int sectionIndex) {
            auto dirtyChunk = weakChunk.lock();
            if (!dirtyChunk) return;
            queueSectionRemesh(dirtyChunk, sectionIndex, 2);
        });
    }
}

void World::queueSectionRemesh(const std::shared_ptr<Chunk>& chunk, int sectionIndex, int urgency) {
    if (!chunk || sectionIndex < 0 || sectionIndex >= Chunk::SECTION_COUNT) return;
    std::lock_guard<std::mutex> lock(m_dirtySectionsMutex);
    m_dirtySections.push_back({chunk, sectionIndex, urgency});
}

std::vector<DirtySectionEvent> World::popDirtySections() {
    std::lock_guard<std::mutex> lock(m_dirtySectionsMutex);
    std::vector<DirtySectionEvent> result;
    result.swap(m_dirtySections);
    return result;
}

void World::notifyChunkUpdated(std::shared_ptr<Chunk> chunk) {
    if (!chunk) return;
    std::lock_guard<std::mutex> lock(m_newChunksMutex);
    m_newChunks.push_back(std::move(chunk));
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
    OC_ZONE_SCOPED;
    if (isRemote || !m_loader) return false;
    bool worldChanged = false;
    std::shared_ptr<Chunk> chunk;
    int processed = 0;
    while (processed < 128 && m_loader->tryPopResult(chunk)) {
        processed++;
        int cx = chunk->getX(), cz = chunk->getZ();
        ChunkState state = chunk->getState();

        if (state == ChunkState::Generated) {
            OC_ZONE_SCOPED_N("HandleGenerated");
            {
                std::lock_guard<std::mutex> lock(m_pendingMutex);
                m_pendingChunks.erase(chunkKey(cx, cz));
            }
            addChunk(chunk);
            worldChanged = true;
            // Decoration only reads block data. Defer lighting until decoration
            // completes instead of flood-filling every generated chunk twice.
            chunk->setState(ChunkState::Lighted);
        } else if (state == ChunkState::Complete) {
            finalizeChunk(chunk);
            worldChanged = true;
        } else if (state == ChunkState::LightingReady) {
            worldChanged = true;
        } else if (state == ChunkState::Lighted) {
            for (int i = 0; i < Chunk::SECTION_COUNT; ++i) chunk->touchSection(i);
        } else if (state == ChunkState::Decorated && !isChunkLoaded(cx, cz)) {
            // Old save records predate the validated-lighting marker. Insert
            // their final block data and send them through final lighting once.
            {
                std::lock_guard<std::mutex> lock(m_pendingMutex);
                m_pendingChunks.erase(chunkKey(cx, cz));
            }
            addChunk(chunk);
            worldChanged = true;
        }

        // Every completed stage can unblock work around this coordinate. This
        // also lets disk-loaded Complete chunks release LightingReady neighbors.
        checkChunkProgression(cx, cz);
        promoteLightingReadyChunks(cx, cz);
    }
    return worldChanged;
}

void World::finalizeChunk(const std::shared_ptr<Chunk>& chunk) {
    if (!chunk) return;
    OC_ZONE_SCOPED_N("HandleComplete");
    const int cx = chunk->getX();
    const int cz = chunk->getZ();
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingChunks.erase(chunkKey(cx, cz));
    }
    addChunk(chunk);

    for (int y = 0; y < Chunk::HEIGHT; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                if (x != 0 && x != 15 && z != 0 && z != 15) continue;
                uint8_t id = chunk->getBlockID(x, y, z);
                if (id == 8 || id == 10) {
                    scheduleBlockUpdate(cx * 16 + x, y, cz * 16 + z, id, Block::blocksList[id]->tickRate());
                }
            }
        }
    }

    chunk->generateBitmask();
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            if (auto neighbor = getChunk(cx + dx, cz + dz)) {
                for (int section = 0; section < Chunk::SECTION_COUNT; ++section) {
                    neighbor->touchSection(section);
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_completeChunksMutex);
        m_completeChunks.push_back(chunk);
    }
    checkChunkProgression(cx, cz);
}

void World::promoteLightingReadyChunks(int chunkX, int chunkZ) {
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            auto candidate = getChunk(chunkX + dx, chunkZ + dz);
            if (!candidate || candidate->getState() != ChunkState::LightingReady) continue;

            bool neighborhoodReady = true;
            for (int nx = -1; nx <= 1 && neighborhoodReady; ++nx) {
                for (int nz = -1; nz <= 1; ++nz) {
                    auto neighbor = getChunk(candidate->getX() + nx, candidate->getZ() + nz);
                    if (!neighbor || neighbor->getState() < ChunkState::LightingReady) {
                        neighborhoodReady = false;
                        break;
                    }
                }
            }
            if (!neighborhoodReady) continue;

            candidate->setState(ChunkState::Complete);
            finalizeChunk(candidate);
        }
    }
}

void World::checkChunkProgression(int cx, int cz) {
    OC_ZONE_SCOPED;
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
