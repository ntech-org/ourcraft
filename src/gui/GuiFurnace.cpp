#include "gui/GuiFurnace.hpp"
#include "world/TileEntityFurnace.hpp"
#include "Minecraft.hpp"
#include "renderer/RenderEngine.hpp"
#include <SDL3/SDL.h>

GuiFurnace::GuiFurnace(TileEntityFurnace& furnace)
    : m_furnace(furnace) {
}

void GuiFurnace::initGui() {
}

void GuiFurnace::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    InventoryPlayer& inv = mc->getPlayer().inventory;

    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    renderEngine.bindTexture(renderEngine.getTexture("/gui/furnace.png"));
    drawTexturedModalRect(uiShader, left, top, 0, 0, (int)GUI_WIDTH, (int)GUI_HEIGHT);

    // Draw furnace slots
    int hoveredSlot = getFurnaceSlotFromMouse(left, top, mouseX, mouseY);
    for (int i = 0; i < 3; ++i) {
        float sx, sy;
        getFurnaceSlotPosition(left, top, i, sx, sy);
        drawStackAt(m_furnace.furnaceItemStacks[i], sx, sy, i == hoveredSlot);
    }

    // Draw player inventory
    for (int i = 0; i < 36; ++i) {
        float sx, sy;
        getSlotPosition(left, top, i, sx, sy);
        drawStackAt(inv.mainInventory[i], sx, sy, (i + 3) == hoveredSlot);
    }

    // Draw burn indicator
    if (m_furnace.isBurning()) {
        int burnScaled = m_furnace.getBurnTimeRemainingScaled(12);
        drawTexturedModalRect(uiShader, left + 56.0f, top + 36.0f + 12.0f - (float)burnScaled,
                              176, 12 - burnScaled, 14, burnScaled + 2);
    }

    // Draw cook progress arrow
    int cookScaled = m_furnace.getCookProgressScaled(24);
    drawTexturedModalRect(uiShader, left + 79.0f, top + 34.0f, 176, 14, cookScaled + 1, 16);

    // Draw text labels
    drawString(font, textShader, "Furnace", left + 60.0f, top + 6.0f, 0xFF404040);
    drawString(font, textShader, "Inventory", left + 8.0f, top + 72.0f, 0xFF404040);

    drawCursorStack(mouseX, mouseY);
}

int GuiFurnace::getFurnaceSlotFromMouse(float left, float top, int mouseX, int mouseY) const {
    const float relX = (float)mouseX - left;
    const float relY = (float)mouseY - top;

    // Furnace input slot (56, 17)
    if (relX >= 56.0f && relX < 72.0f && relY >= 17.0f && relY < 33.0f) return 0;
    // Furnace fuel slot (56, 53)
    if (relX >= 56.0f && relX < 72.0f && relY >= 53.0f && relY < 69.0f) return 1;
    // Furnace output slot (116, 35)
    if (relX >= 116.0f && relX < 132.0f && relY >= 35.0f && relY < 51.0f) return 2;

    // Main inventory 3x9 at y=84
    if (relX >= 7.0f && relX < 169.0f && relY >= 84.0f && relY < 138.0f) {
        int col = (int)((relX - 7.0f) / 18.0f);
        int row = (int)((relY - 84.0f) / 18.0f);
        if (row >= 0 && row < 3 && col >= 0 && col < 9) return 3 + (row * 9 + col);
    }

    // Hotbar at y=142
    if (relX >= 7.0f && relX < 169.0f && relY >= 142.0f && relY < 158.0f) {
        int col = (int)((relX - 7.0f) / 18.0f);
        if (col >= 0 && col < 9) return 3 + 27 + col;
    }

    return -1;
}

void GuiFurnace::getFurnaceSlotPosition(float left, float top, int slot, float& outX, float& outY) const {
    if (slot == 0) { outX = left + 56.0f; outY = top + 17.0f; }
    else if (slot == 1) { outX = left + 56.0f; outY = top + 53.0f; }
    else if (slot == 2) { outX = left + 116.0f; outY = top + 35.0f; }
}

void GuiFurnace::mouseClicked(int mouseX, int mouseY, int button) {
    float left = (width - GUI_WIDTH) * 0.5f;
    float top = (height - GUI_HEIGHT) * 0.5f;

    int slot = getFurnaceSlotFromMouse(left, top, mouseX, mouseY);
    if (slot >= 0) {
        handleFurnaceClick(slot, button == SDL_BUTTON_RIGHT);
    } else {
        handleFurnaceClick(-1, button == SDL_BUTTON_RIGHT);
    }
}

void GuiFurnace::handleFurnaceClick(int slot, bool rightClick) {
    InventoryPlayer& inv = mc->getPlayer().inventory;

    if (slot >= 0 && slot < 3) {
        // Click on furnace slot
        ItemStack& furnaceStack = m_furnace.furnaceItemStacks[slot];

        if (inv.cursorStack.isEmpty()) {
            // Pick up from furnace
            if (!furnaceStack.isEmpty()) {
                int amount = rightClick ? (furnaceStack.count + 1) / 2 : furnaceStack.count;
                inv.cursorStack = furnaceStack.splitStack(amount);
                if (furnaceStack.isEmpty()) furnaceStack = {0, 0, 0};
            }
        } else {
            // Place into furnace slot
            if (furnaceStack.isEmpty()) {
                if (slot == 2) return; // Can't place into output
                int amount = rightClick ? 1 : inv.cursorStack.count;
                furnaceStack = inv.cursorStack.splitStack(amount);
            } else if (furnaceStack.isItemEqual(inv.cursorStack)) {
                int maxAdd = 64 - furnaceStack.count;
                int amount = rightClick ? 1 : std::min(inv.cursorStack.count, maxAdd);
                if (amount > 0 && slot != 2) {
                    furnaceStack.count += amount;
                    inv.cursorStack.splitStack(amount);
                }
            }
        }
    } else if (slot >= 3) {
        // Click on player inventory slot
        int invSlot = slot - 3;
        inv.handleClick(invSlot, rightClick);
    }
}
