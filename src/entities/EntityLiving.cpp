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

    if (hurtTime > 0) hurtTime--;

    handleWaterMovement();

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
        handleWaterMovement();
        
        if (inWater) {
            // Infdev Water Physics
            float waterDrag = 0.8f;
            float acceleration = 0.02f;
            
            // Buoyancy is added before drag in the original logic
            if (jumping) {
                motionY += 0.04f;
            }
            
            moveEntity(motionX, motionY, motionZ);
            
            motionX *= waterDrag;
            motionY *= waterDrag;
            motionZ *= waterDrag;
            
            if (isFlying) {
                // Sinking suppressed
            } else {
                motionY -= 0.02f; // Sinking force
            }


            if (isCollidedHorizontally && isOffsetPositionInLiquid(motionX, motionY + 0.6000000238418579 - posY + prevPosY, motionZ)) {
                motionY = 0.30000001192092896;
            }
        } else {
            // Infdev Ground/Air Physics
            moveEntity(motionX, motionY, motionZ);
            
            float drag = 0.91f;
            if (onGround) {
                // Ground drag depends on the block below, 0.6 is default (sand/dirt/stone)
                drag = 0.6f * 0.91f;
            }
            
            motionX *= drag;
            motionY *= 0.98f;
            motionZ *= drag;
            
            if (isFlying) {
                // Gravity suppressed
            } else {
                motionY -= 0.08f; // Gravity
            }
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

void EntityLiving::attackEntityFrom(Entity* source, int amount) {
    if (amount > 0) {
        health -= amount;
        hurtTime = 20;
    }
}

void EntityLiving::fall(float distance) {
    int damage = (int)std::ceil(distance - 3.0f);
    if (damage > 0) {
        attackEntityFrom(nullptr, damage);
    }
}

