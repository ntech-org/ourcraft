#include "gui/GuiTextField.hpp"
#include <SDL3/SDL.h>
#include "Minecraft.hpp"

GuiTextField::GuiTextField(int id, int x, int y, int width, int height, SDL_Window* window)
    : id(id), x(x), y(y), width(width), height(height), m_window(window) {}

void GuiTextField::updateCursorCounter() {
    m_cursorCounter++;
}

void GuiTextField::setText(const std::string& text) {
    m_text = text;
    if (m_text.length() > (size_t)maxStringLength) {
        m_text = m_text.substr(0, maxStringLength);
    }
}

std::string GuiTextField::getText() const {
    return m_text;
}

void GuiTextField::setFocused(bool focused, SDL_Window* window) {
    m_isFocused = focused;
    SDL_Window* w = window ? window : m_window;
    if (w) {
        if (m_isFocused) {
            SDL_StartTextInput(w);
        } else {
            SDL_StopTextInput(w);
        }
    }
}

bool GuiTextField::isFocused() const {
    return m_isFocused;
}

void GuiTextField::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!m_isFocused || !down) return;

    if (key == SDLK_BACKSPACE) {
        if (!m_text.empty()) {
            m_text.pop_back();
        }
    }
}

void GuiTextField::appendText(const std::string& text) {
    if (!m_isFocused) return;
    for (char c : text) {
        if (m_text.length() < (size_t)maxStringLength) {
            m_text += c;
        }
    }
}

void GuiTextField::mouseClicked(int mouseX, int mouseY, int button) {
    bool over = mouseX >= x && mouseX < x + width && mouseY >= y && mouseY < y + height;
    setFocused(over, m_window);
}

void GuiTextField::drawTextField(Minecraft* mc, Font& font, Shader& shader) {
    if (!visible) return;

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    drawRect(uiShader, (float)x - 1, (float)y - 1, (float)x + (float)width + 1, (float)y + (float)height + 1, 0xFFA0A0A0);
    drawRect(uiShader, (float)x, (float)y, (float)x + (float)width, (float)y + (float)height, 0xFF000000);

    int color = 0xFFE0E0E0;
    std::string display = m_text;
    if (m_isFocused && (m_cursorCounter / 6) % 2 == 0) {
        display += "_";
    }

    font.drawString(mc->getGameRenderer().getTextShader(), display, (float)x + 4, (float)y + ((float)height - 8) / 2.0f, color, false);
}
