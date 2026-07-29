#include "gui/GuiCreativeInventory.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "net/Packets.hpp"
#include <SDL3/SDL.h>

GuiCreativeInventory::GuiCreativeInventory() {
    for (int id = 1; id < 256; ++id) {
        if (Block::blocksList[id]) {
            m_creativeItems.push_back({id, 1, 0});
        }
    }
    for (int id = 256; id < 1024; ++id) {
        if (Item::itemsList[id]) {
            m_creativeItems.push_back({id, 1, 0});
        }
    }
}

void GuiCreativeInventory::initGui() {
    if (mc) {
        mc->getPlayer().inventory.updateCrafting();
        mc->getPlayer().inventory.cursorStack = {0, 0, 0};
    }
}

float GuiCreativeInventory::getInvLeft() const {
    return (width - INVENTORY_GUI_WIDTH) * 0.5f;
}

float GuiCreativeInventory::getInvTop() const {
    float topMargin = 20.0f;
    return topMargin;
}

int GuiCreativeInventory::getCreativeSlotFromMouse(float invLeft, float invTop, int mouseX, int mouseY) const {
    float gridTop = invTop;
    float relX = (float)mouseX - invLeft;
    float relY = (float)mouseY - gridTop;

    float gridWidth = COLS * SLOT_SIZE;
    float gridHeight = 5 * SLOT_SIZE;

    if (relX >= 0.0f && relX < gridWidth && relY >= 0.0f && relY < gridHeight) {
        int col = (int)(relX / SLOT_SIZE);
        int row = (int)(relY / SLOT_SIZE);
        int index = (row + m_scrollOffset) * COLS + col;
        if (index >= 0 && index < (int)m_creativeItems.size()) {
            return index;
        }
    }
    return -1;
}

void GuiCreativeInventory::getCreativeSlotPosition(float invLeft, float invTop, int index, float& outX, float& outY) const {
    int row = index / COLS - m_scrollOffset;
    int col = index % COLS;
    outX = invLeft + (float)col * SLOT_SIZE;
    outY = invTop + (float)row * SLOT_SIZE;
}

int GuiCreativeInventory::getInventorySlotFromMouse(float invLeft, float invTop, int mouseX, int mouseY) const {
    float invGridTop = invTop + 5 * SLOT_SIZE + 14.0f;
    float relX = (float)mouseX - invLeft;
    float relY = (float)mouseY - invGridTop;

    // Hotbar 1x9
    if (relX >= 0.0f && relX < 162.0f && relY >= 132.0f && relY < 150.0f) {
        int col = (int)(relX / 18.0f);
        return col;
    }

    // Main inventory 3x9
    if (relX >= 0.0f && relX < 162.0f && relY >= 74.0f && relY < 128.0f) {
        int col = (int)(relX / 18.0f);
        int row = (int)((relY - 74.0f) / 18.0f);
        return 9 + row * 9 + col;
    }

    return -1;
}

void GuiCreativeInventory::getInventorySlotPosition(float invLeft, float invTop, int slot, float& outX, float& outY) const {
    float invGridTop = invTop + 5 * SLOT_SIZE + 14.0f;
    if (slot >= 0 && slot < 9) {
        outX = invLeft + 8.0f + (float)slot * 18.0f;
        outY = invGridTop + 133.0f;
    } else if (slot >= 9 && slot < 36) {
        int idx = slot - 9;
        outX = invLeft + 8.0f + (float)(idx % 9) * 18.0f;
        outY = invGridTop + 75.0f + (float)(idx / 9) * 18.0f;
    }
}

void GuiCreativeInventory::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    Shader& uiShader = mc->getGameRenderer().getUIShader();

    float invLeft = getInvLeft();
    float invTop = getInvTop();

    drawCreativeGrid(invLeft, invTop, mouseX, mouseY);
    drawInventoryArea(invLeft, invTop, mouseX, mouseY);
    drawCursorStack(mouseX, mouseY);
}

void GuiCreativeInventory::drawCreativeGrid(float invLeft, float invTop, int mouseX, int mouseY) {
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    int hoveredIndex = getCreativeSlotFromMouse(invLeft, invTop, mouseX, mouseY);

    int startRow = m_scrollOffset;
    int endRow = std::min(startRow + 5, (int)((m_creativeItems.size() + COLS - 1) / COLS));

    for (int row = startRow; row < endRow; ++row) {
        for (int col = 0; col < COLS; ++col) {
            int index = row * COLS + col;
            if (index >= (int)m_creativeItems.size()) break;

            float sx, sy;
            getCreativeSlotPosition(invLeft, invTop, index, sx, sy);
            ItemStack stack = {m_creativeItems[index].itemID, m_creativeItems[index].count, m_creativeItems[index].metadata};
            drawStackAt(stack, sx, sy, index == hoveredIndex);
        }
    }
}

void GuiCreativeInventory::drawInventoryArea(float invLeft, float invTop, int mouseX, int mouseY) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    float invGridTop = invTop + 5 * SLOT_SIZE + 14.0f;
    int hoveredSlot = getInventorySlotFromMouse(invLeft, invTop, mouseX, mouseY);

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Gui::drawRect(uiShader, invLeft, invGridTop - 1.0f, invLeft + INVENTORY_GUI_WIDTH, invGridTop + 166.0f, 0xC0000000);

    for (int i = 0; i < 36; ++i) {
        float sx, sy;
        getInventorySlotPosition(invLeft, invTop, i, sx, sy);
        drawStackAt(inv.mainInventory[i], sx, sy, i == hoveredSlot);
    }
}

void GuiCreativeInventory::drawStackAt(const ItemStack& stack, float x, float y, bool highlight) {
    if (highlight) {
        Shader& uiShader = mc->getGameRenderer().getUIShader();
        drawGradientRect(uiShader, x, y, x + 16, y + 16, 0x80ffffff, 0x80ffffff);
    }
    if (stack.isEmpty()) return;
    Gui::drawItemStack(mc, stack, x, y);
}

void GuiCreativeInventory::drawCursorStack(int mouseX, int mouseY) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    if (!inv.cursorStack.isEmpty()) {
        drawStackAt(inv.cursorStack, (float)mouseX - 8, (float)mouseY - 8, false);
    }
}

void GuiCreativeInventory::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        handleMouseWheel(0, event.wheel.y > 0 ? 1 : -1);
        return;
    }
    GuiScreen::handleEvent(event);
}

void GuiCreativeInventory::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;
    if (key == SDLK_E || key == SDLK_ESCAPE) {
        if (mc) {
            mc->getPlayer().inventory.cursorStack = {0, 0, 0};
            mc->displayGuiScreen(nullptr);
        }
        return;
    }
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiCreativeInventory::mouseClicked(int mouseX, int mouseY, int button) {
    float invLeft = getInvLeft();
    float invTop = getInvTop();
    InventoryPlayer& inv = mc->getPlayer().inventory;
    const bool rightClick = (button == SDL_BUTTON_RIGHT);

    int creativeSlot = getCreativeSlotFromMouse(invLeft, invTop, mouseX, mouseY);
    if (creativeSlot >= 0 && creativeSlot < (int)m_creativeItems.size()) {
        const CreativeItem& ci = m_creativeItems[creativeSlot];
        if (rightClick && !inv.cursorStack.isEmpty() &&
            inv.cursorStack.itemID == ci.itemID && inv.cursorStack.metadata == ci.metadata) {
            if (inv.cursorStack.count < InventoryPlayer::MAX_STACK_SIZE)
                inv.cursorStack.count += 1;
        } else {
            inv.cursorStack = {ci.itemID, rightClick ? 1 : ci.count, ci.metadata};
        }
        return;
    }

    int invSlot = getInventorySlotFromMouse(invLeft, invTop, mouseX, mouseY);
    if (invSlot >= 0) {
        if (rightClick) {
            if (inv.cursorStack.isEmpty() && !inv.mainInventory[invSlot].isEmpty()) {
                int take = (inv.mainInventory[invSlot].count + 1) / 2;
                inv.cursorStack = inv.mainInventory[invSlot];
                inv.cursorStack.count = take;
                inv.mainInventory[invSlot].count -= take;
                if (inv.mainInventory[invSlot].count <= 0) inv.mainInventory[invSlot] = {0, 0, 0};
            } else if (!inv.cursorStack.isEmpty()) {
                if (inv.mainInventory[invSlot].isEmpty()) {
                    inv.mainInventory[invSlot] = inv.cursorStack;
                    inv.mainInventory[invSlot].count = 1;
                    inv.cursorStack.count -= 1;
                    if (inv.cursorStack.count <= 0) inv.cursorStack = {0, 0, 0};
                } else if (inv.mainInventory[invSlot].itemID == inv.cursorStack.itemID &&
                           inv.mainInventory[invSlot].metadata == inv.cursorStack.metadata &&
                           inv.mainInventory[invSlot].count < InventoryPlayer::MAX_STACK_SIZE) {
                    inv.mainInventory[invSlot].count += 1;
                    inv.cursorStack.count -= 1;
                    if (inv.cursorStack.count <= 0) inv.cursorStack = {0, 0, 0};
                }
            }
        } else {
            std::swap(inv.cursorStack, inv.mainInventory[invSlot]);
        }

        if (mc->getNetworkHandler()) {
            PacketClickWindow packet;
            packet.windowId = 0;
            packet.slot = invSlot;
            packet.button = rightClick ? 1 : 0;
            packet.actionId = 0;
            packet.shift = false;
            packet.itemID = inv.mainInventory[invSlot].itemID;
            packet.count = inv.mainInventory[invSlot].count;
            packet.metadata = inv.mainInventory[invSlot].metadata;
            mc->getNetworkHandler()->sendPacket(packet);
        }
        return;
    }

    // Click outside inventory: drop cursor stack
    if (!inv.cursorStack.isEmpty() && mc->getNetworkHandler()) {
        PacketClickWindow packet;
        packet.windowId = 0;
        packet.slot = -1;
        packet.button = rightClick ? 1 : 0;
        packet.actionId = 0;
        packet.shift = false;
        packet.itemID = inv.cursorStack.itemID;
        packet.count = inv.cursorStack.count;
        packet.metadata = inv.cursorStack.metadata;
        mc->getNetworkHandler()->sendPacket(packet);

        if (rightClick) {
            inv.cursorStack.count -= 1;
            if (inv.cursorStack.count <= 0) inv.cursorStack = {0, 0, 0};
        } else {
            inv.cursorStack = {0, 0, 0};
        }
    }
}

bool GuiCreativeInventory::handleMouseWheel(int x, int y) {
    if (y > 0) m_scrollOffset = std::max(0, m_scrollOffset - 1);
    else if (y < 0) m_scrollOffset = std::min(getMaxScroll(), m_scrollOffset + 1);
    return true;
}
