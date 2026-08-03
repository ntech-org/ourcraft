#include "entities/EntityZombie.hpp"
#include "items/Item.hpp"
#include <cstdlib>

EntityZombie::EntityZombie(World& world) : EntityLiving(world) {
    setSize(0.6f, 1.8f);
    health = 20;
    maxHealth = 20;
}

void EntityZombie::updateEntityActionState() {
    // Make them bounce in water
    if (inWater || inLava) {
        jumping = true;
    } else {
        jumping = (float)std::rand() / (float)RAND_MAX < 0.01f;
    }

    // Very simple random movement for now
    if ((float)std::rand() / (float)RAND_MAX < 0.05f) {
        rotationYaw = (float)std::rand() / (float)RAND_MAX * 360.0f;
    }

    moveForward = 0.5f;
    moveStrafe = 0.0f;
    moveRelative(moveStrafe, moveForward, onGround ? 0.1f : 0.02f);
}

int EntityZombie::getDropItemID() const {
    return Item::feather ? Item::feather->itemID : 0;
}
