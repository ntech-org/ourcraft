#include "renderer/FontRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "util/UTF8.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>

FontRenderer::FontRenderer(RenderEngine* renderEngine, const std::string& fontPath)
    : m_renderEngine(renderEngine) {

    if (FT_Init_FreeType(&m_ft)) {
        std::cerr << "Could not init freetype library" << std::endl;
        return;
    }

    auto loadFile = [](const std::string& p) -> std::vector<unsigned char> {
        std::string path = "assets/" + p;
        if (p[0] == '/') path = "assets" + p;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> buffer(size);
        file.read((char*)buffer.data(), size);
        return buffer;
    };

    m_primaryFontData = loadFile(fontPath);
    m_fallbackFontData = loadFile("unifont.otf");

    if (FT_New_Memory_Face(m_ft, m_primaryFontData.data(), (FT_Long)m_primaryFontData.size(), 0, &m_face)) {
        std::cerr << "Failed to load primary font: " << fontPath << std::endl;
    }
    if (FT_New_Memory_Face(m_ft, m_fallbackFontData.data(), (FT_Long)m_fallbackFontData.size(), 0, &m_fallbackFace)) {
        std::cerr << "Failed to load fallback font: unifont.otf" << std::endl;
    }

    updateFontSizes();

    // Create empty atlas texture
    std::vector<unsigned char> emptyData(m_atlasWidth * m_atlasHeight * 4, 0);
    m_fontTexture = m_renderEngine->createTexture(m_atlasWidth, m_atlasHeight, emptyData.data());

    // Initialize color table
    for (int i = 0; i < 32; ++i) {
        int brightness = (i >> 3 & 1) * 85;
        int r = (i >> 2 & 1) * 170 + brightness;
        int g = (i >> 1 & 1) * 170 + brightness;
        int b = (i & 1) * 170 + brightness;
        if (i == 6) r += 85;

        if (i >= 16) {
            r /= 4; g /= 4; b /= 4;
        }
        m_colorTable[i] = (255 << 24) | (r << 16) | (g << 8) | b;
    }
}

FontRenderer::~FontRenderer() {
    FT_Done_Face(m_face);
    if (m_fallbackFace) FT_Done_Face(m_fallbackFace);
    FT_Done_FreeType(m_ft);
}

void FontRenderer::setGuiScale(int scale) {
    if (m_guiScale == scale) return;
    m_guiScale = scale;

    updateFontSizes();
    m_glyphCache.clear();
    m_nextX = 1;
    m_nextY = 1;
    m_maxHeight = 0;

    // Clear atlas texture
    std::vector<unsigned char> emptyData(m_atlasWidth * m_atlasHeight * 4, 0);
    m_renderEngine->bindTexture(m_fontTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_atlasWidth, m_atlasHeight, GL_RGBA, GL_UNSIGNED_BYTE, emptyData.data());
}

void FontRenderer::updateFontSizes() {
    // Render at full physical resolution relative to screen pixels
    // logical 8px * guiScale = physical size on screen
    int physicalSize = (int)(8.0f * m_guiScale);
    FT_Set_Pixel_Sizes(m_face, 0, physicalSize);
    if (m_fallbackFace) FT_Set_Pixel_Sizes(m_fallbackFace, 0, physicalSize);
}

const FontRenderer::GlyphInfo& FontRenderer::getGlyph(char32_t codePoint) {
    if (m_glyphCache.find(codePoint) != m_glyphCache.end()) {
        return m_glyphCache[codePoint];
    }

    GlyphInfo info;
    loadGlyphToAtlas(codePoint, info);
    m_glyphCache[codePoint] = info;
    return m_glyphCache[codePoint];
}

void FontRenderer::loadGlyphToAtlas(char32_t codePoint, GlyphInfo& info) {
    FT_Face face = m_face;
    FT_UInt glyphIndex = FT_Get_Char_Index(face, codePoint);

    if (glyphIndex == 0 && m_fallbackFace) {
        face = m_fallbackFace;
        glyphIndex = FT_Get_Char_Index(face, codePoint);
    }

    // Load with anti-aliasing (since we are at full resolution)
    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER)) {
        if (codePoint != '?') {
            loadGlyphToAtlas('?', info);
            return;
        }
        return;
    }

    FT_Bitmap& bitmap = face->glyph->bitmap;
    int w = bitmap.width;
    int h = bitmap.rows;

    int padding = 2;
    if (m_nextX + w + padding >= m_atlasWidth) {
        m_nextX = padding;
        m_nextY += m_maxHeight + padding;
        m_maxHeight = 0;
    }

    if (m_nextY + h + padding >= m_atlasHeight) {
        std::cerr << "Font atlas full!" << std::endl;
        return;
    }

    // Threshold or just use alpha? For "blocky" feel at high res, threshold is safer.
    std::vector<unsigned char> rgba(w * h * 4, 0);


    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char alpha = 0;

            if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                // High threshold to ensure crisp block edges
                alpha = (bitmap.buffer[y * bitmap.pitch + x] > 120) ? 255 : 0;
            } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                alpha = (bitmap.buffer[y * bitmap.pitch + (x >> 3)] & (0x80 >> (x & 7))) ? 255 : 0;
            }
            if (alpha > 0) {
                int idx = (y * w + x) * 4;
                rgba[idx + 0] = 255;
                rgba[idx + 1] = 255;
                rgba[idx + 2] = 255;
                rgba[idx + 3] = 255;
            }
        }
    }

    m_renderEngine->bindTexture(m_fontTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, m_nextX, m_nextY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    // Scale back to logical coordinates for the GUI
    float invScale = 1.0f / (float)m_guiScale;
    info.x0 = (float)m_nextX / (float)m_atlasWidth;
    info.y0 = (float)m_nextY / (float)m_atlasHeight;
    info.x1 = (float)(m_nextX + w) / (float)m_atlasWidth;
    info.y1 = (float)(m_nextY + h) / (float)m_atlasHeight;
    info.offX = (float)face->glyph->bitmap_left * invScale;
    info.offY = (float)-face->glyph->bitmap_top * invScale;
    info.advance = ((float)face->glyph->advance.x / 64.0f) * invScale;
    info.w = w;
    info.h = h;

    // Logical space is 4
    if (codePoint == ' ') info.advance = 4.0f;

    m_nextX += w + padding;
    m_maxHeight = std::max(m_maxHeight, h);
}

void FontRenderer::drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    m_renderEngine->bindTexture(m_fontTexture);

    float r = (float)((color >> 16) & 255) / 255.0f;
    float g = (float)((color >> 8) & 255) / 255.0f;
    float b = (float)(color & 255) / 255.0f;
    float a = (float)((color >> 24) & 255) / 255.0f;
    if (a == 0) a = 1.0f;

    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();
    t->setColorRGBA((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));

    // cx, cy are in logical GUI coordinates.
    float cx = std::floor(x);
    float cy = std::floor(y) + 7.0f;

    std::u32string u32text = UTF8::toUTF32(text);
    float invScale = 1.0f / (float)m_guiScale;

    for (size_t i = 0; i < u32text.length(); ++i) {
        char32_t c = u32text[i];

        if (c == '&' && i + 1 < u32text.length()) {
            std::string codes = "0123456789abcdef";
            size_t codeIdx = codes.find((char)u32text[i + 1]);
            if (codeIdx != std::string::npos) {
                uint32_t col = m_colorTable[codeIdx];
                t->setColorRGBA((col >> 16) & 255, (col >> 8) & 255, col & 255, (int)(a * 255));
                i++;
                continue;
            }
        }

        if (c == '\n') {
            cx = std::floor(x);
            cy += 10.0f;
            continue;
        }

        const auto& gInfo = getGlyph(c);

        // Calculate positions and snap them to the physical grid for maximum sharpness.
        // We do this by going to physical space, flooring, and coming back.
        float vx0 = std::floor((cx + gInfo.offX) * m_guiScale) * invScale;
        float vy0 = std::floor((cy + gInfo.offY) * m_guiScale) * invScale;
        float vx1 = vx0 + (float)gInfo.w * invScale;
        float vy1 = vy0 + (float)gInfo.h * invScale;

        t->addVertexWithUV(vx0, vy1, 0, gInfo.x0, gInfo.y1);
        t->addVertexWithUV(vx1, vy1, 0, gInfo.x1, gInfo.y1);
        t->addVertexWithUV(vx1, vy0, 0, gInfo.x1, gInfo.y0);
        t->addVertexWithUV(vx0, vy0, 0, gInfo.x0, gInfo.y0);

        cx += gInfo.advance;
    }
    t->draw();
}

void FontRenderer::drawStringWithShadow(Shader& shader, const std::string& text, float x, float y, uint32_t color) {
    uint32_t shadowColor = (color & 0xFF000000) | ((color & 0xFCFCFC) >> 2);
    if (shadowColor == (color & 0xFF000000)) shadowColor |= 0x003F3F3F;

    float shadowOff = 1.0f; // 1 logical unit
    drawString(shader, text, x + shadowOff, y + shadowOff, shadowColor);
    drawString(shader, text, x, y, color);
}

int FontRenderer::getStringWidth(const std::string& text) {
    float width = 0;
    std::u32string u32text = UTF8::toUTF32(text);
    for (size_t i = 0; i < u32text.length(); ++i) {
        char32_t c = u32text[i];
        if (c == '&' && i + 1 < u32text.length()) {
            i++;
            continue;
        }
        const auto& gInfo = getGlyph(c);
        width += gInfo.advance;
    }
    return (int)std::ceil(width);
}
