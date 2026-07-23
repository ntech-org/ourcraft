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

    EntityType getType() const override { return EntityType::Player; }
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
    virtual void openFurnace(class World& world, int x, int y, int z) { if (onOpenFurnace) onOpenFurnace(x, y, z); }
    std::function<void()> onOpenCraftingTable;
    std::function<void(int, int, int)> onOpenFurnace;

    void setMinecraft(Minecraft* mc) { this->mc = mc; }
    Minecraft& getMinecraft() { return *mc; }

private:
    Minecraft* mc = nullptr;
};
