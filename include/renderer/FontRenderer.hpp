#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "renderer/RenderEngine.hpp"
#include "renderer/Shader.hpp"

class FontRenderer {
public:
    FontRenderer(RenderEngine* renderEngine, const std::string& fontPath);
    ~FontRenderer();

    void setGuiScale(int scale);
    void drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color);
    void drawStringWithShadow(Shader& shader, const std::string& text, float x, float y, uint32_t color);
    int getStringWidth(const std::string& text);
    
private:
    RenderEngine* m_renderEngine;
    int m_fontTexture;
    int m_guiScale = 1;
    
    struct GlyphInfo {
        float x0, y0, x1, y1; // tex coords
        float offX, offY;
        float advance;
        int w, h;
    };

    const GlyphInfo& getGlyph(char32_t codePoint);
    void loadGlyphToAtlas(char32_t codePoint, GlyphInfo& info);
    void updateFontSizes();

    FT_Library m_ft;
    FT_Face m_face;
    FT_Face m_fallbackFace;
    
    std::vector<unsigned char> m_primaryFontData;
    std::vector<unsigned char> m_fallbackFontData;

    std::unordered_map<char32_t, GlyphInfo> m_glyphCache;
    
    int m_atlasWidth = 1024;
    int m_atlasHeight = 1024;
    int m_nextX = 1;
    int m_nextY = 1;
    int m_maxHeight = 0;
    
    float m_fontSize = 8.0f;
    uint32_t m_colorTable[32];
};
