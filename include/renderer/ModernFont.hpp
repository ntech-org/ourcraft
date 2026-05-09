#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"

struct GlyphInfo {
    int width, height;
    float bearingX, bearingY;
    float advance;
    float u0, v0, u1, v1;
};

class FreeTypeGlyphProvider {
public:
    FreeTypeGlyphProvider(FT_Library ft, std::vector<uint8_t> fontData, float logicalSize);
    ~FreeTypeGlyphProvider();

    void setScale(float guiScale);
    bool getGlyphInfo(uint32_t codePoint, GlyphInfo& outInfo);
    void rasterizeGlyph(uint32_t codePoint, std::vector<uint8_t>& outBuffer, int& outW, int& outH);

    float getLogicalSize() const { return m_logicalSize; }

private:
    std::vector<uint8_t> m_fontData;
    FT_Face m_face;
    float m_logicalSize;
    float m_currentScale = 1.0f;
};

class FontSet {
public:
    FontSet(RenderEngine* renderEngine, float logicalSize);
    ~FontSet();

    void setScale(float guiScale);
    void addProvider(std::unique_ptr<FreeTypeGlyphProvider> provider);
    const GlyphInfo& getGlyph(uint32_t codePoint);

    int getTextureID() const { return m_textureID; }
    float getLogicalSize() const { return m_logicalSize; }
    float getScale() const { return m_currentScale; }

private:
    void uploadGlyph(uint32_t codePoint, const GlyphInfo& info, const std::vector<uint8_t>& bitmap);
    void clearCache();

    RenderEngine* m_renderEngine;
    float m_logicalSize;
    float m_currentScale = 1.0f;
    std::vector<std::unique_ptr<FreeTypeGlyphProvider>> m_providers;
    std::unordered_map<uint32_t, GlyphInfo> m_glyphs;

    int m_textureID = 0;
    int m_atlasWidth = 1024;
    int m_atlasHeight = 1024;
    int m_currentX = 1;
    int m_currentY = 1;
    int m_maxRowHeight = 0;
};

class Font {
public:
    Font(std::unique_ptr<FontSet> fontSet);
    
    void setDisplayContext(int screenW, int screenH, float guiScale);
    void drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color, bool shadow = false);
    int getStringWidth(const std::string& text);
    float getLineHeight() const { return 9.0f; }

private:
    std::unique_ptr<FontSet> m_fontSet;
    uint32_t m_colorTable[32];
    int m_screenW = 0, m_screenH = 0;
    float m_guiScale = 1.0f;
};
