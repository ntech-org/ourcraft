#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/BlockFluid.hpp"
#include "entities/Entity.hpp"
#include <cmath>
#include <glm/geometric.hpp>

std::vector<AxisAlignedBB> World::getCollidingBoundingBoxes(const AxisAlignedBB& bb) {
    std::vector<AxisAlignedBB> list;
    int x0 = (int)std::floor(bb.minX), x1 = (int)std::floor(bb.maxX + 1.0);
    int y0 = (int)std::floor(bb.minY), y1 = (int)std::floor(bb.maxY + 1.0);
    int z0 = (int)std::floor(bb.minZ), z1 = (int)std::floor(bb.maxZ + 1.0);

    for (int x = x0; x < x1; ++x) {
        for (int y = y0; y < y1; ++y) {
            for (int z = z0; z < z1; ++z) {
                uint8_t bid = getBlockID(x, y, z);
                if (bid > 0 && Block::blocksList[bid]) {
                    Block::blocksList[bid]->getCollisionBoxes(*this, x, y, z, bb, list);
                }
            }
        }
    }
    return list;
}

bool World::handleMaterialAcceleration(const AxisAlignedBB& bb, const Material& mat, Entity* entity) {
    int x0 = (int)std::floor(bb.minX), x1 = (int)std::floor(bb.maxX + 1.0);
    int y0 = (int)std::floor(bb.minY), y1 = (int)std::floor(bb.maxY + 1.0);
    int z0 = (int)std::floor(bb.minZ), z1 = (int)std::floor(bb.maxZ + 1.0);
    
    bool collided = false;
    glm::vec3 velocity(0.0f);
    for (int x = x0; x < x1; ++x) {
        for (int y = y0; y < y1; ++y) {
            for (int z = z0; z < z1; ++z) {
                uint8_t bid = getBlockID(x, y, z);
                if (bid > 0 && Block::blocksList[bid]->blockMaterial == mat) {
                    double fluidY = (double)(y + 1) - (double)BlockFluid::getPercentAir(getBlockMetadata(x, y, z));
                    if (fluidY >= bb.minY) {
                        collided = true;
                        Block::blocksList[bid]->velocityToAddToEntity(*this, x, y, z, entity, velocity);
                    }
                }
            }
        }
    }
    
    if (glm::length(velocity) > 0.0f) {
        velocity = glm::normalize(velocity);
        double force = 0.014;
        entity->motionX += velocity.x * force;
        entity->motionY += velocity.y * force;
        entity->motionZ += velocity.z * force;
    }
    return collided;
}

bool World::getIsAnyLiquid(const AxisAlignedBB& bb) {
    int x0 = (int)std::floor(bb.minX), x1 = (int)std::floor(bb.maxX + 1.0);
    int y0 = (int)std::floor(bb.minY), y1 = (int)std::floor(bb.maxY + 1.0);
    int z0 = (int)std::floor(bb.minZ), z1 = (int)std::floor(bb.maxZ + 1.0);

    for (int x = x0; x < x1; ++x) {
        for (int y = y0; y < y1; ++y) {
            for (int z = z0; z < z1; ++z) {
                uint8_t bid = getBlockID(x, y, z);
                if (bid > 0 && Block::blocksList[bid] && Block::blocksList[bid]->blockMaterial.isLiquid()) return true;
            }
        }
    }
    return false;
}
