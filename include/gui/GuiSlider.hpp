#pragma once
#include "gui/GuiButton.hpp"
#include <functional>

class GuiSlider : public GuiButton {
public:
    GuiSlider(int id, int x, int y, float value, const std::string& prefix, std::function<void(float)> callback = nullptr);
    
    void drawButton(Minecraft* mc, FontRenderer& fontRenderer, Shader& shader, int mouseX, int mouseY) override;
    bool mousePressed(int mouseX, int mouseY) override;
    
    float sliderValue;
    bool dragging = false;
    std::string prefix;
    std::function<void(float)> onValueChange;

private:
    void updateText();
};
