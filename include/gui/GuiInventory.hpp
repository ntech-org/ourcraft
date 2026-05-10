#pragma once

#include "gui/GuiScreen.hpp"
#include "entities/InventoryPlayer.hpp"
#include <array>

class GuiInventory : public GuiScreen {
public:
    static constexpr float GUI_WIDTH = 176.0f;
    static constexpr float GUI_HEIGHT = 166.0f;

    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
    int getSlotFromMouse(float left, float top, int mouseX, int mouseY) const;
    void getSlotPosition(float left, float top, int slot, float& outX, float& outY) const;
    void handleClickOnSlot(InventoryPlayer& inv, int slot, bool rightClick);
    void handleDragDistribution(InventoryPlayer& inv, int slot);
    void drawInventorySlots(float left, float top, int mouseX, int mouseY);
    void drawStackAt(const ItemStack& stack, float x, float y, bool highlight);
    void drawBlockStack3D(int blockID, float x, float y);
    void drawBlockStack2D(int blockID, float x, float y);
    void drawItemStack2D(int itemID, float x, float y);
    void drawCursorStack(int mouseX, int mouseY);

    int16_t m_actionCount = 0;
    bool m_draggingLeft = false;
    std::array<bool, InventoryPlayer::TOTAL_SIZE> m_dragVisited {};
};
