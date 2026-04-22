#include "entities/EntityPlayer.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

EntityPlayer::EntityPlayer(World& world) : Entity(world) {
    yOffset = 1.62f;
    stepHeight = 0.5f;
    rotationYaw = -90.0f;
    prevRotationYaw = -90.0f;
    rotationPitch = 0.0f;
    prevRotationPitch = 0.0f;
    setPosition(8, 100, 8);
}

void EntityPlayer::onUpdate() {
    Entity::onUpdate();

    if (jumping && onGround) {
        motionY = 0.42;
    }

    moveRelative(moveStrafe, moveForward, onGround ? 0.1f : 0.02f);

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

    motionX += (double)(forward * cosYaw + strafe * sinYaw);
    motionZ += (double)(forward * sinYaw - strafe * cosYaw);
}
