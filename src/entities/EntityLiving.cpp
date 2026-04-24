#include "entities/EntityLiving.hpp"
#include <cmath>
#include <algorithm>

EntityLiving::EntityLiving(World& world) : Entity(world) {
    limbSwing = 0.0f;
    prevLimbSwing = 0.0f;
    limbSwingAmount = 0.0f;
    prevLimbSwingAmount = 0.0f;
    swingProgress = 0.0f;
    prevSwingProgress = 0.0f;
    isSwinging = false;
    swingProgressInt = 0;
}

void EntityLiving::onUpdate() {
    double dx = posX - prevPosX;
    double dz = posZ - prevPosZ;
    float dist = (float)std::sqrt(dx * dx + dz * dz);

    Entity::onUpdate();

    prevRenderYawOffset = renderYawOffset;

    // Body-follow-head logic from Infdev
    float yawDiff = rotationYaw - renderYawOffset;
    while (yawDiff < -180.0f) yawDiff += 360.0f;
    while (yawDiff >= 180.0f) yawDiff -= 360.0f;

    // Clamp head yaw relative to body to [-75, 75] degrees
    bool isReversed = yawDiff < -90.0f || yawDiff >= 90.0f;
    if (yawDiff < -75.0f) yawDiff = -75.0f;
    if (yawDiff >= 75.0f) yawDiff = 75.0f;

    renderYawOffset = rotationYaw - yawDiff;
    renderYawOffset += yawDiff * 0.1f; // Body slowly turns to follow head

    updateEntityActionState();

    prevSwingProgress = swingProgress;
    if (isSwinging) {
        swingProgressInt++;
        if (swingProgressInt >= 8) {
            swingProgressInt = 0;
            isSwinging = false;
        }
    }
    swingProgress = (float)swingProgressInt / 8.0f;

    if (handlePhysics) {
        moveEntity(motionX, motionY, motionZ);
        
        motionX *= 0.91;
        motionY *= 0.98;
        motionZ *= 0.91;
        motionY -= 0.08; // gravity

        if (onGround) {
            motionX *= 0.6;
            motionZ *= 0.6;
        }
    }

    prevLimbSwing = limbSwing;
    prevLimbSwingAmount = limbSwingAmount;
    
    float f = dist * 4.0f;
    if (f > 1.0f) f = 1.0f;
    limbSwingAmount += (f - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;
}

void EntityLiving::swing() {
    if (!isSwinging || swingProgressInt >= 4 || swingProgressInt < 0) {
        swingProgressInt = -1;
        isSwinging = true;
    }
}

void EntityLiving::updateEntityActionState() {
    // Default AI or behavior
}
