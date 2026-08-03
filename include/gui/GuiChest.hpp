#pragma once

#include "gui/GuiInventory.hpp"
#include "net/Packets.hpp"
#include <vector>

class GuiChest : public GuiInventory {
public:
    GuiChest(int rows, std::vector<ItemStack> contents);

    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void onGuiClosed() override;
    void setItems(const std::vector<PacketWindowItems::Item>& items);
    void setSlot(int slot, const ItemStack& stack);

private:
    int getSlotFromMouse(float left, float top, int mouseX, int mouseY) const;
    void getSlotPosition(float left, float top, int slot, float& x, float& y) const;
    void clickSlot(int slot, bool rightClick);

    int m_rows;
    float m_guiHeight;
    std::vector<ItemStack> m_contents;
};
