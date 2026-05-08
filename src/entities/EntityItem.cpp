#include "entities/EntityItem.hpp"

#include <algorithm>
#include <cmath>

EntityItem::EntityItem(World& world, int itemIDIn, int countIn, uint8_t metadataIn)
    : Entity(world), itemID(itemIDIn), count(countIn), metadata(metadataIn) {
    setSize(0.25f, 0.25f);
    yOffset = 0.125f;
}

void EntityItem::onUpdate() {
    Entity::onUpdate();

    if (pickupDelay > 0) {
        --pickupDelay;
    }
    ++age;

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
}
