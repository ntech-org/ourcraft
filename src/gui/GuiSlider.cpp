#include "gui/GuiSlider.hpp"
#include "Minecraft.hpp"
#include <GLFW/glfw3.h>

GuiSlider::GuiSlider(int id, int x, int y, float value, const std::string& prefix, std::function<void(float)> callback)
    : GuiButton(id, x, y, 200, 20, ""), sliderValue(value), prefix(prefix), onValueChange(callback) {
    updateText();
}

void GuiSlider::updateText() {
    if (id == 1) { // FOV Special Case
        this->text = prefix + std::to_string((int)(30.0f + sliderValue * 80.0f));
    } else {
        this->text = prefix + std::to_string((int)(sliderValue * 100)) + "%";
    }
}

void GuiSlider::drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    GLFWwindow* window = mc->getWindow();
    if (dragging) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            dragging = false;
        } else {
            sliderValue = (float)(mouseX - (x + 4)) / (float)(width - 8);
            if (sliderValue < 0.0f) sliderValue = 0.0f;
            if (sliderValue > 1.0f) sliderValue = 1.0f;

            updateText();
            if (onValueChange) onValueChange(sliderValue);
        }
    }

    mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture("/gui/gui.png"));
    shader.setBool("hasTexture", true);

    // Draw background (disabled state look)
    drawTexturedModalRect(shader, (float)x, (float)y, 0, 46, width / 2, height);
    drawTexturedModalRect(shader, (float)x + (float)width / 2.0f, (float)y, 200 - (width - width / 2), 46, width - width / 2, height);

    // Draw knob
    float knobX = (float)x + sliderValue * (float)(width - 8);
    int knobState = dragging || isMouseOver(mouseX, mouseY) ? 2 : 1;
    drawTexturedModalRect(shader, knobX, (float)y, 0, 46 + knobState * 20, 4, 20);
    drawTexturedModalRect(shader, knobX + 4.0f, (float)y, 196, 46 + knobState * 20, 4, 20);

    drawCenteredString(font, mc->getGameRenderer().getTextShader(), text, (float)x + (float)width / 2.0f, (float)y + (float)(height - 8) / 2.0f, 0xFFE0E0E0);
}

bool GuiSlider::mousePressed(int mouseX, int mouseY) {
    if (GuiButton::mousePressed(mouseX, mouseY)) {
        sliderValue = (float)(mouseX - (x + 4)) / (float)(width - 8);
        if (sliderValue < 0.0f) sliderValue = 0.0f;
        if (sliderValue > 1.0f) sliderValue = 1.0f;
        dragging = true;

        updateText();
        if (onValueChange) onValueChange(sliderValue);
        return true;
    }
    return false;
}

