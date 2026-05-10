#pragma once

#include "gui/GuiInventory.hpp"
#include <array>

class GuiCrafting : public GuiInventory {
public:
    GuiCrafting();

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void onGuiClosed() override;

private:
    int getCraftingSlotFromMouse(float left, float top, int mouseX, int mouseY) const;
    void getCraftingSlotPosition(float left, float top, int slot, float& outX, float& outY) const;
    void handleCraftingClick(int slot, bool rightClick);
    void updateCrafting();

    ItemStack m_craftingGrid[9];
    ItemStack m_craftingResult;
};
