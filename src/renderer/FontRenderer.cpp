#include "renderer/FontRenderer.hpp"
#include "renderer/Tessellator.hpp"

FontRenderer::FontRenderer(RenderEngine* renderEngine, const std::string& fontPath) 
    : m_renderEngine(renderEngine) {
    m_fontTexture = m_renderEngine->getTexture(fontPath);
}

void FontRenderer::drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    m_renderEngine->bindTexture(m_fontTexture);
    
    int r = (color >> 16) & 255;
    int g = (color >> 8) & 255;
    int b = color & 255;
    int a = (color >> 24) & 255;
    if (a == 0) a = 255;

    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();
    t->setColorRGBA(r, g, b, a);

    float cx = x;
    float cy = y;

    for (char c : text) {
        if (c == '\n') {
            cx = x;
            cy += 10.0f; // newline spacing
            continue;
        }

        int charIdx = (unsigned char)c;
        int col = charIdx % 16;
        int row = charIdx / 16;

        float u0 = col / 16.0f;
        float u1 = (col + 1) / 16.0f;
        float v0 = row / 16.0f;
        float v1 = (row + 1) / 16.0f;

        float w = 8.0f;
        float h = 8.0f;

        // Quad: (cx, cy) to (cx+w, cy+h)
        t->addVertexWithUV(cx, cy + h, 0, u0, v1);
        t->addVertexWithUV(cx + w, cy + h, 0, u1, v1);
        t->addVertexWithUV(cx + w, cy, 0, u1, v0);
        t->addVertexWithUV(cx, cy, 0, u0, v0);

        cx += 8.0f; // advance cursor
    }
    t->draw();
}

int FontRenderer::getStringWidth(const std::string& text) {
    // Basic implementation: every char is 8 pixels wide.
    // In original MC it samples char widths from the texture.
    return (int)text.length() * 8;
}
