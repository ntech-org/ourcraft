#pragma once
#include <string>
#include <cstdint>
#include "renderer/RenderEngine.hpp"
#include "renderer/Shader.hpp"

class FontRenderer {
public:
    FontRenderer(RenderEngine* renderEngine, const std::string& fontPath);
    void drawString(Shader& shader, const std::string& text, float x, float y, uint32_t color);
    int getStringWidth(const std::string& text);
    
private:
    RenderEngine* m_renderEngine;
    int m_fontTexture;
};
