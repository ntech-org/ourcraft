#pragma once

#include "world/World.hpp"
#include "world/Block.hpp"

inline void World::applyBlockChange(int x, int y, int z, uint8_t id, uint8_t meta, bool notify) {
    auto chunk = getChunk(x >> 4, z >> 4);
    if (!chunk) return;
    int lx = x & 15, lz = z & 15;

    uint8_t oldID = chunk->getBlockID(lx, y, lz);
    uint8_t oldMeta = chunk->getBlockMetadata(lx, y, lz);
    if (oldID == id && oldMeta == meta) return;

    int oldOpacity = Block::lightOpacity[oldID];
    int oldBlockLight = Block::lightValue[oldID];
    int oldSkyLight = chunk->getLight(LightType::Sky, lx, y, lz);

    int si = Chunk::getSectionIndex(y);
    chunk->setBlockID(lx, y, lz, id);
    chunk->setBlockMetadata(lx, y, lz, meta);

    if (onBlockChanged) {
        onBlockChanged(x, y, z, id, meta);
    }

    if (lx == 0) { if (auto n = getChunk((x >> 4) - 1, z >> 4)) n->touchSection(si); }
    else if (lx == 15) { if (auto n = getChunk((x >> 4) + 1, z >> 4)) n->touchSection(si); }
    if (lz == 0) { if (auto n = getChunk(x >> 4, (z >> 4) - 1)) n->touchSection(si); }
    else if (lz == 15) { if (auto n = getChunk(x >> 4, (z >> 4) + 1)) n->touchSection(si); }

    int newOpacity = Block::lightOpacity[id];
    int newBlockLight = Block::lightValue[id];
    if (newOpacity != oldOpacity || newBlockLight != oldBlockLight || id == 0) {
        updateLightForBlockChange(x, y, z, oldOpacity, newOpacity, oldBlockLight, newBlockLight, oldSkyLight);
    }

    if (id > 0 && Block::blocksList[id]) {
        Block::blocksList[id]->onBlockAdded(*this, x, y, z);
    }

    if (notify) {
        notifyBlockChange(x, y, z, id);
    }
}
