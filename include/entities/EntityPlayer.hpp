#pragma once

#include "entities/EntityLiving.hpp"

class EntityPlayer : public EntityLiving {
public:
    EntityPlayer(World& world);

    void onUpdate() override;
    void updateEntityActionState() override;
    void moveRelative(float strafe, float forward, float friction);

    float moveForward = 0.0f;
    float moveStrafe = 0.0f;

    float cameraYaw = 0.0f;
    float prevCameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float prevCameraPitch = 0.0f;
};
