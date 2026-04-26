#pragma once

#include "entities/EntityLiving.hpp"
#include "entities/InventoryPlayer.hpp"

enum class GameMode {

    SURVIVAL,
    CREATIVE
};

class EntityPlayer : public EntityLiving {
public:
    EntityPlayer(World& world);

    void onUpdate() override;
    void updateEntityActionState() override;

    float cameraYaw = 0.0f;
    float prevCameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float prevCameraPitch = 0.0f;

    GameMode gameMode = GameMode::CREATIVE;
    InventoryPlayer inventory;

    bool isFlying = false;
    bool sneaking = false;
    bool sprinting = false;


    void attackEntityFrom(Entity* source, int amount) override;

};
