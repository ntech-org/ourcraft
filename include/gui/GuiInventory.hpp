#pragma once

#include "gui/GuiScreen.hpp"
#include "entities/InventoryPlayer.hpp"
#include <array>

class GuiInventory : public GuiScreen {
public:
    static constexpr float GUI_WIDTH = 176.0f;
    static constexpr float GUI_HEIGHT = 166.0f;

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void onGuiClosed() override;

protected:
    void returnCraftingItems();
    int getSlotFromMouse(float left, float top, int mouseX, int mouseY) const;
    void getSlotPosition(float left, float top, int slot, float& outX, float& outY) const;
    void handleClickOnSlot(InventoryPlayer& inv, int slot, bool rightClick);
    void handleDragDistribution(InventoryPlayer& inv, int slot);
    void drawInventorySlots(float left, float top, int mouseX, int mouseY);
    void drawStackAt(const ItemStack& stack, float x, float y, bool highlight);
    void drawCursorStack(int mouseX, int mouseY);
};
