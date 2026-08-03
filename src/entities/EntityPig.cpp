#include "entities/EntityPig.hpp"
#include "items/Item.hpp"
#include <cstdlib>
#include <cmath>

EntityPig::EntityPig(World& world) : EntityLiving(world) {
    setSize(0.9f, 0.9f);
    health = 10;
    maxHealth = 10;
}

void EntityPig::updateEntityActionState() {
    if (inWater || inLava) {
        jumping = true;
    } else {
        jumping = (float)std::rand() / (float)RAND_MAX < 0.01f;
    }

    if ((float)std::rand() / (float)RAND_MAX < 0.05f) {
        rotationYaw = (float)std::rand() / (float)RAND_MAX * 360.0f;
    }

    moveForward = 0.3f;
    moveStrafe = 0.0f;
    moveRelative(moveStrafe, moveForward, onGround ? 0.1f : 0.02f);
}

int EntityPig::getDropItemID() const {
    return Item::porkRaw ? Item::porkRaw->itemID : 0;
}
