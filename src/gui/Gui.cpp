#include "gui/Gui.hpp"
#include "renderer/Tessellator.hpp"

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

void Gui::drawCenteredString(FontRenderer& fontRenderer, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    fontRenderer.drawString(shader, text, x - (float)fontRenderer.getStringWidth(text) / 2.0f, y, color);
}

void Gui::drawString(FontRenderer& fontRenderer, Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    fontRenderer.drawString(shader, text, x, y, color);
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
