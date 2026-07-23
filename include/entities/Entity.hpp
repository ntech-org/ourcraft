#pragma once

#include "physics/AxisAlignedBB.hpp"
#include <vector>
#include <cstdint>

class World;

enum class EntityType {
    Unknown,
    Player,
    Living,
    Zombie,
    Item
};

class Entity {
public:
    Entity(World& world);
    virtual ~Entity();

    virtual EntityType getType() const { return EntityType::Unknown; }
    virtual void onUpdate();
    void moveEntity(double dx, double dy, double dz);
    void setPosition(double x, double y, double z);
    void setPosAndPrev(double x, double y, double z);
    void setSize(float width, float height);

    World& worldObj;
    int32_t entityID = -1;
    bool isLocalPlayer = false;
    double prevPosX = 0.0, prevPosY = 0.0, prevPosZ = 0.0;
    double posX = 0.0, posY = 0.0, posZ = 0.0;
    double motionX = 0.0, motionY = 0.0, motionZ = 0.0;
    float rotationYaw = 0.0f, rotationPitch = 0.0f;
    float prevRotationYaw = 0.0f, prevRotationPitch = 0.0f;
    AxisAlignedBB boundingBox;
    bool onGround = false;
    bool handlePhysics = true;
    bool isDead = false;

    // Server-side synchronization and interpolation
    int posRotationIncrements = 0;
    double serverPosX = 0.0, serverPosY = 0.0, serverPosZ = 0.0;
    double serverYaw = 0.0, serverPitch = 0.0;

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
    bool inWater = false;
    bool inLava = false;

    virtual void preparePlayerToSpawn();
    bool handleWaterMovement();
    bool handleLavaMovement();
    bool isOffsetPositionInLiquid(double dx, double dy, double dz);

    virtual void fall(float distance);

    bool isInsideOfMaterial(const class Material& material) const;
    bool isEntityInsideOpaqueBlock() const;
};


