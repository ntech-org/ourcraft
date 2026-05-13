#pragma once
#include <string>
#include <cstdint>
#include "renderer/Shader.hpp"

class Font;

class Minecraft;
struct ItemStack;

class Gui {
public:
    static void drawRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color);
    static void drawGradientRect(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color1, uint32_t color2);
    static void drawCenteredString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    static void drawString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    static void drawTexturedModalRect(Shader& shader, float x, float y, int u, int v, int width, int height);

    // Unified stack rendering helpers
    static void drawItemStack(Minecraft* mc, const ItemStack& stack, float x, float y);
    static void drawBlockStack3D(Minecraft* mc, int blockID, float x, float y);
    static void drawBlockStack2D(Minecraft* mc, int blockID, float x, float y);
    static void drawItemIcon2D(Minecraft* mc, int itemID, float x, float y);
};
