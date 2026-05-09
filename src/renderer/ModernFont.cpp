#include "renderer/ModernFont.hpp"
#include "renderer/Tessellator.hpp"
#include "util/UTF8.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>

// --- FreeTypeGlyphProvider ---

FreeTypeGlyphProvider::FreeTypeGlyphProvider(FT_Library ft, std::vector<uint8_t> fontData, float logicalSize)
    : m_fontData(std::move(fontData)), m_logicalSize(logicalSize) {
    if (FT_New_Memory_Face(ft, m_fontData.data(), (FT_Long)m_fontData.size(), 0, &m_face)) {
        std::cerr << "[Font] Failed to load font face" << std::endl;
    }
}

FreeTypeGlyphProvider::~FreeTypeGlyphProvider() {
    if (m_face) FT_Done_Face(m_face);
}

void FreeTypeGlyphProvider::setScale(float guiScale) {
    m_currentScale = guiScale;
    FT_Set_Pixel_Sizes(m_face, 0, (FT_UInt)std::round(m_logicalSize * guiScale));
}

bool FreeTypeGlyphProvider::getGlyphInfo(uint32_t codePoint, GlyphInfo& outInfo) {
    FT_UInt index = FT_Get_Char_Index(m_face, codePoint);
    if (index == 0) return false;

    // Load with hinting enabled
    if (FT_Load_Glyph(m_face, index, FT_LOAD_DEFAULT)) return false;

    FT_GlyphSlot slot = m_face->glyph;
    outInfo.width = slot->bitmap.width;
    outInfo.height = slot->bitmap.rows;
    outInfo.bearingX = (float)slot->bitmap_left / m_currentScale;
    outInfo.bearingY = (float)-slot->bitmap_top / m_currentScale;
    outInfo.advance = (float)(slot->advance.x >> 6) / m_currentScale;
    return true;
}

void FreeTypeGlyphProvider::rasterizeGlyph(uint32_t codePoint, std::vector<uint8_t>& outBuffer, int& outW, int& outH) {
    FT_UInt index = FT_Get_Char_Index(m_face, codePoint);
    if (FT_Load_Glyph(m_face, index, FT_LOAD_DEFAULT)) return;
    if (FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL)) return;

    FT_Bitmap& bitmap = m_face->glyph->bitmap;
    outW = bitmap.width;
    outH = bitmap.rows;
    outBuffer.resize(outW * outH);

    for (int y = 0; y < outH; ++y) {
        for (int x = 0; x < outW; ++x) {
            outBuffer[y * outW + x] = bitmap.buffer[y * bitmap.pitch + x];
        }
    }
}

// --- FontSet ---

FontSet::FontSet(RenderEngine* renderEngine, float logicalSize) 
    : m_renderEngine(renderEngine), m_logicalSize(logicalSize) {
}

FontSet::~FontSet() {
    if (m_textureID) glDeleteTextures(1, (GLuint*)&m_textureID);
}

void FontSet::setScale(float guiScale) {
    if (std::abs(m_currentScale - guiScale) < 0.01f && m_textureID != 0) return;
    
    m_currentScale = guiScale;
    for (auto& provider : m_providers) {
        provider->setScale(guiScale);
    }
    clearCache();
}

void FontSet::addProvider(std::unique_ptr<FreeTypeGlyphProvider> provider) {
    provider->setScale(m_currentScale);
    m_providers.push_back(std::move(provider));
}

void FontSet::clearCache() {
    m_glyphs.clear();
    if (m_textureID) {
        glDeleteTextures(1, (GLuint*)&m_textureID);
        m_textureID = 0;
    }
    m_currentX = 1;
    m_currentY = 1;
    m_maxRowHeight = 0;

    std::vector<uint8_t> emptyData(m_atlasWidth * m_atlasHeight * 4, 0);
    m_textureID = m_renderEngine->createTexture(m_atlasWidth, m_atlasHeight, emptyData.data());
    
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    // Use Linear filtering for standard high-quality rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

const GlyphInfo& FontSet::getGlyph(uint32_t codePoint) {
    auto it = m_glyphs.find(codePoint);
    if (it != m_glyphs.end()) return it->second;

    if (!m_textureID) clearCache();

    for (auto& provider : m_providers) {
        GlyphInfo info;
        if (provider->getGlyphInfo(codePoint, info)) {
            std::vector<uint8_t> bitmap;
            int w, h;
            provider->rasterizeGlyph(codePoint, bitmap, w, h);
            
            if (m_currentX + w + 1 >= m_atlasWidth) {
                m_currentX = 1;
                m_currentY += m_maxRowHeight + 1;
                m_maxRowHeight = 0;
            }
            if (m_currentY + h + 1 >= m_atlasHeight) {
                std::cerr << "[FontSet] Atlas full!" << std::endl;
                return m_glyphs[63];
            }

            uploadGlyph(codePoint, info, bitmap);
            return m_glyphs[codePoint];
        }
    }

    if (codePoint != 63) return getGlyph(63);
    static GlyphInfo empty{};
    return empty;
}

void FontSet::uploadGlyph(uint32_t codePoint, const GlyphInfo& info, const std::vector<uint8_t>& bitmap) {
    int w = info.width;
    int h = info.height;

    std::vector<uint32_t> rgba(w * h);
    for (int i = 0; i < w * h; ++i) {
        uint32_t alpha = bitmap[i];
        rgba[i] = (alpha << 24) | 0xFFFFFF;
    }

    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, m_currentX, m_currentY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    GlyphInfo finalInfo = info;
    finalInfo.u0 = (float)m_currentX / (float)m_atlasWidth;
    finalInfo.v0 = (float)m_currentY / (float)m_atlasHeight;
    finalInfo.u1 = (float)(m_currentX + w) / (float)m_atlasWidth;
    finalInfo.v1 = (float)(m_currentY + h) / (float)m_atlasHeight;

    m_glyphs[codePoint] = finalInfo;

    m_currentX += w + 2;
    m_maxRowHeight = std::max(m_maxRowHeight, h);
}

// --- Font ---

Font::Font(std::unique_ptr<FontSet> fontSet) : m_fontSet(std::move(fontSet)) {
    for (int i = 0; i < 32; ++i) {
        int brightness = (i >> 3 & 1) * 85;
        int r = (i >> 2 & 1) * 170 + brightness;
        int g = (i >> 1 & 1) * 170 + brightness;
        int b = (i & 1) * 170 + brightness;
        if (i == 6) r += 85;
        if (i >= 16) { r /= 4; g /= 4; b /= 4; }
        m_colorTable[i] = (255 << 24) | (r << 16) | (g << 8) | b;
    }
}

void Font::setDisplayContext(int screenW, int screenH, float guiScale) {
    m_screenW = screenW;
    m_screenH = screenH;
    m_guiScale = guiScale;
    m_fontSet->setScale(guiScale);
}

void Font::drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color, bool shadow) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    shader.setBool("hasTexture", true);

    if (m_screenW > 0 && m_screenH > 0) {
        shader.setMat4("projection", glm::ortho(0.0f, (float)m_screenW, (float)m_screenH, 0.0f, -1.0f, 1.0f));
        shader.setMat4("view", glm::mat4(1.0f));
    }

    glBindTexture(GL_TEXTURE_2D, m_fontSet->getTextureID());
    Tessellator* t = Tessellator::instance;
    
    std::u32string u32text = UTF8::toUTF32(text);
    float drawScale = m_guiScale;
    
    // Snapping the starting position once
    float startX = std::floor(x * drawScale);
    float startY = std::floor(y * drawScale) + std::floor(7.0f * drawScale);

    auto renderPass = [&](uint32_t passColor, float offX, float offY, bool isShadow) {
        float curR = (float)((passColor >> 16) & 0xFF) / 255.0f;
        float curG = (float)((passColor >> 8) & 0xFF) / 255.0f;
        float curB = (float)((passColor >> 0) & 0xFF) / 255.0f;
        float curA = (float)((passColor >> 24) & 0xFF) / 255.0f;
        if (curA == 0.0f) curA = 1.0f;

        t->startDrawingQuads();
        t->setColorRGBA((int)(curR * 255), (int)(curG * 255), (int)(curB * 255), (int)(curA * 255));

        float cx = startX + offX;
        float cy = startY + offY;

        for (size_t i = 0; i < u32text.length(); ++i) {
            uint32_t c = u32text[i];

            if (c == '&' && i + 1 < u32text.length()) {
                std::string codes = "0123456789abcdef";
                size_t codeIdx = codes.find((char)u32text[i + 1]);
                if (codeIdx != std::string::npos) {
                    uint32_t col = m_colorTable[codeIdx];
                    if (isShadow) {
                        col = (col & 0xFF000000) | ((col & 0xFCFCFC) >> 2);
                    }
                    curR = (float)((col >> 16) & 0xFF) / 255.0f;
                    curG = (float)((col >> 8) & 0xFF) / 255.0f;
                    curB = (float)((col >> 0) & 0xFF) / 255.0f;
                    t->setColorRGBA((int)(curR * 255), (int)(curG * 255), (int)(curB * 255), (int)(curA * 255));
                    i++;
                    continue;
                }
            }

            if (c == '\n') {
                cx = startX + offX;
                cy += std::floor(getLineHeight() * drawScale);
                continue;
            }

            const GlyphInfo& glyph = m_fontSet->getGlyph(c);
            if (glyph.width > 0 && glyph.height > 0) {
                float vx0 = cx + std::floor(glyph.bearingX * drawScale);
                float vy0 = cy + std::floor(glyph.bearingY * drawScale);
                float vx1 = vx0 + (float)glyph.width;
                float vy1 = vy0 + (float)glyph.height;

                t->addVertexWithUV(vx0, vy1, 0, glyph.u0, glyph.v1);
                t->addVertexWithUV(vx1, vy1, 0, glyph.u1, glyph.v1);
                t->addVertexWithUV(vx1, vy0, 0, glyph.u1, glyph.v0);
                t->addVertexWithUV(vx0, vy0, 0, glyph.u0, glyph.v0);
            }
            cx += std::floor(glyph.advance * drawScale);
        }
        t->draw();
    };

    if (shadow) {
        uint32_t shadowColor = (color & 0xFF000000) | ((color & 0xFCFCFC) >> 2);
        float sOff = std::floor(1.0f * drawScale);
        renderPass(shadowColor, sOff, sOff, true);
    }
    
    renderPass(color, 0.0f, 0.0f, false);
}

int Font::getStringWidth(const std::string& text) {
    float width = 0;
    std::u32string u32text = UTF8::toUTF32(text);
    for (size_t i = 0; i < u32text.length(); ++i) {
        uint32_t c = u32text[i];
        if (c == '&' && i + 1 < u32text.length()) {
            i++;
            continue;
        }
        const GlyphInfo& glyph = m_fontSet->getGlyph(c);
        width += glyph.advance;
    }
    return (int)std::ceil(width);
}
