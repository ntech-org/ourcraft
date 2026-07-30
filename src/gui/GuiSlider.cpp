#include "gui/GuiSlider.hpp"
#include "Minecraft.hpp"
#include <SDL3/SDL.h>

GuiSlider::GuiSlider(int id, int x, int y, float value, const std::string& prefix, std::function<void(float)> callback)
    : GuiButton(id, x, y, 200, 20, ""), sliderValue(value), prefix(prefix), onValueChange(callback) {
    updateText();
}

void GuiSlider::updateText() {
    if (id == 1) { // Render Distance
        int chunks = 2 + (int)(sliderValue * 126.0f);
        this->text = prefix + std::to_string(chunks) + " chunks";
    } else if (id == 6) { // FOV
        this->text = prefix + std::to_string((int)(30.0f + sliderValue * 80.0f));
    } else if (id == 9) { // Maximum frame rate
        this->text = prefix + std::to_string(30 + (int)(sliderValue * 210.0f));
    } else {
        this->text = prefix + std::to_string((int)(sliderValue * 100)) + "%";
    }
}

void GuiSlider::drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    constexpr int textureHeight = 20;

    if (dragging) {
        float mx, my;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        if (!(buttons & SDL_BUTTON_LMASK)) {
            dragging = false;
        } else {
            sliderValue = (float)(mouseX - (x + 4)) / (float)(width - 8);
            if (sliderValue < 0.0f) sliderValue = 0.0f;
            if (sliderValue > 1.0f) sliderValue = 1.0f;

            updateText();
            if (onValueChange) onValueChange(sliderValue);
        }
    }

    mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture(TEX_GUI));
    shader.setBool("hasTexture", true);

    if (width <= 200) {
        drawTexturedModalRect(shader, (float)x, (float)y, 0, 46, width / 2, textureHeight);
        drawTexturedModalRect(shader, (float)x + (float)width / 2.0f - 1.0f, (float)y,
                              200 - (width - width / 2) - 1, 46, width - width / 2 + 1, textureHeight);
    } else {
        drawTexturedModalRect(shader, (float)x, (float)y, 0, 46, 100, textureHeight);
        drawTexturedModalRect(shader, (float)x + (float)width - 100.0f, (float)y, 100, 46, 100, textureHeight);
        drawTexturedModalRect(shader, (float)x + 100.0f, (float)y, 50, 46, (float)width - 200.0f, textureHeight);
    }

    // Draw knob
    float knobX = (float)x + sliderValue * (float)(width - 8);
    int knobState = dragging || isMouseOver(mouseX, mouseY) ? 2 : 1;
    drawTexturedModalRect(shader, knobX, (float)y, 0, 46 + knobState * 20, 4, textureHeight);
    drawTexturedModalRect(shader, knobX + 4.0f, (float)y, 196, 46 + knobState * 20, 4, textureHeight);

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
