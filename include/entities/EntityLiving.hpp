#pragma once

#include "entities/Entity.hpp"

class EntityLiving : public Entity {
public:
    EntityLiving(World& world);

    void onUpdate() override;

    float limbSwing;
    float prevLimbSwing;
    float limbSwingAmount;
    float prevLimbSwingAmount;
    float swingProgress;
    float prevSwingProgress;
    bool isSwinging;
    int swingProgressInt;

    float renderYawOffset = 0.0f;
    float prevRenderYawOffset = 0.0f;

    float moveForward = 0.0f;
    float moveStrafe = 0.0f;

    int deathTime = 0;
    int attackTime = 0;
    bool jumping = false;

    int health = 20;
    int maxHealth = 20;
    int hurtTime = 0;
    bool isFlying = false;

    int air = 300;
    int maxAir = 300;

    int drowningTimer = 0;
    int suffocationTimer = 0;

    virtual void swing();


    virtual void attackEntityFrom(Entity* source, int amount);

    virtual void moveRelative(float strafe, float forward, float friction);

    void fall(float distance) override;

protected:
    virtual void updateEntityActionState();
};
