#pragma once

#include "entities/EntityLiving.hpp"
#include "entities/InventoryPlayer.hpp"
#include <functional>
#include <string>

enum class GameMode {

    SURVIVAL,
    CREATIVE
};

class Minecraft;

class EntityPlayer : public EntityLiving {
public:
    EntityPlayer(World& world);

    void onUpdate() override;
    void updateEntityActionState() override;

    float cameraYaw = 0.0f;
    float prevCameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float prevCameraPitch = 0.0f;

    GameMode gameMode = GameMode::SURVIVAL;
    InventoryPlayer inventory;

    std::string username = "Player";
    std::string uuid = "";

    bool isFlying = false;
    bool sneaking = false;
    bool sprinting = false;

    virtual void attackEntityFrom(Entity* source, int amount) override;
    virtual void openCraftingTable() { if (onOpenCraftingTable) onOpenCraftingTable(); }
    std::function<void()> onOpenCraftingTable;

    void setMinecraft(Minecraft* mc) { this->mc = mc; }
    Minecraft& getMinecraft() { return *mc; }

private:
    Minecraft* mc = nullptr;
};
