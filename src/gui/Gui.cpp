#include "gui/Gui.hpp"
#include "renderer/Tessellator.hpp"

#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"

void Gui::drawRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color) {
    if (x1 < x2) { float temp = x1; x1 = x2; x2 = temp; }
    if (y1 < y2) { float temp = y1; y1 = y2; y2 = temp; }

    float a = (float)(color >> 24 & 255) / 255.0f;
    float r = (float)(color >> 16 & 255) / 255.0f;
    float g = (float)(color >> 8 & 255) / 255.0f;
    float b = (float)(color & 255) / 255.0f;

    Tessellator* t = Tessellator::instance;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    shader.setBool("hasTexture", false);

    t->startDrawingQuads();
    t->setColorRGBA((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
    t->addVertex(x1, y2, 0.0);
    t->addVertex(x2, y2, 0.0);
    t->addVertex(x2, y1, 0.0);
    t->addVertex(x1, y1, 0.0);
    t->draw();

    shader.setBool("hasTexture", true);
    glDisable(GL_BLEND);
}

void Gui::drawGradientRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color1, uint32_t color2) {
    float a1 = (float)(color1 >> 24 & 255) / 255.0f;
    float r1 = (float)(color1 >> 16 & 255) / 255.0f;
    float g1 = (float)(color1 >> 8 & 255) / 255.0f;
    float b1 = (float)(color1 & 255) / 255.0f;

    float a2 = (float)(color2 >> 24 & 255) / 255.0f;
    float r2 = (float)(color2 >> 16 & 255) / 255.0f;
    float g2 = (float)(color2 >> 8 & 255) / 255.0f;
    float b2 = (float)(color2 & 255) / 255.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    shader.setBool("hasTexture", false);

    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();
    t->setColorRGBA((int)(r1 * 255), (int)(g1 * 255), (int)(b1 * 255), (int)(a1 * 255));
    t->addVertex(x2, y1, 0.0);
    t->addVertex(x1, y1, 0.0);
    t->setColorRGBA((int)(r2 * 255), (int)(g2 * 255), (int)(b2 * 255), (int)(a2 * 255));
    t->addVertex(x1, y2, 0.0);
    t->addVertex(x2, y2, 0.0);
    t->draw();

    shader.setBool("hasTexture", true);
    glDisable(GL_BLEND);
}

void Gui::drawCenteredString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    font.drawString(shader, text, x - (float)font.getStringWidth(text) / 2.0f, y, color, true);
}

void Gui::drawString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    font.drawString(shader, text, x, y, color, true);
}


void Gui::drawTexturedModalRect(Shader& shader, float x, float y, int u, int v, int width, int height) {
    float f = 0.00390625f; // 1/256
    Tessellator* t = Tessellator::instance;

    shader.use();
    shader.setBool("hasTexture", true);

    t->startDrawingQuads();
    t->setColorOpaque_I(0xFFFFFFFF);
    t->addVertexWithUV(x, y + (float)height, 0.0, (float)u * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y + (float)height, 0.0, (float)(u + width) * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y, 0.0, (float)(u + width) * f, (float)v * f);
    t->addVertexWithUV(x, y, 0.0, (float)u * f, (float)v * f);
    t->draw();
}

void Gui::drawItemStack(Minecraft* mc, const ItemStack& stack, float x, float y) {
    if (stack.isEmpty()) return;

    if (stack.itemID < 256 && Block::blocksList[stack.itemID]) {
        if (Block::blocksList[stack.itemID]->getRenderShape() == BlockRenderShape::Cross) {
            drawBlockStack2D(mc, stack.itemID, x, y);
        } else {
            drawBlockStack3D(mc, stack.itemID, x, y);
        }
    } else {
        drawItemIcon2D(mc, stack.itemID, x, y);
    }

    if (stack.count > 1) {
        std::string countStr = std::to_string(stack.count);
        mc->getFont().drawString(mc->getGameRenderer().getTextShader(), countStr, x + 17 - mc->getFont().getStringWidth(countStr), y + 9, 0xffffff, true);
    }
}

void Gui::drawBlockStack3D(Minecraft* mc, int blockID, float x, float y) {
    const Block* block = Block::blocksList[blockID];
    if (!block) return;

    RenderEngine& renderEngine = mc->getGameRenderer().getRenderEngine();
    renderEngine.bindTexture(renderEngine.getTexture(TEX_TERRAIN));

    auto tileUV = [](int tex, float& u0, float& v0, float& u1, float& v1) {
        u0 = (float)((tex & 15) * 16) / 256.0f;
        v0 = (float)((tex >> 4) * 16) / 256.0f;
        u1 = u0 + 16.0f / 256.0f;
        v1 = v0 + 16.0f / 256.0f;
    };

    float u0, v0, u1, v1;
    const int texTop = block->getTexture(1);
    const int texSide = block->getTexture(2);

    float s = 1.2f; // Adjusted scale to fit better within the slot without being "too big"
    float ox = x + 8.0f;
    float oy = y + 9.0f; // Nudged up from 10.0f

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    uiShader.use();
    uiShader.setBool("hasTexture", true);

    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();

    // Top
    tileUV(texTop, u0, v0, u1, v1);
    t->setColorRGBA(230, 230, 230, 255);
    t->addVertexWithUV(ox - 6.0f * s, oy - 4.0f * s, 0.0, u0, v1);
    t->addVertexWithUV(ox, oy - 7.0f * s, 0.0, u1, v1);
    t->addVertexWithUV(ox + 6.0f * s, oy - 4.0f * s, 0.0, u1, v0);
    t->addVertexWithUV(ox, oy - 1.0f * s, 0.0, u0, v0);

    // Side 1
    tileUV(texSide, u0, v0, u1, v1);
    t->setColorRGBA(170, 170, 170, 255);
    t->addVertexWithUV(ox - 6.0f * s, oy - 4.0f * s, 0.0, u0, v0);
    t->addVertexWithUV(ox, oy - 1.0f * s, 0.0, u1, v0);
    t->addVertexWithUV(ox, oy + 6.0f * s, 0.0, u1, v1);
    t->addVertexWithUV(ox - 6.0f * s, oy + 3.0f * s, 0.0, u0, v1);

    // Side 2
    t->setColorRGBA(200, 200, 200, 255);
    t->addVertexWithUV(ox, oy - 1.0f * s, 0.0, u0, v0);
    t->addVertexWithUV(ox + 6.0f * s, oy - 4.0f * s, 0.0, u1, v0);
    t->addVertexWithUV(ox + 6.0f * s, oy + 3.0f * s, 0.0, u1, v1);
    t->addVertexWithUV(ox, oy + 6.0f * s, 0.0, u0, v1);

    t->draw();
}

void Gui::drawBlockStack2D(Minecraft* mc, int blockID, float x, float y) {
    const Block* block = Block::blocksList[blockID];
    if (!block) return;
    mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture(TEX_TERRAIN));
    const int tex = block->getTexture(0);
    // Use 16x16 with slight offset to center in 18x18 if needed, but here we use the provided x,y
    drawTexturedModalRect(mc->getGameRenderer().getUIShader(), x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
}

void Gui::drawItemIcon2D(Minecraft* mc, int itemID, float x, float y) {
    mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture(TEX_ITEMS));
    int tex = 0;
    if (itemID >= 0 && itemID < 1024 && Item::itemsList[itemID]) {
        tex = Item::itemsList[itemID]->iconIndex;
    } else {
        tex = itemID & 255;
    }
    drawTexturedModalRect(mc->getGameRenderer().getUIShader(), x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
}
