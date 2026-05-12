#include "world/BlockFluid.hpp"
#include "world/World.hpp"
#include "world/JavaRandom.hpp"

BlockStationary::BlockStationary(int id, const Material& mat) : BlockFluid(id, mat) {}

void BlockStationary::onBlockAdded(World& world, int x, int y, int z) const {
    BlockFluid::onBlockAdded(world, x, y, z);
    if (world.getBlockID(x, y, z) == blockID) {
        int meta = world.getBlockMetadata(x, y, z);
        world.setBlockIDAndMetadata(x, y, z, blockID - 1, meta);
        world.scheduleBlockUpdate(x, y, z, blockID - 1, tickRate());
    }
}

void BlockStationary::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    BlockFluid::onNeighborBlockChange(world, x, y, z, neighborID);
    if (world.getBlockID(x, y, z) == blockID) {
        bool neighborIsSameLiquid = (neighborID == blockID || neighborID == blockID - 1);
        if (!neighborIsSameLiquid) {
            int meta = world.getBlockMetadata(x, y, z);
            world.setBlockIDAndMetadata(x, y, z, blockID - 1, meta);
            world.scheduleBlockUpdate(x, y, z, blockID - 1, tickRate());
        }
    }
}

void BlockStationary::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    if (world.getBlockID(x, y, z) == blockID) {
        int meta = world.getBlockMetadata(x, y, z);
        world.setBlockIDAndMetadata(x, y, z, blockID - 1, meta);
        world.scheduleBlockUpdate(x, y, z, blockID - 1, tickRate());
    }
}
