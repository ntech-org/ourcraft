#include "entities/EntityItem.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>

EntityItem::EntityItem(World& world, int itemIDIn, int countIn, uint8_t metadataIn)
    : Entity(world), itemID(itemIDIn), count(countIn), metadata(metadataIn) {
    setSize(0.25f, 0.25f);
    yOffset = 0.125f;
    hoverStart = (float)std::rand() / (float)RAND_MAX * 6.28318530718f;
    motionX = ((float)std::rand() / (float)RAND_MAX * 0.2f - 0.1f);
    motionY = 0.2;
    motionZ = ((float)std::rand() / (float)RAND_MAX * 0.2f - 0.1f);
}

void EntityItem::startPickupAnimation(double targetX, double targetY, double targetZ) {
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

void EntityItem::onUpdate() {
    Entity::onUpdate();

    if (pickupDelay > 0) {
        --pickupDelay;
    }
    ++age;

    if (pickupAnimationTicks > 0) {
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

    if (!handlePhysics) {
        return;
    }

    motionY -= 0.04;
    moveEntity(motionX, motionY, motionZ);

    motionX *= 0.98;
    motionY *= 0.98;
    motionZ *= 0.98;

    if (onGround) {
        motionX *= 0.7;
        motionZ *= 0.7;
        motionY = 0.0;
    }
}
