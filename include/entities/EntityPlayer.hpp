#pragma once

#include "entities/Entity.hpp"

class EntityPlayer : public Entity {
public:
    EntityPlayer(World& world);
    
    void onUpdate() override;
    void moveRelative(float strafe, float forward, float friction);
    
    float moveForward = 0.0f;
    float moveStrafe = 0.0f;
    bool jumping = false;
};
