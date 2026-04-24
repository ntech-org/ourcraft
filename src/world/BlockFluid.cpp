#include "world/BlockFluid.hpp"
#include "world/World.hpp"
#include "world/JavaRandom.hpp"
#include "physics/AxisAlignedBB.hpp"
#include <glm/glm.hpp>
#include <cmath>

BlockFluid::BlockFluid(int id, const Material& mat) : Block(id, (id == 10 || id == 11 ? 14 : 12) * 16 + 13, mat) {
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockFluid::onBlockAdded(World& world, int x, int y, int z) const {
    checkForHarden(world, x, y, z);
}

void BlockFluid::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    checkForHarden(world, x, y, z);
}

void BlockFluid::checkForHarden(World& world, int x, int y, int z) const {
    if (world.getBlockID(x, y, z) != blockID) return;
    if (blockMaterial == Material::lava) {
        bool hasWater = false;
        if (world.getBlockMaterial(x, y, z - 1) == Material::water) hasWater = true;
        if (world.getBlockMaterial(x, y, z + 1) == Material::water) hasWater = true;
        if (world.getBlockMaterial(x - 1, y, z) == Material::water) hasWater = true;
        if (world.getBlockMaterial(x + 1, y, z) == Material::water) hasWater = true;
        if (world.getBlockMaterial(x, y + 1, z) == Material::water) hasWater = true;

        if (hasWater) {
            int meta = world.getBlockMetadata(x, y, z);
            if (meta == 0) {
                world.setBlockWithNotify(x, y, z, Block::obsidian->blockID);
            } else if (meta <= 4) {
                world.setBlockWithNotify(x, y, z, Block::cobblestone->blockID);
            }
            triggerLavaMixEffects(world, x, y, z);
        }
    }
}

void BlockFluid::triggerLavaMixEffects(World& world, int x, int y, int z) const {
    // Missing: sound and particles
}

int BlockFluid::getFlowDecay(World& world, int x, int y, int z) const {
    return world.getBlockMaterial(x, y, z) != blockMaterial ? -1 : world.getBlockMetadata(x, y, z);
}

int BlockFluid::getEffectiveFlowDecay(const IBlockAccess& world, int x, int y, int z) const {
    if (world.getBlockMaterial(x, y, z) != blockMaterial) return -1;
    int meta = world.getBlockMetadata(x, y, z);
    if (meta >= 8) meta = 0;
    return meta;
}

int BlockFluid::getTexture(int side) const {
    if (side == 0 || side == 1) return blockIndexInTexture;
    return blockIndexInTexture + 1;
}

void BlockFluid::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    // Overridden in BlockFlowing
}

int BlockFluid::tickRate() const {
    if (blockMaterial == Material::water) return 5;
    if (blockMaterial == Material::lava) return 30;
    return 0;
}

BlockRenderLayer BlockFluid::getRenderLayer() const {
    return blockMaterial == Material::water ? BlockRenderLayer::Translucent : BlockRenderLayer::Opaque;
}

bool BlockFluid::shouldSideBeRendered(const IBlockAccess& world, int x, int y, int z, int side) const {
    uint8_t bid = world.getBlockID(x, y, z);
    if (bid == 0) return true;
    if (world.getBlockMaterial(x, y, z) == blockMaterial) return false;
    if (side == 1) return true;
    if (Block::blocksList[bid]) {
        return !Block::blocksList[bid]->isOccluder();
    }
    return true;
}

void BlockFluid::velocityToAddToEntity(World& world, int x, int y, int z, Entity* entity, glm::vec3& velocity) const {
    glm::vec3 flow = getFlowVector(world, x, y, z);
    velocity.x += flow.x;
    velocity.y += flow.y;
    velocity.z += flow.z;
}

float BlockFluid::getPercentAir(int meta) {
    if (meta >= 8) meta = 0;
    return (float)(meta + 1) / 9.0f;
}

glm::vec3 BlockFluid::getFlowVector(const IBlockAccess& world, int x, int y, int z) const {
    glm::vec3 vector(0.0f);
    int decay = getEffectiveFlowDecay(world, x, y, z);
    
    for (int i = 0; i < 4; ++i) {
        int nx = x;
        int nz = z;
        if (i == 0) nx--;
        if (i == 1) nz--;
        if (i == 2) nx++;
        if (i == 3) nz++;
        
        int nDecay = getEffectiveFlowDecay(world, nx, y, nz);
        if (nDecay < 0) {
            if (!world.getBlockMaterial(nx, y, nz).isSolid()) {
                nDecay = getEffectiveFlowDecay(world, nx, y - 1, nz);
                if (nDecay >= 0) {
                    int diff = nDecay - (decay - 8);
                    vector.x += (nx - x) * diff;
                    vector.z += (nz - z) * diff;
                }
            }
        } else if (nDecay >= 0) {
            int diff = nDecay - decay;
            vector.x += (nx - x) * diff;
            vector.z += (nz - z) * diff;
        }
    }
    
    if (world.getBlockMetadata(x, y, z) >= 8) {
        bool hasFreeSide = false;
        if (shouldSideBeRendered(world, x, y, z - 1, 2)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x, y, z + 1, 3)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x - 1, y, z, 4)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x + 1, y, z, 5)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x, y + 1, z - 1, 2)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x, y + 1, z + 1, 3)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x - 1, y + 1, z, 4)) hasFreeSide = true;
        if (shouldSideBeRendered(world, x + 1, y + 1, z, 5)) hasFreeSide = true;
        
        if (hasFreeSide) {
            vector = glm::normalize(vector) + glm::vec3(0, -6, 0);
        }
    }
    
    return glm::normalize(vector);
}

double BlockFluid::getFlowDirection(const IBlockAccess& world, int x, int y, int z, const Material& mat) {
    glm::vec3 vector(0.0f);
    if (mat == Material::water) {
        vector = ((BlockFluid*)Block::waterMoving)->getFlowVector(world, x, y, z);
    } else if (mat == Material::lava) {
        vector = ((BlockFluid*)Block::lavaMoving)->getFlowVector(world, x, y, z);
    }
    
    if (vector.x == 0.0f && vector.z == 0.0f) return -1000.0;
    return std::atan2(vector.z, vector.x) - (3.14159265 / 2.0);
}

// BlockFlowing implementation

BlockFlowing::BlockFlowing(int id, const Material& mat) : BlockFluid(id, mat) {}

void BlockFlowing::onBlockAdded(World& world, int x, int y, int z) const {
    BlockFluid::onBlockAdded(world, x, y, z);
    if (world.getBlockID(x, y, z) == blockID) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

void BlockFlowing::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    int decay = getFlowDecay(world, x, y, z);
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
        
        if (getFlowDecay(world, x, y + 1, z) >= 0) {
            int topDecay = getFlowDecay(world, x, y + 1, z);
            if (topDecay >= 8) newDecay = topDecay;
            else newDecay = topDecay + 8;
        }
        
        if (numAdjacentSources >= 2 && blockMaterial == Material::water) {
            newDecay = 0;
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
                world.notifyBlocksOfNeighborChange(x, y, z, blockID);
            }
        } else if (flag) {
            // Stationary change logic
            world.setBlockAndMetadataWithNotify(x, y, z, blockID + 1, decay);
        }
    } else {
        world.setBlockAndMetadataWithNotify(x, y, z, blockID + 1, decay);
    }
    
    if (liquidCanDisplaceBlock(world, x, y - 1, z)) {
        if (decay >= 8) {
            world.setBlockAndMetadataWithNotify(x, y - 1, z, blockID, decay);
        } else {
            world.setBlockAndMetadataWithNotify(x, y - 1, z, blockID, decay + 8);
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
            if (blockMaterial == Material::lava) {
                triggerLavaMixEffects(world, x, y, z);
            } else {
                // Block::blocksList[id]->dropBlockAsItem(...)
            }
        }
        world.setBlockAndMetadataWithNotify(x, y, z, blockID, meta);
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
            if (distance < 4) {
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
    for (int i = 0; i < 4; ++i) result[i] = (flowCost[i] == minCost && minCost < 1000);
    return result;
}

bool BlockFlowing::blockBlocksFlow(World& world, int x, int y, int z) const {
    int id = world.getBlockID(x, y, z);
    if (id == Block::doorWood->blockID || id == Block::signStanding->blockID || id == Block::ladder->blockID) return true;
    if (id == 0) return false;
    return Block::blocksList[id]->blockMaterial.isSolid();
}

int BlockFlowing::getSmallestFlowDecay(World& world, int x, int y, int z, int currentSmallest, int& numAdjacentSources) const {
    int decay = getFlowDecay(world, x, y, z);
    if (decay < 0) return currentSmallest;
    if (decay == 0) numAdjacentSources++;
    if (decay >= 8) decay = 0;
    return (currentSmallest >= 0 && decay >= currentSmallest) ? currentSmallest : decay;
}

bool BlockFlowing::liquidCanDisplaceBlock(World& world, int x, int y, int z) const {
    Material mat = world.getBlockMaterial(x, y, z);
    if (mat == blockMaterial) return false;
    if (mat == Material::lava) return false;
    return !blockBlocksFlow(world, x, y, z);
}

// BlockStationary implementation

BlockStationary::BlockStationary(int id, const Material& mat) : BlockFluid(id, mat) {}

void BlockStationary::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    BlockFluid::onNeighborBlockChange(world, x, y, z, neighborID);
    if (world.getBlockID(x, y, z) == blockID) {
        int meta = world.getBlockMetadata(x, y, z);
        world.setBlockAndMetadataWithNotify(x, y, z, blockID - 1, meta);
        world.scheduleBlockUpdate(x, y, z, blockID - 1, tickRate());
    }
}
