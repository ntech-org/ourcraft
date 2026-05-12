#include "gui/GuiInventory.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

void GuiInventory::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    renderEngine.bindTexture(renderEngine.getTexture("/gui/inventory.png"));
    drawTexturedModalRect(uiShader, left, top, 0, 0, (int)GUI_WIDTH, (int)GUI_HEIGHT);

    drawInventorySlots(left, top, mouseX, mouseY);

    // Draw armor and 2x2 crafting
    InventoryPlayer& inv = mc->getPlayer().inventory;
    int hoveredSlot = getSlotFromMouse(left, top, mouseX, mouseY);
    for (int i = InventoryPlayer::ARMOR_START; i < InventoryPlayer::ARMOR_START + 4; ++i) {
        float sx, sy;
        getSlotPosition(left, top, i, sx, sy);
        drawStackAt(inv.mainInventory[i], sx, sy, i == hoveredSlot);
    }
    for (int i = InventoryPlayer::CRAFT_START; i < InventoryPlayer::CRAFT_START + 4; ++i) {
        float sx, sy;
        getSlotPosition(left, top, i, sx, sy);
        drawStackAt(inv.mainInventory[i], sx, sy, i == hoveredSlot);
    }
    {
        float sx, sy;
        getSlotPosition(left, top, InventoryPlayer::RESULT_SLOT, sx, sy);
        drawStackAt(inv.mainInventory[InventoryPlayer::RESULT_SLOT], sx, sy, InventoryPlayer::RESULT_SLOT == hoveredSlot);
    }

    drawCursorStack(mouseX, mouseY);
}

void GuiInventory::drawInventorySlots(float left, float top, int mouseX, int mouseY) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    int hoveredSlot = getSlotFromMouse(left, top, mouseX, mouseY);

    for (int i = 0; i < 36; ++i) {
        float sx, sy;
        getSlotPosition(left, top, i, sx, sy);
        drawStackAt(inv.mainInventory[i], sx, sy, i == hoveredSlot);
    }
}

void GuiInventory::drawStackAt(const ItemStack& stack, float x, float y, bool highlight) {
    if (highlight) {
        Shader& uiShader = mc->getGameRenderer().getUIShader();
        drawGradientRect(uiShader, x, y, x + 16, y + 16, 0x80ffffff, 0x80ffffff);
    }

    if (stack.isEmpty()) return;

    Gui::drawItemStack(mc, stack, x, y);
}

void GuiInventory::drawBlockStack3D(int blockID, float x, float y) {
    Gui::drawBlockStack3D(mc, blockID, x, y);
}

void GuiInventory::drawItemStack2D(int itemID, float x, float y) {
    Gui::drawItemIcon2D(mc, itemID, x, y);
}

void GuiInventory::drawCursorStack(int mouseX, int mouseY) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    if (!inv.cursorStack.isEmpty()) {
        drawStackAt(inv.cursorStack, (float)mouseX - 8, (float)mouseY - 8, false);
    }
}

int GuiInventory::getSlotFromMouse(float left, float top, int mouseX, int mouseY) const {
    const float relX = (float)mouseX - left;
    const float relY = (float)mouseY - top;

    // Main inventory 3x9
    if (relX >= 7.0f && relX < 169.0f && relY >= 83.0f && relY < 137.0f) {
        int col = (int)((relX - 7.0f) / 18.0f);
        int row = (int)((relY - 83.0f) / 18.0f);
        return 9 + row * 9 + col;
    }

    // Hotbar 1x9
    if (relX >= 7.0f && relX < 169.0f && relY >= 141.0f && relY < 159.0f) {
        int col = (int)((relX - 7.0f) / 18.0f);
        return col;
    }

    // Armor slots 1x4
    if (relX >= 7.0f && relX < 27.0f && relY >= 7.0f && relY < 79.0f) {
        int row = (int)((relY - 7.0f) / 18.0f);
        return InventoryPlayer::ARMOR_START + row;
    }

    // 2x2 Crafting Grid (Inventory Screen)
    if (relX >= 87.0f && relX < 123.0f && relY >= 15.0f && relY < 51.0f) {
        int col = (int)((relX - 87.0f) / 18.0f);
        int row = (int)((relY - 15.0f) / 18.0f);
        return InventoryPlayer::CRAFT_START + row * 2 + col;
    }

    // 2x2 Crafting Result
    if (relX >= 143.0f && relX < 161.0f && relY >= 27.0f && relY < 45.0f) {
        return InventoryPlayer::RESULT_SLOT;
    }

    return -1;
}

void GuiInventory::getSlotPosition(float left, float top, int slot, float& outX, float& outY) const {
    if (slot >= 0 && slot < 9) {
        outX = left + 8.0f + (float)slot * 18.0f;
        outY = top + 142.0f;
    } else if (slot >= 9 && slot < 36) {
        int idx = slot - 9;
        outX = left + 8.0f + (float)(idx % 9) * 18.0f;
        outY = top + 84.0f + (float)(idx / 9) * 18.0f;
    } else if (slot >= InventoryPlayer::ARMOR_START && slot < InventoryPlayer::ARMOR_START + 4) {
        outX = left + 8.0f;
        outY = top + 8.0f + (float)(slot - InventoryPlayer::ARMOR_START) * 18.0f;
    } else if (slot >= InventoryPlayer::CRAFT_START && slot < InventoryPlayer::CRAFT_START + 4) {
        int idx = slot - InventoryPlayer::CRAFT_START;
        outX = left + 88.0f + (float)(idx % 2) * 18.0f;
        outY = top + 16.0f + (float)(idx / 2) * 18.0f;
    } else if (slot == InventoryPlayer::RESULT_SLOT) {
        outX = left + 144.0f;
        outY = top + 28.0f;
    }
}

void GuiInventory::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;
    if (key == SDLK_E || key == SDLK_ESCAPE) {
        if (parentScreen) {
            mc->displayGuiScreen(parentScreen);
        } else if (mc->getGameState() != GameState::MainMenu) {
            mc->displayGuiScreen(nullptr);
        }
        return;
    }
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiInventory::mouseClicked(int mouseX, int mouseY, int button) {
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    int slot = getSlotFromMouse(left, top, mouseX, mouseY);
    if (slot >= 0) {
        handleClickOnSlot(mc->getPlayer().inventory, slot, button == SDL_BUTTON_RIGHT);
    } else {
        // Drop item
        handleClickOnSlot(mc->getPlayer().inventory, -1, button == SDL_BUTTON_RIGHT);
    }
}

void GuiInventory::handleClickOnSlot(InventoryPlayer& inv, int slot, bool rightClick) {
    inv.handleClick(slot, rightClick);
    
    if (mc->getNetworkHandler()) {
        PacketClickWindow packet;
        packet.windowId = 0; // Inventory
        packet.slot = slot;
        packet.button = rightClick ? 1 : 0;
        packet.actionId = 0; // For now
        packet.shift = false; // For now
        
        ItemStack stack = (slot >= 0 && slot < InventoryPlayer::TOTAL_SIZE) ? inv.mainInventory[slot] : ItemStack{0, 0, 0};
        packet.itemID = stack.itemID;
        packet.count = stack.count;
        packet.metadata = stack.metadata;
        
        mc->getNetworkHandler()->sendPacket(packet);
    }
}

