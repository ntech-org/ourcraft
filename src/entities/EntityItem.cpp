#include "entities/EntityItem.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>

EntityItem::EntityItem(World& world, int itemIDIn, int countIn, uint8_t metadataIn)
    : Entity(world), itemID(itemIDIn), count(countIn), metadata(metadataIn) {
    setSize(0.25f, 0.25f);
    yOffset = height / 2.0f;
    hoverStart = (float)(std::rand() / (double)RAND_MAX * 3.14159265358979323846 * 2.0);
    rotationYaw = (float)(std::rand() / (double)RAND_MAX * 360.0);
    motionX = (float)(std::rand() / (double)RAND_MAX * 0.2 - 0.1);
    motionY = 0.2;
    motionZ = (float)(std::rand() / (double)RAND_MAX * 0.2 - 0.1);
}

EntityItem::EntityItem(World& world)
    : Entity(world) {
    setSize(0.25f, 0.25f);
    yOffset = height / 2.0f;
}

void EntityItem::startPickupAnimation(double targetX, double targetY, double targetZ) {
    if (pickingUp) return;
    pickingUp = true;
    pickupStartX = posX;
    pickupStartY = posY;
    pickupStartZ = posZ;
    pickupTargetX = targetX;
    pickupTargetY = targetY;
    pickupTargetZ = targetZ;
    pickupAnimationTotalTicks = 6;
    pickupAnimationTicks = pickupAnimationTotalTicks;
    handlePhysics = false;
    motionX = 0.0;
    motionY = 0.0;
    motionZ = 0.0;
}

void EntityItem::pushOutOfBlocks() {
    int bx = (int)std::floor(posX);
    int by = (int)std::floor(posY);
    int bz = (int)std::floor(posZ);

    double dx = posX - bx;
    double dy = posY - by;
    double dz = posZ - bz;

    if (!Block::opaqueCubeLookup[worldObj.getBlockID(bx, by, bz)]) return;

    bool canNegX = !Block::opaqueCubeLookup[worldObj.getBlockID(bx - 1, by, bz)];
    bool canPosX = !Block::opaqueCubeLookup[worldObj.getBlockID(bx + 1, by, bz)];
    bool canNegY = !Block::opaqueCubeLookup[worldObj.getBlockID(bx, by - 1, bz)];
    bool canPosY = !Block::opaqueCubeLookup[worldObj.getBlockID(bx, by + 1, bz)];
    bool canNegZ = !Block::opaqueCubeLookup[worldObj.getBlockID(bx, by, bz - 1)];
    bool canPosZ = !Block::opaqueCubeLookup[worldObj.getBlockID(bx, by, bz + 1)];

    int bestDir = -1;
    double bestDist = 9999.0;

    if (canNegX && dx < bestDist)       { bestDist = dx;    bestDir = 0; }
    if (canPosX && 1.0 - dx < bestDist) { bestDist = 1.0-dx; bestDir = 1; }
    if (canNegY && dy < bestDist)       { bestDist = dy;    bestDir = 2; }
    if (canPosY && 1.0 - dy < bestDist) { bestDist = 1.0-dy; bestDir = 3; }
    if (canNegZ && dz < bestDist)       { bestDist = dz;    bestDir = 4; }
    if (canPosZ && 1.0 - dz < bestDist) { bestDist = 1.0-dz; bestDir = 5; }

    float speed = (float)std::rand() / (float)RAND_MAX * 0.2f + 0.1f;
    if (bestDir == 0) motionX = -speed;
    if (bestDir == 1) motionX =  speed;
    if (bestDir == 2) motionY = -speed;
    if (bestDir == 3) motionY =  speed;
    if (bestDir == 4) motionZ = -speed;
    if (bestDir == 5) motionZ =  speed;
}

void EntityItem::onUpdate() {
    Entity::onUpdate();

    if (delayBeforeCanPickup > 0) {
        --delayBeforeCanPickup;
    }

    ++m_ageTicks;
    ++age;

    if (pickingUp) {
        float progress = 1.0f - ((float)pickupAnimationTicks / (float)pickupAnimationTotalTicks);
        progress = std::clamp(progress, 0.0f, 1.0f);
        double pull = progress * progress * (3.0 - 2.0 * progress);
        double lift = std::sin(progress * 3.14159265358979323846) * 0.12;
        setPosition(
            pickupStartX + (pickupTargetX - pickupStartX) * pull,
            pickupStartY + (pickupTargetY - pickupStartY) * pull + lift,
            pickupStartZ + (pickupTargetZ - pickupStartZ) * pull
        );
        if (--pickupAnimationTicks <= 0) {
            worldObj.removeEntity(entityID, false);
        }
        return;
    }

    if (!handlePhysics) return;

    motionY -= 0.04;

    if (worldObj.getBlockMaterial((int)std::floor(posX), (int)std::floor(posY), (int)std::floor(posZ)) == Material::lava) {
        motionY = 0.2;
        motionX = ((float)std::rand() / (float)RAND_MAX - (float)std::rand() / (float)RAND_MAX) * 0.2;
        motionZ = ((float)std::rand() / (float)RAND_MAX - (float)std::rand() / (float)RAND_MAX) * 0.2;
    }

    pushOutOfBlocks();
    handleWaterMovement();
    moveEntity(motionX, motionY, motionZ);
    motionX *= 0.98;
    motionY *= 0.98;
    motionZ *= 0.98;

    if (onGround) {
        motionX *= 0.7;
        motionZ *= 0.7;
        motionY *= -0.5;
    }

    if (age >= 6000) {
        worldObj.removeEntity(entityID, false);
    }
}

void EntityItem::onCollideWithPlayer(EntityPlayer& player) {
    if (delayBeforeCanPickup > 0) return;
    if (player.inventory.addItem(itemID, count, metadata)) {
        worldObj.removeEntity(entityID, false);
    }
}

bool EntityItem::tryMergeWithNearby() {
    for (auto& entity : worldObj.getEntities()) {
        if (entity.get() == this) continue;
        if (entity->entityID == entityID) continue;
        if (entity->getType() != EntityType::Item) continue;

        auto* other = static_cast<EntityItem*>(entity.get());
        if (other->pickingUp) continue;
        if (other->itemID != itemID) continue;
        if (other->metadata != metadata) continue;

        double dx = posX - other->posX;
        double dy = posY - other->posY;
        double dz = posZ - other->posZ;
        if (dx * dx + dy * dy + dz * dz > MERGE_RADIUS * MERGE_RADIUS) continue;

        int total = other->count + count;
        if (total <= MAX_MERGE_COUNT) {
            other->count = total;
            return true;
        } else {
            other->count = MAX_MERGE_COUNT;
            count = total - MAX_MERGE_COUNT;
            return false;
        }
    }
    return false;
}
