#pragma once
#include "gui/GuiInventory.hpp"

class TileEntityFurnace;

class GuiFurnace : public GuiInventory {
public:
    GuiFurnace(TileEntityFurnace& furnace);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

private:
    int getFurnaceSlotFromMouse(float left, float top, int mouseX, int mouseY) const;
    void getFurnaceSlotPosition(float left, float top, int slot, float& outX, float& outY) const;
    void handleFurnaceClick(int slot, bool rightClick);

    TileEntityFurnace& m_furnace;
};
