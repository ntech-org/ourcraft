#include "entities/EntityPlayer.hpp"
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

        isFlying = false; // Force land if not creative
        if (jumping && onGround && !inWater) {
            motionY = 0.42;
        }

        float speed = onGround ? 0.1f : 0.02f;
        if (inWater) speed = 0.02f;

        moveRelative(moveStrafe, moveForward, speed);
    }
}


void EntityPlayer::moveRelative(float strafe, float forward, float friction) {
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

void EntityPlayer::attackEntityFrom(Entity* source, int amount) {
    if (gameMode == GameMode::CREATIVE) return;
    EntityLiving::attackEntityFrom(source, amount);
}

