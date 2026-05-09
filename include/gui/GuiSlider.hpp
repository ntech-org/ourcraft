#pragma once
#include "gui/GuiButton.hpp"
#include "renderer/ModernFont.hpp"
#include <functional>

class GuiSlider : public GuiButton {
public:
    GuiSlider(int id, int x, int y, float value, const std::string& prefix, std::function<void(float)> callback = nullptr);
    
    void drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) override;
    bool mousePressed(int mouseX, int mouseY) override;
    
    float sliderValue;
    bool dragging = false;
    std::string prefix;
    std::function<void(float)> onValueChange;

private:
    void updateText();
};
