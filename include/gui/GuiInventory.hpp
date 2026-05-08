#pragma once

#include "gui/GuiScreen.hpp"
#include "entities/InventoryPlayer.hpp"
#include <array>

class GuiInventory : public GuiScreen {
public:
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(int key, int scancode, int action, int mods) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

private:
    static constexpr float GUI_WIDTH = 176.0f;
    static constexpr float GUI_HEIGHT = 166.0f;

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

    ItemStack m_cursorStack {};
    bool m_draggingLeft = false;
    std::array<bool, InventoryPlayer::INVENTORY_SIZE> m_dragVisited {};
};
