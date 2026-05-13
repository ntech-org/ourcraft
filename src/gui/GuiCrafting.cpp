#include "gui/GuiCrafting.hpp"
#include "Minecraft.hpp"
#include "inventory/CraftingManager.hpp"
#include "renderer/RenderEngine.hpp"
#include <SDL3/SDL.h>

GuiCrafting::GuiCrafting() {
}

void GuiCrafting::initGui() {
    if (mc) {
        mc->getPlayer().inventory.updateCrafting();
    }
}

int GuiCrafting::getCraftingSlotFromMouse(float left, float top, int mouseX, int mouseY) const {
    const float relX = (float)mouseX - (left + 30.0f);
    const float relY = (float)mouseY - (top + 17.0f);
    if (relX >= 0.0f && relY >= 0.0f) {
        int col = (int)(relX / 18.0f);
        int row = (int)(relY / 18.0f);
        if (col >= 0 && col < 3 && row >= 0 && row < 3) return InventoryPlayer::WORKBENCH_START + row * 3 + col;
    }

    const float resX = (float)mouseX - (left + 124.0f);
    const float resY = (float)mouseY - (top + 35.0f);
    if (resX >= 0.0f && resX < 18.0f && resY >= 0.0f && resY < 18.0f) return InventoryPlayer::WORKBENCH_RESULT;

    return -1;
}

void GuiCrafting::getCraftingSlotPosition(float left, float top, int slot, float& outX, float& outY) const {
    if (slot >= InventoryPlayer::WORKBENCH_START && slot < InventoryPlayer::WORKBENCH_START + 9) {
        int idx = slot - InventoryPlayer::WORKBENCH_START;
        outX = left + 30.0f + (float)(idx % 3) * 18.0f;
        outY = top + 17.0f + (float)(idx / 3) * 18.0f;
    } else if (slot == InventoryPlayer::WORKBENCH_RESULT) {
        outX = left + 124.0f;
        outY = top + 35.0f;
    }
}

void GuiCrafting::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    InventoryPlayer& inv = mc->getPlayer().inventory;
    
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    renderEngine.bindTexture(renderEngine.getTexture("/gui/crafting.png"));
    drawTexturedModalRect(uiShader, left, top, 0, 0, (int)GUI_WIDTH, (int)GUI_HEIGHT);

    // Draw grid and result
    int hoveredCraftSlot = getCraftingSlotFromMouse(left, top, mouseX, mouseY);
    for (int i = 0; i < 10; ++i) {
        int slot = (i < 9) ? (InventoryPlayer::WORKBENCH_START + i) : InventoryPlayer::WORKBENCH_RESULT;
        float sx, sy;
        getCraftingSlotPosition(left, top, slot, sx, sy);
        drawStackAt(inv.mainInventory[slot], sx, sy, slot == hoveredCraftSlot);
    }

    // Draw player inventory (reusing GuiInventory logic)
    drawInventorySlots(left, top, mouseX, mouseY);
    drawCursorStack(mouseX, mouseY);
}

void GuiCrafting::mouseClicked(int mouseX, int mouseY, int button) {
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    int craftSlot = getCraftingSlotFromMouse(left, top, mouseX, mouseY);
    if (craftSlot >= 0) {
        handleClickOnSlot(mc->getPlayer().inventory, craftSlot, button == SDL_BUTTON_RIGHT);
    } else {
        int invSlot = getSlotFromMouse(left, top, mouseX, mouseY);
        if (invSlot >= 0) {
            handleClickOnSlot(mc->getPlayer().inventory, invSlot, button == SDL_BUTTON_RIGHT);
        }
    }
}

void GuiCrafting::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;
    if (key == SDLK_E || key == SDLK_ESCAPE) {
        if (parentScreen) {
            mc->displayGuiScreen(parentScreen);
        } else if (mc->getGameState() != GameState::MainMenu) {
            mc->displayGuiScreen(nullptr);
        }
        return;
    }
}

void GuiCrafting::onGuiClosed() {
    // Return items to inventory
    InventoryPlayer& inv = mc->getPlayer().inventory;
    for (int i = 0; i < 9; ++i) {
        int slot = InventoryPlayer::WORKBENCH_START + i;
        if (!inv.mainInventory[slot].isEmpty()) {
            ItemStack stack = inv.mainInventory[slot];
            inv.mainInventory[slot] = {0, 0, 0};
            if (!inv.addItem(stack.itemID, stack.count, stack.metadata)) {
                // Drop item (not implemented yet)
            }
        }
    }
}
