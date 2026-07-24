#pragma once

#include "gui/GuiScreen.hpp"
#include "entities/InventoryPlayer.hpp"
#include <vector>

struct CreativeItem {
    int itemID;
    int count;
    uint8_t metadata;
};

class GuiCreativeInventory : public GuiScreen {
public:
    GuiCreativeInventory();

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void handleEvent(const SDL_Event& event) override;
    bool handleMouseWheel(int x, int y);
    bool wantsCursor() const override { return true; }

private:
    static constexpr int COLS = 9;
    static constexpr float SLOT_SIZE = 18.0f;
    static constexpr float INVENTORY_GUI_WIDTH = 176.0f;
    static constexpr float INVENTORY_GUI_HEIGHT = 166.0f;

    float getInvLeft() const;
    float getInvTop() const;

    int getCreativeSlotFromMouse(float invLeft, float invTop, int mouseX, int mouseY) const;
    void getCreativeSlotPosition(float invLeft, float invTop, int index, float& outX, float& outY) const;
    int getInventorySlotFromMouse(float invLeft, float invTop, int mouseX, int mouseY) const;
    void getInventorySlotPosition(float invLeft, float invTop, int slot, float& outX, float& outY) const;

    void drawInventoryArea(float invLeft, float invTop, int mouseX, int mouseY);
    void drawCreativeGrid(float invLeft, float invTop, int mouseX, int mouseY);
    void drawStackAt(const ItemStack& stack, float x, float y, bool highlight);
    void drawCursorStack(int mouseX, int mouseY);

    std::vector<CreativeItem> m_creativeItems;
    int m_scrollOffset = 0;
    int getRows() const { return ((int)m_creativeItems.size() + COLS - 1) / COLS; }
    int getMaxScroll() const { return std::max(0, getRows() - 5); }
};
