#include "entities/EntitySkeleton.hpp"
#include "items/Item.hpp"
#include <cstdlib>
#include <cmath>

EntitySkeleton::EntitySkeleton(World& world) : EntityLiving(world) {
    setSize(0.6f, 1.8f);
    health = 20;
    maxHealth = 20;
}

void EntitySkeleton::updateEntityActionState() {
    if (inWater || inLava) {
        jumping = true;
    } else {
        jumping = (float)std::rand() / (float)RAND_MAX < 0.01f;
    }

    if ((float)std::rand() / (float)RAND_MAX < 0.05f) {
        rotationYaw = (float)std::rand() / (float)RAND_MAX * 360.0f;
    }

    moveForward = 0.4f;
    moveStrafe = 0.0f;
    moveRelative(moveStrafe, moveForward, onGround ? 0.1f : 0.02f);

    if (attackTime > 0) attackTime--;
}

int EntitySkeleton::getDropItemID() const {
    return Item::arrow ? Item::arrow->itemID : 0;
}
