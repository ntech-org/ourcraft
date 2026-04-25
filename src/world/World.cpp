#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/JavaRandom.hpp"
#include "entities/Entity.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

World::World() : m_worldTime(6000.0) {}

void World::setGenerator(std::unique_ptr<WorldGenerator> generator) {
    m_generator = std::move(generator);
    m_loader = std::make_unique<ChunkLoader>(*m_generator, this);
}

uint8_t World::getBlockID(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(x >> 4, z >> 4);
    return chunk ? chunk->getBlockID(x & 15, y, z & 15) : 0;
}

void World::setBlockID(int x, int y, int z, uint8_t id) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;
    int lx = x & 15, lz = z & 15;
    
    uint8_t oldID = chunk->getBlockID(lx, y, lz);
    if (oldID == id) return;
    
    int oldOpacity = Block::lightOpacity[oldID];
    int oldBlockLight = Block::lightValue[oldID];
    int oldSkyLight = chunk->getLight(LightType::Sky, lx, y, lz);

    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id);
    
    if (lx == 0) { if (auto n = getChunk((x >> 4) - 1, z >> 4)) n->touchSection(si); }
    else if (lx == 15) { if (auto n = getChunk((x >> 4) + 1, z >> 4)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(x >> 4, (z >> 4) - 1)) n->touchSection(si); }
    else if (lz == 15) { if (auto n = getChunk(x >> 4, (z >> 4) + 1)) n->touchSection(si); }

    updateLightForBlockChange(x, y, z, oldOpacity, Block::lightOpacity[id], oldBlockLight, Block::lightValue[id], oldSkyLight);
}

void World::setBlockWithNotify(int x, int y, int z, uint8_t id) { setBlockID(x, y, z, id); notifyBlockChange(x, y, z, id); }

void World::setBlockAndMetadataWithNotify(int x, int y, int z, uint8_t id, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;
    int lx = x & 15, lz = z & 15;
    
    uint8_t oldID = chunk->getBlockID(lx, y, lz);
    int oldOpacity = Block::lightOpacity[oldID];
    int oldBlockLight = Block::lightValue[oldID];
    int oldSkyLight = chunk->getLight(LightType::Sky, lx, y, lz);

    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id); 
    chunk->setBlockMetadata(lx, y, lz, meta);

    if (lx == 0) { if (auto n = getChunk((x >> 4) - 1, z >> 4)) n->touchSection(si); }
    else if (lx == 15) { if (auto n = getChunk((x >> 4) + 1, z >> 4)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(x >> 4, (z >> 4) - 1)) n->touchSection(si); }
    else if (lz == 15) { if (auto n = getChunk(x >> 4, (z >> 4) + 1)) n->touchSection(si); }
    
    updateLightForBlockChange(x, y, z, oldOpacity, Block::lightOpacity[id], oldBlockLight, Block::lightValue[id], oldSkyLight);

    notifyBlockChange(x, y, z, id);
}

uint8_t World::getBlockMetadata(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(x >> 4, z >> 4);
    return chunk ? chunk->getBlockMetadata(x & 15, y, z & 15) : 0;
}

void World::setBlockMetadataWithNotify(int x, int y, int z, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;
    chunk->setBlockMetadata(x & 15, y, z & 15, meta);
    notifyBlockChange(x, y, z, getBlockID(x, y, z));
}

const Material& World::getBlockMaterial(int x, int y, int z) const {
    uint8_t id = getBlockID(x, y, z);
    return id == 0 ? Material::air : Block::blocksList[id]->blockMaterial;
}

void World::scheduleBlockUpdate(int x, int y, int z, int id, int delay) {
    NextTickListEntry e; e.x = x; e.y = y; e.z = z; e.blockID = id; e.scheduledTime = m_tickCount + delay;
    m_scheduledTickSet.insert(e);
}

void World::notifyBlocksOfNeighborChange(int x, int y, int z, int id) {
    auto nc = [&](int nx, int ny, int nz) {
        uint8_t nid = getBlockID(nx, ny, nz);
        if (nid > 0 && Block::blocksList[nid]) Block::blocksList[nid]->onNeighborBlockChange(*this, nx, ny, nz, id);
    };
    nc(x - 1, y, z); nc(x + 1, y, z); nc(x, y - 1, z); nc(x, y + 1, z); nc(x, y, z - 1); nc(x, y, z + 1);
}

void World::notifyBlockChange(int x, int y, int z, int id) { notifyBlocksOfNeighborChange(x, y, z, id); }

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
                if (cid == e.blockID && cid > 0 && Block::blocksList[cid]) Block::blocksList[cid]->updateTick(*this, e.x, e.y, e.z, rand);
            }
        }
    }
    for (auto& e : m_entities) e->onUpdate();
}

void World::spawnEntity(std::unique_ptr<Entity> e) { if (e->entityID == -1) e->entityID = m_nextEntityID++; m_entities.push_back(std::move(e)); }
void World::removeEntity(int32_t id) { m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(), [id](const auto& e) { return e->entityID == id; }), m_entities.end()); }

int World::floorDiv(int v, int d) { int q = v / d, r = v % d; if (r != 0 && ((r < 0) != (d < 0))) --q; return q; }
int World::floorMod(int v, int d) { int r = v % d; return r < 0 ? r + d : r; }
std::uint64_t World::chunkKey(int cx, int cz) { return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) | static_cast<std::uint32_t>(cz); }

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
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>> localCache;
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
            if (!nChunk) return;

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
    std::unordered_map<std::uint64_t, std::shared_ptr<Chunk>> localCache;
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
            if (!nChunk) return;

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
    {
        std::vector<LightNode> addQueue; std::vector<LightRemovalNode> removeQueue;
        int current = getSavedLightValue(LightType::Block, x, y, z);
        if (newBlockLight > 0) { setLightValue(LightType::Block, x, y, z, newBlockLight); addQueue.push_back({x, y, z}); }
        else { setLightValue(LightType::Block, x, y, z, 0); if (oldBlockLight > 0) removeQueue.push_back({x, y, z, oldBlockLight}); else if (current > 0) removeQueue.push_back({x, y, z, current}); }
        if (!removeQueue.empty()) unpropagateLight(LightType::Block, removeQueue, addQueue);
        if (!addQueue.empty()) propagateLight(LightType::Block, addQueue);
    }
    {
        std::vector<LightNode> addQueue; std::vector<LightRemovalNode> removeQueue;
        if (newOpacity > oldOpacity) { removeQueue.push_back({x, y, z, oldSkyLight}); setLightValue(LightType::Sky, x, y, z, 0); if (oldSkyLight == 15) { for (int ty = y - 1; ty >= 0; --ty) { int sl = getSavedLightValue(LightType::Sky, x, ty, z); if (sl == 0) break; removeQueue.push_back({x, ty, z, sl}); setLightValue(LightType::Sky, x, ty, z, 0); } } }
        else if (newOpacity < oldOpacity) { addQueue.push_back({x, y, z}); bool canSeeSky = true; for (int ty = Chunk::HEIGHT - 1; ty > y; --ty) { if (Block::lightOpacity[getBlockID(x, ty, z)] > 0) { canSeeSky = false; break; } } if (canSeeSky) { setLightValue(LightType::Sky, x, y, z, 15); for (int ty = y - 1; ty >= 0; --ty) { if (Block::lightOpacity[getBlockID(x, ty, z)] > 0) break; setLightValue(LightType::Sky, x, ty, z, 15); addQueue.push_back({x, ty, z}); } } }
        else { addQueue.push_back({x, y, z}); }
        if (!removeQueue.empty()) unpropagateLight(LightType::Sky, removeQueue, addQueue);
        if (!addQueue.empty()) propagateLight(LightType::Sky, addQueue);
    }
}

void World::calculateInitialSkylight(Chunk& chunk) {
    chunk.generateHeightMap();
    int cx = chunk.getX() << 4, cz = chunk.getZ() << 4;
    std::vector<LightNode> skyQueue, blockQueue;
    
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int h = chunk.getHeight(x, z);
            for (int y = Chunk::HEIGHT - 1; y >= h; --y) chunk.setLightInternal(LightType::Sky, (x << 11) | (z << 7) | y, 15);
            for (int y = h - 1; y >= 0; --y) chunk.setLightInternal(LightType::Sky, (x << 11) | (z << 7) | y, 0);
            
            // Seed vertical column for 15-to-14 horizontal spread
            skyQueue.push_back({cx + x, h, cz + z});
            if (h > 0) skyQueue.push_back({cx + x, h - 1, cz + z});
            
            for (int y = 0; y < Chunk::HEIGHT; ++y) if (Block::lightValue[chunk.getBlockID(x, y, z)] > 0) blockQueue.push_back({cx + x, y, cz + z});
            
            // Seed internal horizontal spread for sunlight shafts (1-block holes)
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (std::abs(dx) + std::abs(dz) != 1) continue;
                    int nx = x + dx, nz = z + dz;
                    if (nx >= 0 && nx < 16 && nz >= 0 && nz < 16) {
                        int nh = chunk.getHeight(nx, nz);
                        if (nh > h) {
                            for (int y = h; y < nh; ++y) skyQueue.push_back({cx + x, y, cz + z});
                        }
                    }
                }
            }
        }
    }
    
    // Modern industry standard boundary seeding (Lock-free fast path)
    std::shared_ptr<Chunk> spW = getChunk((cx - 1) >> 4, cz >> 4);
    std::shared_ptr<Chunk> spE = getChunk((cx + 16) >> 4, cz >> 4);
    std::shared_ptr<Chunk> spN = getChunk(cx >> 4, (cz - 1) >> 4);
    std::shared_ptr<Chunk> spS = getChunk(cx >> 4, (cz + 16) >> 4);
    Chunk* neighborW = spW.get(); Chunk* neighborE = spE.get();
    Chunk* neighborN = spN.get(); Chunk* neighborS = spS.get();

    auto seedBoundary = [&](Chunk* nChunk, int nx, int nz) {
        if (!nChunk) return;
        for (int y = 0; y < Chunk::HEIGHT; ++y) {
            int nlx = nx & 15, nlz = nz & 15;
            int idx = (nlx << 11) | (nlz << 7) | y;
            if (nChunk->getLightInternal(LightType::Sky, idx) > 0) skyQueue.push_back({nx, y, nz});
            if (nChunk->getLightInternal(LightType::Block, idx) > 0) blockQueue.push_back({nx, y, nz});
        }
    };
    
    auto seedOurBoundary = [&](int nx, int nz) {
        for (int y = 0; y < Chunk::HEIGHT; ++y) {
            int nlx = nx & 15, nlz = nz & 15;
            int idx = (nlx << 11) | (nlz << 7) | y;
            if (chunk.getLightInternal(LightType::Sky, idx) > 0) skyQueue.push_back({nx, y, nz});
            if (chunk.getLightInternal(LightType::Block, idx) > 0) blockQueue.push_back({nx, y, nz});
        }
    };

    for (int i = 0; i < 16; ++i) {
        seedBoundary(neighborW, cx - 1, cz + i); seedBoundary(neighborE, cx + 16, cz + i);
        seedBoundary(neighborN, cx + i, cz - 1); seedBoundary(neighborS, cx + i, cz + 16);
        seedOurBoundary(cx, cz + i); seedOurBoundary(cx + 15, cz + i);
        seedOurBoundary(cx + i, cz); seedOurBoundary(cx + i, cz + 15);
    }
    
    propagateLight(LightType::Sky, skyQueue); propagateLight(LightType::Block, blockQueue);
}
