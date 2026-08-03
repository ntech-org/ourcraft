#include "entities/EntityTNTPrimed.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include <cmath>

EntityTNTPrimed::EntityTNTPrimed(World& world)
    : Entity(world) {
    setSize(0.98f, 0.98f);
    yOffset = height / 2.0f;
    fuse = 0;
}

EntityTNTPrimed::EntityTNTPrimed(World& world, float x, float y, float z)
    : Entity(world) {
    setSize(0.98f, 0.98f);
    yOffset = height / 2.0f;
    setPosition((double)x, (double)y, (double)z);
    motionY = 0.2;
    motionX = 0.0;
    motionZ = 0.0;
    prevPosX = (double)x;
    prevPosY = (double)y;
    prevPosZ = (double)z;
    fuse = 80;
}

void EntityTNTPrimed::onUpdate() {
    Entity::onUpdate();

    motionY -= 0.04;
    moveEntity(motionX, motionY, motionZ);
    motionX *= 0.98;
    motionY *= 0.98;
    motionZ *= 0.98;

    if (onGround) {
        motionX *= 0.7;
        motionZ *= 0.7;
        motionY *= -0.5;
    }

    if (fuse-- <= 0) {
        isDead = true;
        int ix = (int)std::floor(posX);
        int iy = (int)std::floor(posY);
        int iz = (int)std::floor(posZ);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (worldObj.getBlockID(ix + dx, iy + dy, iz + dz) != 0) {
                        worldObj.setBlockWithNotify(ix + dx, iy + dy, iz + dz, 0);
                    }
                }
            }
        }
    }
}
