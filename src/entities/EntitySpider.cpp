#include "entities/EntitySpider.hpp"
#include "items/Item.hpp"
#include <cstdlib>
#include <cmath>

EntitySpider::EntitySpider(World& world) : EntityLiving(world) {
    setSize(1.4f, 0.9f);
    health = 20;
    maxHealth = 20;
}

void EntitySpider::updateEntityActionState() {
    if (inWater || inLava) {
        jumping = true;
    } else {
        jumping = (float)std::rand() / (float)RAND_MAX < 0.02f;
    }

    if ((float)std::rand() / (float)RAND_MAX < 0.05f) {
        rotationYaw = (float)std::rand() / (float)RAND_MAX * 360.0f;
    }

    moveForward = 0.6f;
    moveStrafe = 0.0f;
    moveRelative(moveStrafe, moveForward, onGround ? 0.1f : 0.02f);
}

int EntitySpider::getDropItemID() const {
    return Item::silk ? Item::silk->itemID : 0;
}
