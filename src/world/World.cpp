#include "world/World.hpp"
#include "world/WorldHelpers.hpp"
#include "world/Block.hpp"
#include "world/JavaRandom.hpp"
#include "entities/Entity.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

World::World() : m_worldTime(6000.0) {}

World::~World() {
    if (m_loader) {
        m_loader->stopWorldAccess();
    }
}

void World::initSaveHandler(const std::string& worldDir) {
    m_saveHandler = std::make_unique<SaveHandler>(worldDir);
}

void World::setGenerator(std::unique_ptr<WorldGenerator> generator) {
    m_generator = std::move(generator);
    if (!isRemote) {
        m_loader = std::make_unique<ChunkLoader>(*m_generator, this, m_saveHandler.get());
    }
}

uint8_t World::getBlockID(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(x >> 4, z >> 4);
    return chunk ? chunk->getBlockID(x & 15, y, z & 15) : 0;
}

uint8_t World::getBlockMetadata(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(x >> 4, z >> 4);
    return chunk ? chunk->getBlockMetadata(x & 15, y, z & 15) : 0;
}

const Material& World::getBlockMaterial(int x, int y, int z) const {
    uint8_t id = getBlockID(x, y, z);
    return id == 0 ? Material::air : Block::blocksList[id]->blockMaterial;
}

bool World::setBlockID(int x, int y, int z, uint8_t id) {
    if (y < 0 || y >= Chunk::HEIGHT) return false;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return false;
    int lx = x & 15, lz = z & 15;

    uint8_t oldID = chunk->getBlockID(lx, y, lz);
    if (oldID == id) return false;

    int oldOpacity = Block::lightOpacity[oldID];
    int oldBlockLight = Block::lightValue[oldID];
    int oldSkyLight = chunk->getLight(LightType::Sky, lx, y, lz);

    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id);

    if (onBlockChanged) {
        onBlockChanged(x, y, z, id, chunk->getBlockMetadata(lx, y, lz));
    }

    if (lx == 0) { if (auto n = getChunk((x >> 4) - 1, z >> 4)) n->touchSection(si); }
    else if (lx == 15) { if (auto n = getChunk((x >> 4) + 1, z >> 4)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(x >> 4, (z >> 4) - 1)) n->touchSection(si); }
    else if (lz == 15) { if (auto n = getChunk(x >> 4, (z >> 4) + 1)) n->touchSection(si); }

    updateLightForBlockChange(x, y, z, oldOpacity, Block::lightOpacity[id], oldBlockLight, Block::lightValue[id], oldSkyLight);

    if (id > 0 && Block::blocksList[id]) {
        Block::blocksList[id]->onBlockAdded(*this, x, y, z);
    }

    return true;
}

bool World::setBlockIDAndMetadata(int x, int y, int z, uint8_t id, uint8_t meta) {
    applyBlockChange(x, y, z, id, meta, false);
    return true;
}

void World::setBlockWithNotify(int x, int y, int z, uint8_t id) {
    setBlockID(x, y, z, id);
    notifyBlockChange(x, y, z, id);
}

void World::setBlockAndMetadataWithNotify(int x, int y, int z, uint8_t id, uint8_t meta) {
    applyBlockChange(x, y, z, id, meta, true);
}

void World::setBlockMetadata(int x, int y, int z, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;
    chunk->setBlockMetadata(x & 15, y, z & 15, meta);
}

void World::setBlockMetadataWithNotify(int x, int y, int z, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;

    uint8_t oldMeta = chunk->getBlockMetadata(x & 15, y, z & 15);
    if (oldMeta == meta) return;

    chunk->setBlockMetadata(x & 15, y, z & 15, meta);
    uint8_t id = getBlockID(x, y, z);
    if (onBlockChanged) {
        onBlockChanged(x, y, z, id, meta);
    }
    notifyBlockChange(x, y, z, id);
}

void World::scheduleBlockUpdate(int x, int y, int z, int id, int delay) {
    for (const auto& existing : m_scheduledTickSet) {
        if (existing.x == x && existing.y == y && existing.z == z && existing.blockID == id) {
            return;
        }
    }
    NextTickListEntry e; e.x = x; e.y = y; e.z = z; e.blockID = id; e.scheduledTime = m_tickCount + delay;
    m_scheduledTickSet.insert(e);
}

void World::notifyBlockOfNeighborChange(int x, int y, int z, int id) {
    if (!m_editingBlocks) {
        uint8_t nid = getBlockID(x, y, z);
        if (nid > 0 && Block::blocksList[nid]) {
            Block::blocksList[nid]->onNeighborBlockChange(*this, x, y, z, id);
        }
    }
}

void World::notifyBlockChange(int x, int y, int z, int id) {
    m_notificationQueue.push_back({x, y, z, id});
    if (m_processingNotifications) return;

    m_processingNotifications = true;
    while (!m_notificationQueue.empty()) {
        BlockUpdate u = m_notificationQueue.front();
        m_notificationQueue.pop_front();

        notifyBlockOfNeighborChange(u.x - 1, u.y, u.z, u.id);
        notifyBlockOfNeighborChange(u.x + 1, u.y, u.z, u.id);
        notifyBlockOfNeighborChange(u.x, u.y - 1, u.z, u.id);
        notifyBlockOfNeighborChange(u.x, u.y + 1, u.z, u.id);
        notifyBlockOfNeighborChange(u.x, u.y, u.z - 1, u.id);
        notifyBlockOfNeighborChange(u.x, u.y, u.z + 1, u.id);
    }
    m_processingNotifications = false;
}

void World::update(float dt) {
    m_worldTime = std::fmod(m_worldTime + dt * 20.0, 24000.0);
    static double ta = 0.0; ta += dt * 20.0;
    while (ta >= 1.0) {
        ta -= 1.0; m_tickCount++;
        if (!isRemote) {
            JavaRandom rand(m_tickCount);
            while (!m_scheduledTickSet.empty()) {
                auto it = m_scheduledTickSet.begin();
                if (it->scheduledTime > m_tickCount) break;
                NextTickListEntry e = *it; m_scheduledTickSet.erase(it);
                uint8_t cid = getBlockID(e.x, e.y, e.z);
                if (cid > 0 && Block::blocksList[cid]) {
                    bool canTick = (cid == e.blockID);
                    if (!canTick) {
                        if ((e.blockID == 8 || e.blockID == 9) && (cid == 8 || cid == 9)) canTick = true;
                        if ((e.blockID == 10 || e.blockID == 11) && (cid == 10 || cid == 11)) canTick = true;
                    }
                    if (canTick) Block::blocksList[cid]->updateTick(*this, e.x, e.y, e.z, rand);
                }
            }
        }
    }
    for (auto& e : m_entities) {
        e->onUpdate();
    }
    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(), [](const auto& e) { return e->isDead; }), m_entities.end());
}

HitResult World::rayTraceBlocks(glm::dvec3 start, glm::dvec3 end, bool ignoreLiquids) {
    if (std::isnan(start.x) || std::isnan(start.y) || std::isnan(start.z)) return {HitType::NONE};
    if (std::isnan(end.x) || std::isnan(end.y) || std::isnan(end.z)) return {HitType::NONE};

    int x1 = (int)std::floor(start.x);
    int y1 = (int)std::floor(start.y);
    int z1 = (int)std::floor(start.z);
    int x2 = (int)std::floor(end.x);
    int y2 = (int)std::floor(end.y);
    int z2 = (int)std::floor(end.z);

    uint8_t id = getBlockID(x1, y1, z1);
    if (id > 0) {
        if (!ignoreLiquids || Block::blocksList[id]->blockMaterial.isSolid()) {
            return {HitType::BLOCK, x1, y1, z1, -1, start};
        }
    }

    int count = 200;
    while (count-- >= 0) {
        if (x1 == x2 && y1 == y2 && z1 == z2) return {HitType::NONE};

        bool changedX = true, changedY = true, changedZ = true;
        double nextX = 999.0, nextY = 999.0, nextZ = 999.0;

        if (x2 > x1) nextX = (double)x1 + 1.0; else if (x2 < x1) nextX = (double)x1 + 0.0; else changedX = false;
        if (y2 > y1) nextY = (double)y1 + 1.0; else if (y2 < y1) nextY = (double)y1 + 0.0; else changedY = false;
        if (z2 > z1) nextZ = (double)z1 + 1.0; else if (z2 < z1) nextZ = (double)z1 + 0.0; else changedZ = false;

        double dx = 999.0, dy = 999.0, dz = 999.0;
        double vx = end.x - start.x, vy = end.y - start.y, vz = end.z - start.z;

        if (changedX) dx = (nextX - start.x) / vx;
        if (changedY) dy = (nextY - start.y) / vy;
        if (changedZ) dz = (nextZ - start.z) / vz;

        int side = -1;
        if (dx < dy && dx < dz) {
            side = (x2 > x1) ? 4 : 5;
            start.x = nextX; start.y += vy * dx; start.z += vz * dx;
        } else if (dy < dz) {
            side = (y2 > y1) ? 0 : 1;
            start.x += vx * dy; start.y = nextY; start.z += vz * dy;
        } else {
            side = (z2 > z1) ? 2 : 3;
            start.x += vx * dz; start.y += vy * dz; start.z = nextZ;
        }

        x1 = (int)std::floor(start.x) - (side == 5 ? 1 : 0);
        y1 = (int)std::floor(start.y) - (side == 1 ? 1 : 0);
        z1 = (int)std::floor(start.z) - (side == 3 ? 1 : 0);

        uint8_t hitID = getBlockID(x1, y1, z1);
        if (hitID > 0) {
            if (!ignoreLiquids || Block::blocksList[hitID]->blockMaterial.isSolid()) {
                return {HitType::BLOCK, x1, y1, z1, side, start};
            }
        }
    }
    return {HitType::NONE};
}

void World::spawnEntity(std::unique_ptr<Entity> e) { if (e->entityID == -1) e->entityID = m_nextEntityID++; m_entities.push_back(std::move(e)); }

void World::removeEntity(int32_t id, bool notify) {
    for (auto& entity : m_entities) {
        if (entity->entityID == id) {
            entity->isDead = true;
            break;
        }
    }
    if (notify) {
        std::lock_guard<std::mutex> lock(m_removedEntitiesMutex);
        m_removedEntities.push_back(id);
    }
}

int World::getSavedLightValue(LightType type, int x, int y, int z) const {
    if (y < 0) return 0;
    if (y >= Chunk::HEIGHT) return (type == LightType::Sky) ? 15 : 0;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return (type == LightType::Sky) ? 15 : 0;
    return chunk->getLight(type, x & 15, y, z & 15);
}

void World::setLightValue(LightType type, int x, int y, int z, int val) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (chunk) chunk->setLight(type, x & 15, y, z & 15, val);
}

void World::propagateLight(LightType type, std::vector<LightNode>& queue) {
    size_t head = 0;
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>, ChunkHasher> localCache;
    std::shared_ptr<Chunk> lastChunk = nullptr;
    int lastCX = -999999, lastCZ = -999999;

    auto getRawChunk = [&](int x, int z) -> Chunk* {
        int cx = x >> 4, cz = z >> 4;
        if (cx == lastCX && cz == lastCZ) return lastChunk.get();
        std::uint64_t key = chunkKey(cx, cz);
        auto it = localCache.find(key);
        if (it != localCache.end()) {
            lastCX = cx; lastCZ = cz;
            lastChunk = it->second;
            return lastChunk.get();
        }
        auto ptr = getChunk(cx, cz);
        localCache[key] = ptr;
        lastCX = cx; lastCZ = cz;
        lastChunk = ptr;
        return ptr.get();
    };

    while (head < queue.size()) {
        LightNode n = queue[head++];
        Chunk* chunk = getRawChunk(n.x, n.z);
        if (!chunk) continue;

        int lx = n.x & 15, lz = n.z & 15;
        int idx = (lx << 11) | (lz << 7) | n.y;
        int currentLight = chunk->getLightInternal(type, idx);

        auto check = [&](int nx, int ny, int nz) {
            if (ny < 0 || ny >= Chunk::HEIGHT) return;
            Chunk* nChunk = getRawChunk(nx, nz);
            if (!nChunk || !nChunk->isLightWipeComplete()) return;

            int nlx = nx & 15, nlz = nz & 15;
            int nidx = (nlx << 11) | (nlz << 7) | ny;
            int oldLight = nChunk->getLightInternal(type, nidx);
            int opacity = Block::lightOpacity[nChunk->getBlockID(nlx, ny, nlz)];

            int newLight;
            if (type == LightType::Sky && ny == n.y - 1 && currentLight == 15 && opacity == 0) newLight = 15;
            else newLight = currentLight - (opacity < 1 ? 1 : opacity);

            if (newLight > oldLight) {
                nChunk->setLightInternal(type, nidx, newLight);
                nChunk->markSectionDirtyInternal(ny >> 4);

                if ((ny & 15) == 0 && ny > 0) nChunk->markSectionDirtyInternal((ny >> 4) - 1);
                else if ((ny & 15) == 15 && ny < Chunk::HEIGHT - 1) nChunk->markSectionDirtyInternal((ny >> 4) + 1);

                if (nlx == 0) { if (auto nb = getRawChunk(nx - 1, nz)) nb->markSectionDirtyInternal(ny >> 4); }
                else if (nlx == 15) { if (auto nb = getRawChunk(nx + 1, nz)) nb->markSectionDirtyInternal(ny >> 4); }
                if (nlz == 0) { if (auto nb = getRawChunk(nx, nz - 1)) nb->markSectionDirtyInternal(ny >> 4); }
                else if (nlz == 15) { if (auto nb = getRawChunk(nx, nz + 1)) nb->markSectionDirtyInternal(ny >> 4); }

                if (queue.size() < 10000000) queue.push_back({nx, ny, nz});
            }
        };

        check(n.x, n.y - 1, n.z); check(n.x, n.y + 1, n.z);
        check(n.x - 1, n.y, n.z); check(n.x + 1, n.y, n.z);
        check(n.x, n.y, n.z - 1); check(n.x, n.y, n.z + 1);
    }
}

void World::unpropagateLight(LightType type, std::vector<LightRemovalNode>& removeQueue, std::vector<LightNode>& addQueue) {
    size_t head = 0;
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>, ChunkHasher> localCache;
    std::shared_ptr<Chunk> lastChunk = nullptr;
    int lastCX = -999999, lastCZ = -999999;

    auto getRawChunk = [&](int x, int z) -> Chunk* {
        int cx = x >> 4, cz = z >> 4;
        if (cx == lastCX && cz == lastCZ) return lastChunk.get();
        std::uint64_t key = chunkKey(cx, cz);
        auto it = localCache.find(key);
        if (it != localCache.end()) {
            lastCX = cx; lastCZ = cz;
            lastChunk = it->second;
            return lastChunk.get();
        }
        auto ptr = getChunk(cx, cz);
        localCache[key] = ptr;
        lastCX = cx; lastCZ = cz;
        lastChunk = ptr;
        return ptr.get();
    };

    while (head < removeQueue.size()) {
        LightRemovalNode n = removeQueue[head++];

        auto check = [&](int nx, int ny, int nz) {
            if (ny < 0 || ny >= Chunk::HEIGHT) return;
            Chunk* nChunk = getRawChunk(nx, nz);
            if (!nChunk || !nChunk->isLightWipeComplete()) return;

            int nlx = nx & 15, nlz = nz & 15;
            int nidx = (nlx << 11) | (nlz << 7) | ny;
            int neighborLight = nChunk->getLightInternal(type, nidx);

            bool dependent = false;
            if (neighborLight != 0) {
                if (type == LightType::Sky && ny == n.y - 1 && n.val == 15) dependent = (neighborLight == 15);
                else {
                    int opacity = Block::lightOpacity[nChunk->getBlockID(nlx, ny, nlz)];
                    if (neighborLight == n.val - (opacity < 1 ? 1 : opacity)) dependent = true;
                }
            }

            if (dependent) {
                nChunk->setLightInternal(type, nidx, 0);
                nChunk->markSectionDirtyInternal(ny >> 4);

                if ((ny & 15) == 0 && ny > 0) nChunk->markSectionDirtyInternal((ny >> 4) - 1);
                else if ((ny & 15) == 15 && ny < Chunk::HEIGHT - 1) nChunk->markSectionDirtyInternal((ny >> 4) + 1);

                if (nlx == 0) { if (auto nb = getRawChunk(nx - 1, nz)) nb->markSectionDirtyInternal(ny >> 4); }
                else if (nlx == 15) { if (auto nb = getRawChunk(nx + 1, nz)) nb->markSectionDirtyInternal(ny >> 4); }
                if (nlz == 0) { if (auto nb = getRawChunk(nx, nz - 1)) nb->markSectionDirtyInternal(ny >> 4); }
                else if (nlz == 15) { if (auto nb = getRawChunk(nx, nz + 1)) nb->markSectionDirtyInternal(ny >> 4); }

                removeQueue.push_back({nx, ny, nz, neighborLight});
            } else if (neighborLight > 0) {
                addQueue.push_back({nx, ny, nz});
            }
        };

        check(n.x, n.y - 1, n.z); check(n.x, n.y + 1, n.z);
        check(n.x - 1, n.y, n.z); check(n.x + 1, n.y, n.z);
        check(n.x, n.y, n.z - 1); check(n.x, n.y, n.z + 1);
    }
}

void World::updateLightForBlockChange(int x, int y, int z, int oldOpacity, int newOpacity, int oldBlockLight, int newBlockLight, int oldSkyLight) {
    auto queueNeighbors = [&](int x, int y, int z, std::vector<LightNode>& queue) {
        queue.push_back({x - 1, y, z}); queue.push_back({x + 1, y, z});
        queue.push_back({x, y - 1, z}); queue.push_back({x, y + 1, z});
        queue.push_back({x, y, z - 1}); queue.push_back({x, y, z + 1});
    };

    {
        std::vector<LightNode> addQueue; std::vector<LightRemovalNode> removeQueue;
        int currentSaved = getSavedLightValue(LightType::Block, x, y, z);

        if (newOpacity > oldOpacity || newBlockLight < currentSaved) {
            if (currentSaved > 0) removeQueue.push_back({x, y, z, currentSaved});
            setLightValue(LightType::Block, x, y, z, newBlockLight);
            if (newBlockLight > 0) addQueue.push_back({x, y, z});
        } else if (newBlockLight > currentSaved) {
            setLightValue(LightType::Block, x, y, z, newBlockLight);
            addQueue.push_back({x, y, z});
        } else if (newOpacity < oldOpacity) {
            queueNeighbors(x, y, z, addQueue);
        }

        if (!removeQueue.empty()) unpropagateLight(LightType::Block, removeQueue, addQueue);
        if (!addQueue.empty()) propagateLight(LightType::Block, addQueue);
    }
    {
        std::vector<LightNode> addQueue; std::vector<LightRemovalNode> removeQueue;
        if (newOpacity > oldOpacity) {
            removeQueue.push_back({x, y, z, oldSkyLight});
            setLightValue(LightType::Sky, x, y, z, 0);
            if (oldSkyLight == 15) {
                for (int ty = y - 1; ty >= 0; --ty) {
                    int sl = getSavedLightValue(LightType::Sky, x, ty, z);
                    if (sl <= 0) break;
                    removeQueue.push_back({x, ty, z, sl});
                    setLightValue(LightType::Sky, x, ty, z, 0);
                }
            }
        }
        else if (newOpacity < oldOpacity) {
            bool canSeeSky = true;
            for (int ty = Chunk::HEIGHT - 1; ty > y; --ty) {
                if (Block::lightOpacity[getBlockID(x, ty, z)] > 0) { canSeeSky = false; break; }
            }
            if (canSeeSky) {
                setLightValue(LightType::Sky, x, y, z, 15);
                addQueue.push_back({x, y, z});
                for (int ty = y - 1; ty >= 0; --ty) {
                    if (Block::lightOpacity[getBlockID(x, ty, z)] > 0) break;
                    setLightValue(LightType::Sky, x, ty, z, 15);
                    addQueue.push_back({x, ty, z});
                }
            } else {
                queueNeighbors(x, y, z, addQueue);
            }
        }
        else { addQueue.push_back({x, y, z}); }
        if (!removeQueue.empty()) unpropagateLight(LightType::Sky, removeQueue, addQueue);
        if (!addQueue.empty()) propagateLight(LightType::Sky, addQueue);
    }
}

void World::calculateInitialSkylight(Chunk& chunk) {
    chunk.generateHeightMap();
    const int cx = chunk.getX() << 4;
    const int cz = chunk.getZ() << 4;

    std::vector<LightNode> skyQueue;
    std::vector<LightNode> blockQueue;
    skyQueue.reserve(Chunk::SIZE / 2);
    blockQueue.reserve(Chunk::SIZE / 32);

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int sky = 15;
            for (int y = Chunk::HEIGHT - 1; y >= 0; --y) {
                const int idx = (x << 11) | (z << 7) | y;
                const uint8_t id = chunk.getBlockID(x, y, z);
                int opacity = 0;
                if (id != 0) {
                    opacity = Block::lightOpacity[id];
                    if (sky > 0) {
                        sky -= opacity;
                        if (sky < 0) sky = 0;
                    }
                }
                chunk.setLightInternal(LightType::Sky, idx, sky);
                if (sky > 0) {
                    skyQueue.push_back({cx + x, y, cz + z});
                }

                const int emitted = Block::lightValue[id];
                chunk.setLightInternal(LightType::Block, idx, emitted);
                if (emitted > 0) {
                    blockQueue.push_back({cx + x, y, cz + z});
                }
            }
        }
    }

    chunk.setLightWipeComplete(true);

    auto seedNeighborBoundary = [&](Chunk* neighbor, int wx, int wz) {
        if (!neighbor) return;
        for (int y = 0; y < Chunk::HEIGHT; ++y) {
            const int nidx = ((wx & 15) << 11) | ((wz & 15) << 7) | y;
            if (neighbor->getLightInternal(LightType::Sky, nidx) > 0) {
                skyQueue.push_back({wx, y, wz});
            }
            if (neighbor->getLightInternal(LightType::Block, nidx) > 0) {
                blockQueue.push_back({wx, y, wz});
            }
        }
    };

    Chunk* neighborW = getChunk(chunk.getX() - 1, chunk.getZ()).get();
    Chunk* neighborE = getChunk(chunk.getX() + 1, chunk.getZ()).get();
    Chunk* neighborN = getChunk(chunk.getX(), chunk.getZ() - 1).get();
    Chunk* neighborS = getChunk(chunk.getX(), chunk.getZ() + 1).get();

    for (int i = 0; i < 16; ++i) {
        seedNeighborBoundary(neighborW, cx - 1, cz + i);
        seedNeighborBoundary(neighborE, cx + 16, cz + i);
        seedNeighborBoundary(neighborN, cx + i, cz - 1);
        seedNeighborBoundary(neighborS, cx + i, cz + 16);
    }

    propagateLight(LightType::Sky, skyQueue);
    propagateLight(LightType::Block, blockQueue);
}

void World::predictLighting(Chunk& chunk) {
    chunk.generateHeightMap();
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int sky = 15;
            for (int y = Chunk::HEIGHT - 1; y >= 0; --y) {
                const int idx = (x << 11) | (z << 7) | y;
                const uint8_t id = chunk.getBlockID(x, y, z);
                if (id != 0) {
                    const int opacity = std::clamp(Block::lightOpacity[id], 1, 15);
                    if (sky > 0) {
                        sky -= opacity;
                        if (sky < 0) sky = 0;
                    }
                }
                chunk.setLightInternal(LightType::Sky, idx, sky);
                chunk.setLightInternal(LightType::Block, idx, Block::lightValue[id]);
            }
        }
    }
    chunk.setLightWipeComplete(true);
}

std::vector<Entity*> World::getEntitiesWithinAABB(const AxisAlignedBB& bb) {
    std::vector<Entity*> result;
    for (const auto& entity : m_entities) {
        if (entity->boundingBox.intersectsWith(bb)) {
            result.push_back(entity.get());
        }
    }
    return result;
}
