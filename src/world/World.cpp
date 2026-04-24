#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/JavaRandom.hpp"
#include "entities/Entity.hpp"
#include <algorithm>
#include <cmath>

World::World() : m_worldTime(6000.0) {}

void World::setGenerator(std::unique_ptr<WorldGenerator> generator) {
    m_generator = std::move(generator);
    m_loader = std::make_unique<ChunkLoader>(*m_generator);
}

uint8_t World::getBlockID(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(floorDiv(x, Chunk::WIDTH), floorDiv(z, Chunk::DEPTH));
    return chunk ? chunk->getBlockID(floorMod(x, Chunk::WIDTH), y, floorMod(z, Chunk::DEPTH)) : 0;
}

void World::setBlockID(int x, int y, int z, uint8_t id) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    int cx = floorDiv(x, Chunk::WIDTH), cz = floorDiv(z, Chunk::DEPTH);
    auto chunk = getChunk(cx, cz);
    if (!chunk) return;
    int lx = floorMod(x, Chunk::WIDTH), lz = floorMod(z, Chunk::DEPTH);
    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id);
    if (lx == 0) { if (auto n = getChunk(cx - 1, cz)) n->touchSection(si); }
    else if (lx == Chunk::WIDTH - 1) { if (auto n = getChunk(cx + 1, cz)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(cx, cz - 1)) n->touchSection(si); }
    else if (lz == Chunk::DEPTH - 1) { if (auto n = getChunk(cx, cz + 1)) n->touchSection(si); }
}

void World::setBlockWithNotify(int x, int y, int z, uint8_t id) { setBlockID(x, y, z, id); notifyBlockChange(x, y, z, id); }

void World::setBlockAndMetadataWithNotify(int x, int y, int z, uint8_t id, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    int cx = floorDiv(x, Chunk::WIDTH), cz = floorDiv(z, Chunk::DEPTH);
    auto chunk = getChunk(cx, cz);
    if (!chunk) return;
    int lx = floorMod(x, Chunk::WIDTH), lz = floorMod(z, Chunk::DEPTH);
    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id); chunk->setBlockMetadata(lx, y, lz, meta);
    if (lx == 0) { if (auto n = getChunk(cx - 1, cz)) n->touchSection(si); }
    else if (lx == Chunk::WIDTH - 1) { if (auto n = getChunk(cx + 1, cz)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(cx, cz - 1)) n->touchSection(si); }
    else if (lz == Chunk::DEPTH - 1) { if (auto n = getChunk(cx, cz + 1)) n->touchSection(si); }
    notifyBlockChange(x, y, z, id);
}

uint8_t World::getBlockMetadata(int x, int y, int z) const {
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    auto chunk = getChunk(floorDiv(x, Chunk::WIDTH), floorDiv(z, Chunk::DEPTH));
    return chunk ? chunk->getBlockMetadata(floorMod(x, Chunk::WIDTH), y, floorMod(z, Chunk::DEPTH)) : 0;
}

void World::setBlockMetadataWithNotify(int x, int y, int z, uint8_t meta) {
    if (y < 0 || y >= Chunk::HEIGHT) return;
    auto chunk = getChunk(floorDiv(x, Chunk::WIDTH), floorDiv(z, Chunk::DEPTH));
    if (!chunk) return;
    chunk->setBlockMetadata(floorMod(x, Chunk::WIDTH), y, floorMod(z, Chunk::DEPTH), meta);
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
