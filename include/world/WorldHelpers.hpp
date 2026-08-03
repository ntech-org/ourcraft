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

    if (oldID > 0 && oldID != id && Block::blocksList[oldID]) {
        Block::blocksList[oldID]->onBlockRemoval(*this, x, y, z, oldMeta);
    }

    int oldOpacity = Block::lightOpacity[oldID];
    int oldBlockLight = Block::lightValue[oldID];
    int oldSkyLight = chunk->getLight(LightType::Sky, lx, y, lz);

    chunk->setBlockID(lx, y, lz, id);
    chunk->setBlockMetadata(lx, y, lz, meta);

    if (onBlockChanged) {
        onBlockChanged(x, y, z, id, meta);
    }

    const bool waterChanged = (oldID == 8 || oldID == 9) != (id == 8 || id == 9);
    invalidateMeshDependencies(x, y, z, waterChanged);

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
