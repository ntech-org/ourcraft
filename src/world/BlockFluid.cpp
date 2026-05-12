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
            } else if (meta > 0) {
                world.setBlockWithNotify(x, y, z, Block::cobblestone->blockID);
            }
            triggerLavaMixEffects(world, x, y, z);
        }
    }
}

void BlockFluid::triggerLavaMixEffects(World& world, int x, int y, int z) const {
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
            if (vector.x != 0.0f || vector.z != 0.0f) {
                vector = glm::normalize(vector) + glm::vec3(0, -6, 0);
            } else {
                vector = glm::vec3(0, -6, 0);
            }
        }
    }

    if (vector.x != 0.0f || vector.z != 0.0f || vector.y != 0.0f) {
        return glm::normalize(vector);
    }
    return vector;
}

double BlockFluid::getFlowDirection(const IBlockAccess& world, int x, int y, int z, const Material& mat) {
    glm::vec3 vector(0.0f);
    if (mat == Material::water) {
        vector = ((BlockFluid*)Block::waterMoving)->getFlowVector(world, x, y, z);
    } else if (mat == Material::lava) {
        vector = ((BlockFluid*)Block::lavaMoving)->getFlowVector(world, x, y, z);
    }

    if (vector.x == 0.0f && vector.z == 0.0f) return -1000.0;
    return std::atan2(vector.x, vector.z);
}
