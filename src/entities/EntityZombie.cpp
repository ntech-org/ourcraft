#include "entities/EntityZombie.hpp"
#include <cstdlib>

EntityZombie::EntityZombie(World& world) : EntityLiving(world) {
    setSize(0.6f, 1.8f);
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

    float forward = 0.5f;
    float strafe = 0.0f;

    // moveRelative logic would go here if we want them to actually move
}
