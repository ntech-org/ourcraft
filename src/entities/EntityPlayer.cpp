#include "entities/EntityPlayer.hpp"
#include "world/Material.hpp"
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

EntityPlayer::EntityPlayer(World& world) : EntityLiving(world) {
    yOffset = 1.62f;
    stepHeight = 0.5f;
    rotationYaw = -90.0f;
    prevRotationYaw = -90.0f;
    rotationPitch = 0.0f;
    prevRotationPitch = 0.0f;
    setPosition(8, 100, 8);
}

void EntityPlayer::onUpdate() {
    prevCameraYaw = cameraYaw;
    prevCameraPitch = cameraPitch;
    EntityLiving::onUpdate();

    if (isInsideOfMaterial(Material::water)) {
        if (air > 0) {
            air--;
        } else {
            // Drowning damage every 20 ticks
            if (drowningTimer++ >= 20) {
                attackEntityFrom(nullptr, 1);
                drowningTimer = 0;
            }
        }
    } else {
        if (!isEntityInsideOpaqueBlock()) {
            air = maxAir;
        }
    }

    if (isEntityInsideOpaqueBlock()) {
        // Suffocation damage every 20 ticks
        if (suffocationTimer++ >= 20) {
            attackEntityFrom(nullptr, 1);
            suffocationTimer = 0;
        }
    } else {
        suffocationTimer = 0;
    }



    float speed = (float)std::sqrt(motionX * motionX + motionZ * motionZ);

    float pitchTarget = (float)std::atan(-motionY * 0.2f) * 15.0f;

    if (speed > 0.1f) speed = 0.1f;
    if (!onGround) speed = 0.0f;
    if (onGround) pitchTarget = 0.0f;

    cameraYaw += (speed - cameraYaw) * 0.4f;
    cameraPitch += (pitchTarget - cameraPitch) * 0.8f;
}

void EntityPlayer::updateEntityActionState() {
    if (isFlying && gameMode == GameMode::CREATIVE) {
        float flySpeed = 0.12f; // Default horizontal
        if (sprinting) flySpeed *= 2.0f;
        if (sneaking) flySpeed *= 0.5f;

        moveRelative(moveStrafe, moveForward, flySpeed);

        float vertSpeed = 0.2f;
        if (sprinting) vertSpeed *= 2.0f;

        if (jumping) {
            motionY += vertSpeed;
        } else if (sneaking) {
            motionY -= vertSpeed;
        } else {
            motionY = 0; // Explicitly hover when no vertical keys are pressed
        }

        // Damping only applied when actually moving vertically
        if (jumping || sneaking) {
            motionY *= 0.8;
        }

    } else {
        EntityLiving::updateEntityActionState();
    }
}


void EntityPlayer::attackEntityFrom(Entity* source, int amount) {
    if (gameMode == GameMode::CREATIVE) return;
    EntityLiving::attackEntityFrom(source, amount);
}
