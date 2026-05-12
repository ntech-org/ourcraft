#include "entities/EntityLiving.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/BlockFluid.hpp"
#include <cmath>
#include <algorithm>
#include <glm/ext/scalar_constants.hpp>

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


void EntityLiving::moveRelative(float strafe, float forward, float friction) {
    float dist = strafe * strafe + forward * forward;
    if (dist < 1.0E-4F) return;

    dist = std::sqrt(dist);
    if (dist < 1.0f) dist = 1.0f;
    dist = friction / dist;
    strafe *= dist;
    forward *= dist;

    float sinYaw = std::sin(rotationYaw * glm::pi<float>() / 180.0f);
    float cosYaw = std::cos(rotationYaw * glm::pi<float>() / 180.0f);

    motionX += (double)(strafe * cosYaw - forward * sinYaw);
    motionZ += (double)(forward * cosYaw + strafe * sinYaw);
}


void EntityLiving::onUpdate() {
    double dx = posX - prevPosX;
    double dz = posZ - prevPosZ;
    float dist = (float)std::sqrt(dx * dx + dz * dz);

    Entity::onUpdate();

    prevRenderYawOffset = renderYawOffset;

    // Body-follow-head and movement rotation logic
    if (dist > 0.05f) {
        float moveYaw = (float)(std::atan2(-dx, dz) * 180.0 / glm::pi<double>());
        float yawDiff = moveYaw - renderYawOffset;
        while (yawDiff < -180.0f) yawDiff += 360.0f;
        while (yawDiff >= 180.0f) yawDiff -= 360.0f;
        renderYawOffset += yawDiff * 0.1f;
    }

    while (renderYawOffset < -180.0f) renderYawOffset += 360.0f;
    while (renderYawOffset >= 180.0f) renderYawOffset -= 360.0f;

    float yawDiff = rotationYaw - renderYawOffset;
    while (yawDiff < -180.0f) yawDiff += 360.0f;
    while (yawDiff >= 180.0f) yawDiff -= 360.0f;

    // Clamp head yaw relative to body to [-75, 75] degrees
    if (yawDiff < -75.0f) yawDiff = -75.0f;
    if (yawDiff >= 75.0f) yawDiff = 75.0f;

    renderYawOffset = rotationYaw - yawDiff;
    if (yawDiff * yawDiff > 2500.0f) {
        renderYawOffset += yawDiff * 0.2f;
    }

    if (hurtTime > 0) hurtTime--;

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

    if (worldObj.isRemote && !isLocalPlayer) {
        prevLimbSwing = limbSwing;
        prevLimbSwingAmount = limbSwingAmount;
        float f = dist * 4.0f;
        if (f > 1.0f) f = 1.0f;
        limbSwingAmount += (f - limbSwingAmount) * 0.4f;
        limbSwing += limbSwingAmount;

        // Skip all movement/physics/liquid logic for remote entities on client
        return;
    }

    handleWaterMovement();
    handleLavaMovement();

    if (inWater && !wasInWater) {
        if (onPlaySound) onPlaySound("liquid.splash", 1.0f, 1.0f);
    }
    wasInWater = inWater;

    if (handlePhysics) {
        bool chunkLoaded = worldObj.isChunkLoaded((int)std::floor(posX / 16.0), (int)std::floor(posZ / 16.0));

        if (!chunkLoaded) {
            motionX = 0;
            motionY = 0;
            motionZ = 0;
        } else {
            if (inWater) {
                // Infdev Water Physics
                float waterDrag = 0.8f;
                int decay = ((BlockFluid*)Block::waterMoving)->getEffectiveFlowDecay(worldObj, (int)std::floor(posX), (int)std::floor(posY), (int)std::floor(posZ));
                float jumpBoost = (decay < 0 ? 0.0f : (0.16f * ((float)decay / 8.0f)));
                float buoyancy = 0.04f + jumpBoost;

                if (jumping) motionY += buoyancy;
                moveEntity(motionX, motionY, motionZ);
                motionX *= waterDrag; motionY *= waterDrag; motionZ *= waterDrag;
                if (!isFlying) motionY -= 0.02f;

                if (isCollidedHorizontally && isOffsetPositionInLiquid(motionX, motionY + 0.6000000238418579 - posY + prevPosY, motionZ)) {
                    motionY = 0.30000001192092896;
                }
            } else if (inLava) {
                // Infdev Lava Physics
                float lavaDrag = 0.5f;
                int decay = ((BlockFluid*)Block::lavaMoving)->getEffectiveFlowDecay(worldObj, (int)std::floor(posX), (int)std::floor(posY), (int)std::floor(posZ));
                float jumpBoost = (decay < 0 ? 0.0f : (0.16f * ((float)decay / 8.0f)));
                float buoyancy = 0.04f + jumpBoost;

                if (jumping) motionY += buoyancy;
                moveEntity(motionX, motionY, motionZ);
                motionX *= lavaDrag; motionY *= lavaDrag; motionZ *= lavaDrag;
                if (!isFlying) motionY -= 0.02f;

                if (isCollidedHorizontally && isOffsetPositionInLiquid(motionX, motionY + 0.6000000238418579 - posY + prevPosY, motionZ)) {
                    motionY = 0.30000001192092896;
                }
            } else {
                moveEntity(motionX, motionY, motionZ);
                float drag = 0.91f;
                if (onGround) drag = 0.6f * 0.91f;

                motionX *= drag;
                motionY *= 0.98f;
                motionZ *= drag;

                if (!isFlying) motionY -= 0.08f;
            }
        }
    }

    prevLimbSwing = limbSwing;
    prevLimbSwingAmount = limbSwingAmount;

    float f = dist * 4.0f;
    if (f > 1.0f) f = 1.0f;
    limbSwingAmount += (f - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;

    if (onPlaySound && (onGround || inWater)) {
        // Apply the 0.6 multiplier used in moveEntity to match engine expectations
        float walked = dist * 0.6f;
        
        if (walked > 0.005f) {
            footstepAccum += walked;
            if (footstepAccum >= 0.85f) {
                footstepAccum = 0.0f;
                
                // Check mid-body and feet for liquid
                int feetY = (int)std::floor(posY - 0.2f);
                uint8_t feetBlock = worldObj.getBlockID((int)std::floor(posX), feetY, (int)std::floor(posZ));
                uint8_t midBlock = worldObj.getBlockID((int)std::floor(posX), (int)std::floor(posY + 0.5f), (int)std::floor(posZ));
                
                bool isInLiquid = (feetBlock >= 8 && feetBlock <= 11) || (midBlock >= 8 && midBlock <= 11);
                
                if (isInLiquid) {
                    onPlaySound("liquid.water", 0.4f, 1.0f);
                } else {
                    int bx = (int)std::floor(posX);
                    int by = feetY;
                    int bz = (int)std::floor(posZ);
                    uint8_t bid = feetBlock;
                    if (bid == 0) bid = worldObj.getBlockID(bx, by - 1, bz);
                    if (const Block* b = Block::blocksList[bid]) {
                        onPlaySound(b->stepSound->getStepSound(), 0.5f, 1.0f);
                    }
                }
            }
        }
    }
}

void EntityLiving::swing() {
    if (!isSwinging || swingProgressInt >= 4 || swingProgressInt < 0) {
        swingProgressInt = -1;
        isSwinging = true;
    }
}

void EntityLiving::updateEntityActionState() {
    // regular walking/jumping logic for non-player entities. Player overrides this with flying/sneaking/sprinting logic.
    isFlying = false;

    if (jumping && onGround && (!inWater && !inLava)) {
        motionY = 0.42;
    }

    float speed = onGround ? 0.1f : 0.02f;
    if (inWater || inLava) speed = 0.02f;

    moveRelative(moveStrafe, moveForward, speed);
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
        if (onPlaySound) {
            onPlaySound(distance > 5.0f ? "damage.fallbig" : "damage.hit", 1.0f, 1.0f);
        }
    }
}
