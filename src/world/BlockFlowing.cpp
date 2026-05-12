#include "world/BlockFluid.hpp"
#include "world/World.hpp"
#include "world/JavaRandom.hpp"

BlockFlowing::BlockFlowing(int id, const Material& mat) : BlockFluid(id, mat) {}

void BlockFlowing::onBlockAdded(World& world, int x, int y, int z) const {
    BlockFluid::onBlockAdded(world, x, y, z);
    if (world.getBlockID(x, y, z) == blockID) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

void BlockFlowing::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    BlockFluid::onNeighborBlockChange(world, x, y, z, neighborID);
    if (world.getBlockID(x, y, z) == blockID) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

void BlockFlowing::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    int decay = getFlowDecay(world, x, y, z);
    if (decay < 0) return;

    int fluidType = (blockMaterial == Material::lava ? 2 : 1);
    bool flag = true;

    if (decay > 0) {
        int smallestDecay = -100;
        int numAdjacentSources = 0;
        smallestDecay = getSmallestFlowDecay(world, x - 1, y, z, smallestDecay, numAdjacentSources);
        smallestDecay = getSmallestFlowDecay(world, x + 1, y, z, smallestDecay, numAdjacentSources);
        smallestDecay = getSmallestFlowDecay(world, x, y, z - 1, smallestDecay, numAdjacentSources);
        smallestDecay = getSmallestFlowDecay(world, x, y, z + 1, smallestDecay, numAdjacentSources);

        int newDecay = smallestDecay + fluidType;
        if (newDecay >= 8 || smallestDecay < 0) {
            newDecay = -1;
        }

        int topDecay = getFlowDecay(world, x, y + 1, z);
        if (topDecay >= 0) {
            if (topDecay >= 8) newDecay = topDecay;
            else newDecay = topDecay + 8;
        }

        if (numAdjacentSources >= 2 && blockMaterial == Material::water) {
            const Material& belowMat = world.getBlockMaterial(x, y - 1, z);
            if (belowMat.isSolid() || belowMat == blockMaterial) {
                newDecay = 0;
            }
        }

        if (blockMaterial == Material::lava && decay < 8 && newDecay < 8 && newDecay > decay && random.nextInt(4) != 0) {
            newDecay = decay;
            flag = false;
        }

        if (newDecay != decay) {
            decay = newDecay;
            if (decay < 0) {
                world.setBlockWithNotify(x, y, z, 0);
            } else {
                world.setBlockMetadataWithNotify(x, y, z, decay);
                world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
                world.notifyBlockChange(x, y, z, blockID);
            }
        } else if (flag) {
            if (world.getBlockID(x, y, z) == blockID && world.getBlockMetadata(x, y, z) != decay) {
                world.setBlockIDAndMetadata(x, y, z, blockID + 1, decay);
            }
        }
    } else {
    }

    if (liquidCanDisplaceBlock(world, x, y - 1, z)) {
        if (blockMaterial == Material::lava && world.getBlockMaterial(x, y - 1, z) == Material::water) {
            world.setBlockWithNotify(x, y - 1, z, Block::stone->blockID);
            triggerLavaMixEffects(world, x, y - 1, z);
            return;
        }
        int belowId = world.getBlockID(x, y - 1, z);
        int belowMeta = world.getBlockMetadata(x, y - 1, z);
        if (decay >= 8) {
            if (belowId != blockID || belowMeta != decay) {
                world.setBlockAndMetadataWithNotify(x, y - 1, z, blockID, decay);
            }
        } else {
            if (belowId != blockID || belowMeta != decay + 8) {
                world.setBlockAndMetadataWithNotify(x, y - 1, z, blockID, decay + 8);
            }
        }
    } else if (decay >= 0 && (decay == 0 || blockBlocksFlow(world, x, y - 1, z))) {
        std::vector<bool> directions = getOptimalFlowDirections(world, x, y, z);
        int nextDecay = decay + fluidType;
        if (decay >= 8) nextDecay = 1;
        if (nextDecay >= 8) return;

        if (directions[0]) flowIntoBlock(world, x - 1, y, z, nextDecay);
        if (directions[1]) flowIntoBlock(world, x + 1, y, z, nextDecay);
        if (directions[2]) flowIntoBlock(world, x, y, z - 1, nextDecay);
        if (directions[3]) flowIntoBlock(world, x, y, z + 1, nextDecay);
    }
}

void BlockFlowing::flowIntoBlock(World& world, int x, int y, int z, int meta) const {
    if (liquidCanDisplaceBlock(world, x, y, z)) {
        int id = world.getBlockID(x, y, z);
        if (id > 0) {
            if (blockMaterial == Material::lava && world.getBlockMaterial(x, y, z) == Material::water) {
                world.setBlockWithNotify(x, y, z, Block::cobblestone->blockID);
                triggerLavaMixEffects(world, x, y, z);
                return;
            }
            if (blockMaterial == Material::lava) {
                triggerLavaMixEffects(world, x, y, z);
            }
        }
        if (id != blockID || world.getBlockMetadata(x, y, z) != meta) {
            world.setBlockAndMetadataWithNotify(x, y, z, blockID, meta);
        }
    }
}

int BlockFlowing::calculateFlowCost(World& world, int x, int y, int z, int distance, int side) const {
    int cost = 1000;
    for (int i = 0; i < 4; ++i) {
        if ((i == 0 && side == 1) || (i == 1 && side == 0) || (i == 2 && side == 3) || (i == 3 && side == 2)) continue;

        int nx = x;
        int nz = z;
        if (i == 0) nx--;
        if (i == 1) nx++;
        if (i == 2) nz--;
        if (i == 3) nz++;

        if (!blockBlocksFlow(world, nx, y, nz) && (world.getBlockMaterial(nx, y, nz) != blockMaterial || world.getBlockMetadata(nx, y, nz) != 0)) {
            if (!blockBlocksFlow(world, nx, y - 1, nz)) return distance;
            int maxDistance = (blockMaterial == Material::lava ? 3 : 5);
            if (distance < maxDistance) {
                int nCost = calculateFlowCost(world, nx, y, nz, distance + 1, i);
                if (nCost < cost) cost = nCost;
            }
        }
    }
    return cost;
}

std::vector<bool> BlockFlowing::getOptimalFlowDirections(World& world, int x, int y, int z) const {
    std::vector<int> flowCost(4, 1000);
    for (int i = 0; i < 4; ++i) {
        int nx = x;
        int nz = z;
        if (i == 0) nx--;
        if (i == 1) nx++;
        if (i == 2) nz--;
        if (i == 3) nz++;

        if (!blockBlocksFlow(world, nx, y, nz) && (world.getBlockMaterial(nx, y, nz) != blockMaterial || world.getBlockMetadata(nx, y, nz) != 0)) {
            if (!blockBlocksFlow(world, nx, y - 1, nz)) {
                flowCost[i] = 0;
            } else {
                flowCost[i] = calculateFlowCost(world, nx, y, nz, 1, i);
            }
        }
    }

    int minCost = flowCost[0];
    for (int i = 1; i < 4; ++i) if (flowCost[i] < minCost) minCost = flowCost[i];

    std::vector<bool> result(4);
    if (minCost < 1000) {
        for (int i = 0; i < 4; ++i) result[i] = (flowCost[i] == minCost);
    } else {
        for (int i = 0; i < 4; ++i) {
            int nx = x;
            int nz = z;
            if (i == 0) nx--;
            if (i == 1) nx++;
            if (i == 2) nz--;
            if (i == 3) nz++;
            result[i] = (!blockBlocksFlow(world, nx, y, nz) && (world.getBlockMaterial(nx, y, nz) != blockMaterial || world.getBlockMetadata(nx, y, nz) != 0));
        }
    }
    return result;
}

bool BlockFlowing::blockBlocksFlow(World& world, int x, int y, int z) const {
    int id = world.getBlockID(x, y, z);
    if (id == 64 || id == 63 || id == 65 || id == 68) return true;
    if (id == 0) return false;
    return world.getBlockMaterial(x, y, z).isSolid();
}

int BlockFlowing::getSmallestFlowDecay(World& world, int x, int y, int z, int currentSmallest, int& numAdjacentSources) const {
    int decay = getFlowDecay(world, x, y, z);
    if (decay < 0) return currentSmallest;
    if (decay == 0) numAdjacentSources++;

    int val = decay;
    if (val >= 8) val = 0;

    return (currentSmallest >= 0 && val >= currentSmallest) ? currentSmallest : val;
}

bool BlockFlowing::liquidCanDisplaceBlock(World& world, int x, int y, int z) const {
    const Material& mat = world.getBlockMaterial(x, y, z);
    if (mat == blockMaterial) return false;
    if (mat == Material::lava) return false;
    return !blockBlocksFlow(world, x, y, z);
}
