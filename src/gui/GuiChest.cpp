#include "gui/GuiChest.hpp"
#include "Minecraft.hpp"
#include "net/Packets.hpp"
#include "renderer/RenderEngine.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

GuiChest::GuiChest(int rows, std::vector<ItemStack> contents)
    : m_rows(std::clamp(rows, 1, 6)), m_guiHeight(114.0f + (float)std::clamp(rows, 1, 6) * 18.0f),
      m_contents(std::move(contents)) {
    m_contents.resize((std::size_t)m_rows * 9);
}

void GuiChest::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    InventoryPlayer& inventory = mc->getPlayer().inventory;
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - m_guiHeight) * 0.5f;

    renderEngine.bindTexture(renderEngine.getTexture("/gui/container.png"));
    int chestHeight = m_rows * 18 + 17;
    drawTexturedModalRect(uiShader, left, top, 0, 0, (int)GUI_WIDTH, chestHeight);
    drawTexturedModalRect(uiShader, left, top + (float)chestHeight, 0, 126, (int)GUI_WIDTH, 96);

    int hovered = getSlotFromMouse(left, top, mouseX, mouseY);
    for (int i = 0; i < (int)m_contents.size() + InventoryPlayer::INVENTORY_SIZE; ++i) {
        float x, y;
        getSlotPosition(left, top, i, x, y);
        const ItemStack& stack = i < (int)m_contents.size()
            ? m_contents[i]
            : inventory.mainInventory[i - (int)m_contents.size()];
        drawStackAt(stack, x, y, i == hovered);
    }

    drawString(font, textShader, m_rows == 6 ? "Large Chest" : "Chest", left + 8.0f, top + 6.0f, 0xFFA0A0A0);
    drawString(font, textShader, "Inventory", left + 8.0f, top + (float)m_rows * 18.0f + 20.0f, 0xFFA0A0A0);
    drawCursorStack(mouseX, mouseY);
}

int GuiChest::getSlotFromMouse(float left, float top, int mouseX, int mouseY) const {
    float x = (float)mouseX - left;
    float y = (float)mouseY - top;
    if (x >= 7.0f && x < 169.0f && y >= 17.0f && y < 17.0f + m_rows * 18.0f) {
        int col = (int)((x - 7.0f) / 18.0f);
        int row = (int)((y - 17.0f) / 18.0f);
        if (col >= 0 && col < 9 && row >= 0 && row < m_rows) return row * 9 + col;
    }

    float inventoryY = (float)m_rows * 18.0f + 31.0f;
    if (x >= 7.0f && x < 169.0f && y >= inventoryY && y < inventoryY + 54.0f) {
        int col = (int)((x - 7.0f) / 18.0f);
        int row = (int)((y - inventoryY) / 18.0f);
        if (col >= 0 && col < 9 && row >= 0 && row < 3) return (int)m_contents.size() + row * 9 + col + 9;
    }
    if (x >= 7.0f && x < 169.0f && y >= inventoryY + 58.0f && y < inventoryY + 76.0f) {
        int col = (int)((x - 7.0f) / 18.0f);
        if (col >= 0 && col < 9) return (int)m_contents.size() + col;
    }
    return -1;
}

void GuiChest::getSlotPosition(float left, float top, int slot, float& x, float& y) const {
    if (slot < (int)m_contents.size()) {
        x = left + 8.0f + (slot % 9) * 18.0f;
        y = top + 18.0f + (slot / 9) * 18.0f;
        return;
    }
    int inventorySlot = slot - (int)m_contents.size();
    float inventoryY = top + (float)m_rows * 18.0f + 32.0f;
    if (inventorySlot < 9) {
        x = left + 8.0f + inventorySlot * 18.0f;
        y = inventoryY + 58.0f;
    } else {
        int mainSlot = inventorySlot - 9;
        x = left + 8.0f + (mainSlot % 9) * 18.0f;
        y = inventoryY + (mainSlot / 9) * 18.0f;
    }
}

void GuiChest::mouseClicked(int mouseX, int mouseY, int button) {
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - m_guiHeight) * 0.5f;
    clickSlot(getSlotFromMouse(left, top, mouseX, mouseY), button == SDL_BUTTON_RIGHT);
}

void GuiChest::clickSlot(int slot, bool rightClick) {
    InventoryPlayer& inventory = mc->getPlayer().inventory;
    if (slot < 0) return;
    if (slot < (int)m_contents.size()) {
        ItemStack& target = m_contents[slot];
        if (inventory.cursorStack.isEmpty()) {
            if (!target.isEmpty()) inventory.cursorStack = target.splitStack(rightClick ? (target.count + 1) / 2 : target.count);
        } else if (target.isEmpty()) {
            target = inventory.cursorStack.splitStack(rightClick ? 1 : inventory.cursorStack.count);
        } else if (target.isItemEqual(inventory.cursorStack)) {
            int amount = std::min(rightClick ? 1 : inventory.cursorStack.count, 64 - target.count);
            if (amount > 0) { target.count += amount; inventory.cursorStack.splitStack(amount); }
        } else if (!rightClick) {
            std::swap(target, inventory.cursorStack);
        }
    } else {
        inventory.handleClick(slot - (int)m_contents.size(), rightClick);
    }

    PacketClickWindow packet;
    packet.windowId = 1; packet.slot = slot; packet.button = rightClick ? 1 : 0;
    packet.actionId = 0; packet.shift = false;
    packet.itemID = 0; packet.count = 0; packet.metadata = 0;
    mc->getNetworkHandler()->sendPacket(packet);
}

void GuiChest::setItems(const std::vector<PacketWindowItems::Item>& items) {
    InventoryPlayer& inventory = mc->getPlayer().inventory;
    for (std::size_t i = 0; i < items.size(); ++i) {
        ItemStack stack = {items[i].id, items[i].count, items[i].metadata, 0};
        if (i < m_contents.size()) m_contents[i] = stack;
        else if (i - m_contents.size() < InventoryPlayer::INVENTORY_SIZE) inventory.mainInventory[i - m_contents.size()] = stack;
    }
}

void GuiChest::setSlot(int slot, const ItemStack& stack) {
    if (slot >= 0 && slot < (int)m_contents.size()) m_contents[slot] = stack;
}

void GuiChest::onGuiClosed() {
    if (!mc || !mc->getNetworkHandler()) return;
    PacketClickWindow packet;
    packet.windowId = 1; packet.slot = -2; packet.button = 0; packet.actionId = 0;
    packet.shift = false; packet.itemID = 0; packet.count = 0; packet.metadata = 0;
    mc->getNetworkHandler()->sendPacket(packet);
}
