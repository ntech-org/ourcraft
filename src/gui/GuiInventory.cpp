#include "gui/GuiInventory.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>

int GuiInventory::getSlotFromMouse(float left, float top, int mouseX, int mouseY) const {
    auto inGrid = [&](float sx, float sy, int rows, int startSlot) -> int {
        const float relX = (float)mouseX - sx;
        const float relY = (float)mouseY - sy;
        if (relX < 0.0f || relY < 0.0f) return -1;
        const int col = (int)(relX / 18.0f);
        const int row = (int)(relY / 18.0f);
        if (col < 0 || col >= 9 || row < 0 || row >= rows) return -1;
        return startSlot + row * 9 + col;
    };

    const int mainSlot = inGrid(left + 8.0f, top + 84.0f, 3, 9);
    if (mainSlot >= 9 && mainSlot < InventoryPlayer::INVENTORY_SIZE) return mainSlot;

    const int hotbarSlot = inGrid(left + 8.0f, top + 142.0f, 1, 0);
    if (hotbarSlot >= 0 && hotbarSlot < 9) return hotbarSlot;
    return -1;
}

void GuiInventory::getSlotPosition(float left, float top, int slot, float& outX, float& outY) const {
    const int col = slot % 9;
    if (slot < 9) {
        outX = left + 8.0f + (float)col * 18.0f;
        outY = top + 142.0f;
    } else {
        const int row = (slot - 9) / 9;
        outX = left + 8.0f + (float)col * 18.0f;
        outY = top + 84.0f + (float)row * 18.0f;
    }
}

void GuiInventory::handleClickOnSlot(InventoryPlayer& inv, int slot, bool rightClick) {
    if (slot < 0 || slot >= InventoryPlayer::INVENTORY_SIZE) return;
    ItemStack& target = inv.mainInventory[slot];

    const bool cursorEmpty = m_cursorStack.isEmpty();
    const bool targetEmpty = target.isEmpty();

    if (rightClick) {
        if (cursorEmpty) {
            if (targetEmpty) return;
            const int take = (target.count + 1) / 2;
            m_cursorStack = target;
            m_cursorStack.count = take;
            target.count -= take;
            if (target.count <= 0) target = {0, 0, 0};
            return;
        }

        if (targetEmpty) {
            target = m_cursorStack;
            target.count = 1;
            m_cursorStack.count -= 1;
            if (m_cursorStack.count <= 0) m_cursorStack = {0, 0, 0};
            return;
        }

        if (target.itemID == m_cursorStack.itemID &&
            target.metadata == m_cursorStack.metadata &&
            target.count < InventoryPlayer::MAX_STACK_SIZE) {
            target.count += 1;
            m_cursorStack.count -= 1;
            if (m_cursorStack.count <= 0) m_cursorStack = {0, 0, 0};
        }
        return;
    }

    if (cursorEmpty) {
        if (targetEmpty) return;
        m_cursorStack = target;
        target = {0, 0, 0};
        return;
    }

    if (targetEmpty) {
        target = m_cursorStack;
        m_cursorStack = {0, 0, 0};
        return;
    }

    if (target.itemID == m_cursorStack.itemID && target.metadata == m_cursorStack.metadata) {
        const int free = InventoryPlayer::MAX_STACK_SIZE - target.count;
        const int moved = std::min(free, m_cursorStack.count);
        target.count += moved;
        m_cursorStack.count -= moved;
        if (m_cursorStack.count <= 0) m_cursorStack = {0, 0, 0};
        return;
    }

    std::swap(target, m_cursorStack);
}

void GuiInventory::handleDragDistribution(InventoryPlayer& inv, int slot) {
    if (!m_draggingLeft || slot < 0 || slot >= InventoryPlayer::INVENTORY_SIZE || m_cursorStack.isEmpty()) return;
    if (m_dragVisited[slot]) return;
    m_dragVisited[slot] = true;

    ItemStack& target = inv.mainInventory[slot];
    if (target.isEmpty()) {
        target = m_cursorStack;
        target.count = 1;
        m_cursorStack.count -= 1;
    } else if (target.itemID == m_cursorStack.itemID &&
               target.metadata == m_cursorStack.metadata &&
               target.count < InventoryPlayer::MAX_STACK_SIZE) {
        target.count += 1;
        m_cursorStack.count -= 1;
    }

    if (m_cursorStack.count <= 0) {
        m_cursorStack = {0, 0, 0};
        m_draggingLeft = false;
    }
}

void GuiInventory::drawBlockStack3D(int blockID, float x, float y) {
    const Block* block = Block::blocksList[blockID];
    if (!block) return;

    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    renderEngine.bindTexture(renderEngine.getTexture("/terrain.png"));
    Tessellator* t = Tessellator::instance;

    auto tileUV = [](int tex, float& u0, float& v0, float& u1, float& v1) {
        u0 = (float)((tex & 15) * 16) / 256.0f;
        v0 = (float)((tex >> 4) * 16) / 256.0f;
        u1 = u0 + 16.0f / 256.0f;
        v1 = v0 + 16.0f / 256.0f;
    };

    const int texTop = block->getTexture(1);
    const int texSide = block->getTexture(2);
    float u0, v0, u1, v1;

    // Shift Y slightly up to center the 13px tall isometric block in 16px slot
    y -= 1.5f;

    t->startDrawingQuads();
    tileUV(texTop, u0, v0, u1, v1);
    t->setColorRGBA(230, 230, 230, 255);
    t->addVertexWithUV(x + 2.0f, y + 6.0f, 0.0f, u0, v1);
    t->addVertexWithUV(x + 8.0f, y + 3.0f, 0.0f, u1, v1);
    t->addVertexWithUV(x + 14.0f, y + 6.0f, 0.0f, u1, v0);
    t->addVertexWithUV(x + 8.0f, y + 9.0f, 0.0f, u0, v0);

    tileUV(texSide, u0, v0, u1, v1);
    t->setColorRGBA(170, 170, 170, 255);
    t->addVertexWithUV(x + 2.0f, y + 6.0f, 0.0f, u0, v0);
    t->addVertexWithUV(x + 8.0f, y + 9.0f, 0.0f, u1, v0);
    t->addVertexWithUV(x + 8.0f, y + 16.0f, 0.0f, u1, v1);
    t->addVertexWithUV(x + 2.0f, y + 13.0f, 0.0f, u0, v1);

    t->setColorRGBA(200, 200, 200, 255);
    t->addVertexWithUV(x + 8.0f, y + 9.0f, 0.0f, u0, v0);
    t->addVertexWithUV(x + 14.0f, y + 6.0f, 0.0f, u1, v0);
    t->addVertexWithUV(x + 14.0f, y + 13.0f, 0.0f, u1, v1);
    t->addVertexWithUV(x + 8.0f, y + 16.0f, 0.0f, u0, v1);
    t->draw();
}

void GuiInventory::drawBlockStack2D(int blockID, float x, float y) {
    const Block* block = Block::blocksList[blockID];
    if (!block) return;
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    renderEngine.bindTexture(renderEngine.getTexture("/terrain.png"));
    const int tex = block->getTexture(0);
    drawTexturedModalRect(mc->getGameRenderer().getUIShader(), x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
}

void GuiInventory::drawItemStack2D(int itemID, float x, float y) {
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    renderEngine.bindTexture(renderEngine.getTexture("/gui/items.png"));
    const int tex = itemID & 255;
    drawTexturedModalRect(mc->getGameRenderer().getUIShader(), x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
}

void GuiInventory::drawStackAt(const ItemStack& stack, float x, float y, bool highlight) {
    Shader& shader = mc->getGameRenderer().getUIShader();
    FontRenderer& font = mc->getGameRenderer().getFontRenderer();

    if (highlight) {
        drawRect(shader, x - 1.0f, y - 1.0f, x + 17.0f, y + 17.0f, 0x70FFFFFF);
    }

    if (stack.isEmpty()) return;

    if (stack.itemID > 0 && Block::blocksList[stack.itemID]) {
        if (Block::blocksList[stack.itemID]->getRenderShape() == BlockRenderShape::Cross) {
            drawBlockStack2D(stack.itemID, x, y);
        } else {
            drawBlockStack3D(stack.itemID, x, y);
        }
    } else {
        drawItemStack2D(stack.itemID, x, y);
    }

    if (stack.count > 1) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", stack.count);
        font.drawStringWithShadow(shader, buf, x + 19.0f - font.getStringWidth(buf), y + 9.0f, 0xFFFFFFFF);
    }
}

void GuiInventory::drawInventorySlots(float left, float top, int mouseX, int mouseY) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    const int hoveredSlot = getSlotFromMouse(left, top, mouseX, mouseY);
    for (int slot = 0; slot < InventoryPlayer::INVENTORY_SIZE; ++slot) {
        float sx, sy;
        getSlotPosition(left, top, slot, sx, sy);
        const bool selectedHotbar = slot == inv.currentSlot;
        const bool hovered = slot == hoveredSlot;
        drawStackAt(inv.mainInventory[slot], sx, sy, selectedHotbar || hovered);
    }
}

void GuiInventory::drawCursorStack(int mouseX, int mouseY) {
    if (m_cursorStack.isEmpty()) return;
    drawStackAt(m_cursorStack, (float)mouseX - 8.0f, (float)mouseY - 8.0f, false);
}

void GuiInventory::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& shader = mc->getGameRenderer().getUIShader();
    FontRenderer& font = mc->getGameRenderer().getFontRenderer();
    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();

    const float left = (width - GUI_WIDTH) * 0.5f;
    const float top = (height - GUI_HEIGHT) * 0.5f;

    renderEngine.bindTexture(renderEngine.getTexture("/gui/inventory.png"));
    drawTexturedModalRect(shader, left, top, 0, 0, (int)GUI_WIDTH, (int)GUI_HEIGHT);

    InventoryPlayer& inv = mc->getPlayer().inventory;
    GLFWwindow* window = glfwGetCurrentContext();
    const bool leftDown = window && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftDown && m_draggingLeft && !m_cursorStack.isEmpty()) {
        const int slot = getSlotFromMouse(left, top, mouseX, mouseY);
        handleDragDistribution(inv, slot);
    }
    if (!leftDown) {
        m_draggingLeft = false;
        m_dragVisited.fill(false);
    }

    drawInventorySlots(left, top, mouseX, mouseY);
    drawCursorStack(mouseX, mouseY);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiInventory::keyTyped(int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_E || key == GLFW_KEY_ESCAPE) {
        mc->displayGuiScreen(nullptr);
        return;
    }
    GuiScreen::keyTyped(key, scancode, action, mods);
}

void GuiInventory::mouseClicked(int mouseX, int mouseY, int button) {
    InventoryPlayer& inv = mc->getPlayer().inventory;
    const float left = (width - GUI_WIDTH) * 0.5f;
    const float top = (height - GUI_HEIGHT) * 0.5f;

    const int slot = getSlotFromMouse(left, top, mouseX, mouseY);
    if (slot >= 0) {
        handleClickOnSlot(inv, slot, button == GLFW_MOUSE_BUTTON_RIGHT);
        if (button == GLFW_MOUSE_BUTTON_LEFT && !m_cursorStack.isEmpty()) {
            m_draggingLeft = true;
            m_dragVisited.fill(false);
            m_dragVisited[slot] = true;
        }
    } else {
        if (!m_cursorStack.isEmpty()) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                m_cursorStack.count -= 1;
                if (m_cursorStack.count <= 0) m_cursorStack = {0, 0, 0};
            } else {
                m_cursorStack = {0, 0, 0};
            }
        }
    }

    GuiScreen::mouseClicked(mouseX, mouseY, button);
}
