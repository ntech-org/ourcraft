#pragma once
#include "gui/Gui.hpp"
#include <string>

class Font;

class Minecraft;

class GuiButton : public Gui {
public:
    GuiButton(int id, int x, int y, int width, int height, const std::string& text);
    virtual ~GuiButton() = default;

    virtual void drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY);
    virtual bool mousePressed(int mouseX, int mouseY);

    int id;
    int x, y;
    int width, height;
    std::string text;
    bool enabled = true;
    bool visible = true;

protected:
    bool isMouseOver(int mouseX, int mouseY);
};
