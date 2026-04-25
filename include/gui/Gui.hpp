#pragma once
#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include "renderer/Shader.hpp"
#include "renderer/FontRenderer.hpp"

class Gui {
public:
    static void drawRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color);
    static void drawGradientRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color1, uint32_t color2);
    static void drawCenteredString(FontRenderer& fontRenderer, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    static void drawString(FontRenderer& fontRenderer, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    static void drawTexturedModalRect(Shader& shader, float x, float y, int u, int v, int width, int height);
};
