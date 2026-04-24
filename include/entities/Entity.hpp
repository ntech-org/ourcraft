#pragma once

#include "physics/AxisAlignedBB.hpp"
#include <vector>
#include <cstdint>

class World;

class Entity {
public:
    Entity(World& world);
    virtual ~Entity();

    virtual void onUpdate();
    void moveEntity(double dx, double dy, double dz);
    void setPosition(double x, double y, double z);
    void setSize(float width, float height);

    World& worldObj;
    int32_t entityID = -1;
    double prevPosX = 0.0, prevPosY = 0.0, prevPosZ = 0.0;
    double posX = 0.0, posY = 0.0, posZ = 0.0;
    double motionX = 0.0, motionY = 0.0, motionZ = 0.0;
    float rotationYaw = 0.0f, rotationPitch = 0.0f;
    float prevRotationYaw = 0.0f, prevRotationPitch = 0.0f;
    AxisAlignedBB boundingBox;
    bool onGround = false;
    bool handlePhysics = true;
    bool isCollidedHorizontally = false;
    bool isCollided = false;
    float width = 0.6f;
    float height = 1.8f;
    float yOffset = 0.0f;
    float ySize = 0.0f;
    float stepHeight = 0.0f;
    float fallDistance = 0.0f;
    float distanceWalkedModified = 0.0f;
    float prevDistanceWalkedModified = 0.0f;

    virtual void preparePlayerToSpawn();
};
