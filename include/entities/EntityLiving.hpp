#pragma once

#include "entities/Entity.hpp"
#include <string>
#include <functional>

class EntityLiving : public Entity {
public:
    EntityLiving(World& world);

    EntityType getType() const override { return EntityType::Living; }
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
    int maxHurtTime = 10;
    float attackedAtYaw = 0.0f;
    bool isFlying = false;

    int air = 300;
    int maxAir = 300;

    int drowningTimer = 0;
    int suffocationTimer = 0;

    virtual void swing();


    virtual void attackEntityFrom(Entity* source, int amount);

    virtual void moveRelative(float strafe, float forward, float friction);

    void fall(float distance) override;

    std::function<void(const std::string&, float, float)> onPlaySound;
    std::function<void()> onHurt;
    int prevHealth = 20;
    float footstepAccum = 0.0f;
    bool wasInWater = false;

protected:
    virtual void updateEntityActionState();
};
