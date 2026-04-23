#include "entities/EntityLiving.hpp"
#include <cmath>
#include <algorithm>

EntityLiving::EntityLiving(World& world) : Entity(world) {
    limbSwing = 0.0f;
    limbSwingAmount = 0.0f;
    prevLimbSwingAmount = 0.0f;
}

void EntityLiving::onUpdate() {
    Entity::onUpdate();
    
    updateEntityActionState();

    motionY -= 0.08; // gravity
    moveEntity(motionX, motionY, motionZ);
    
    motionX *= 0.91;
    motionY *= 0.98;
    motionZ *= 0.91;

    if (onGround) {
        motionX *= 0.6;
        motionZ *= 0.6;
    }

    prevLimbSwingAmount = limbSwingAmount;
    double dx = posX - prevPosX;
    double dz = posZ - prevPosZ;
    float dist = (float)std::sqrt(dx * dx + dz * dz);
    
    float f = dist * 4.0f;
    if (f > 1.0f) f = 1.0f;
    limbSwingAmount += (f - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;
}

void EntityLiving::updateEntityActionState() {
    // Default AI or behavior
}
