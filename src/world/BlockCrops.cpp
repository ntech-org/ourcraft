#include "world/BlockCrops.hpp"
#include "world/Block.hpp"
#include "world/JavaRandom.hpp"
#include "world/World.hpp"
#include "entities/EntityItem.hpp"
#include "items/Item.hpp"
#include <algorithm>
#include <cstdlib>

BlockCrops::BlockCrops(int id, int tex) : Block(id, tex, Material::plants) {
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
}

int BlockCrops::getTexture(int side) const {
    return blockIndexInTexture;
}

int BlockCrops::getTexture(int side, int metadata) const {
    return blockIndexInTexture + std::clamp(metadata, 0, 7);
}

bool BlockCrops::canThisPlantGrowOnThisBlockID(int id) const {
    return id == Block::farmland->blockID;
}

void BlockCrops::onBlockAdded(World& world, int x, int y, int z) const {
    if (world.getBlockID(x, y, z) == blockID) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

void BlockCrops::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    (void)neighborID;
    if (world.getBlockID(x, y, z) != blockID) return;
    int belowID = world.getBlockID(x, y - 1, z);
    if (!canThisPlantGrowOnThisBlockID(belowID)) {
        dropSeeds(world, x, y, z, world.getBlockMetadata(x, y, z));
        world.setBlockWithNotify(x, y, z, 0);
    }
}

void BlockCrops::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    if (world.getBlockID(x, y, z) != blockID) return;
    auto lightPair = world.getLightPair(x, y, z);
    int light = std::max(lightPair.first, lightPair.second);
    if (light < 9) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
        return;
    }
    int meta = world.getBlockMetadata(x, y, z);
    if (meta < 7) {
        if (random.nextInt(5) == 0) {
            meta++;
            world.setBlockMetadataWithNotify(x, y, z, (uint8_t)meta);
        }
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

int BlockCrops::idDropped(int metadata) const {
    if (metadata == 7) {
        if (Item::wheat) return Item::wheat->itemID;
    }
    return -1;
}

void BlockCrops::onBlockDestroyedByPlayer(World& world, int x, int y, int z, int metadata) const {
    if (!world.isRemote) dropSeeds(world, x, y, z, metadata);
}

void BlockCrops::dropSeeds(World& world, int x, int y, int z, int meta) const {
    if (!Item::seeds) return;
    int seedsID = Item::seeds->itemID;
    for (int i = 0; i < 3; ++i) {
        if ((std::rand() % 15) <= meta) {
            float f = 0.7f;
            float dx = ((float)std::rand() / (float)RAND_MAX) * f + (1.0f - f) * 0.5f;
            float dy = ((float)std::rand() / (float)RAND_MAX) * f + (1.0f - f) * 0.5f;
            float dz = ((float)std::rand() / (float)RAND_MAX) * f + (1.0f - f) * 0.5f;
            auto item = std::make_unique<EntityItem>(world, seedsID, 1, 0);
            item->setPosition((double)((float)x + dx), (double)((float)y + dy), (double)((float)z + dz));
            item->delayBeforeCanPickup = 10;
            world.spawnEntity(std::move(item));
        }
    }
}
