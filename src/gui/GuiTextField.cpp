#include "gui/GuiTextField.hpp"
#include <SDL3/SDL.h>
#include "Minecraft.hpp"

GuiTextField::GuiTextField(int id, int x, int y, int width, int height)
    : id(id), x(x), y(y), width(width), height(height) {}

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

void GuiTextField::setFocused(bool focused) {
    m_isFocused = focused;
    if (m_isFocused) {
        SDL_StartTextInput(SDL_GetKeyboardFocus());
    } else {
        SDL_StopTextInput(SDL_GetKeyboardFocus());
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
    } else if (m_text.length() < (size_t)maxStringLength) {
        // In SDL3, we should use SDL_EVENT_TEXT_INPUT for actual characters,
        // but for basic IP addresses/ASCII we can still map keycodes.
        // Let's implement a very basic mapping for now to keep it similar to the original.
        
        char c = 0;
        const bool shift = SDL_GetModState() & SDL_KMOD_SHIFT;

        if (key >= SDLK_0 && key <= SDLK_9) {
            c = '0' + (key - SDLK_0);
            if (shift) {
                if (c == '0') c = ')';
                else if (c == '1') c = '!';
                else if (c == '2') c = '@';
                else if (c == '3') c = '#';
                else if (c == '4') c = '$';
                else if (c == '5') c = '%';
                else if (c == '6') c = '^';
                else if (c == '7') c = '&';
                else if (c == '8') c = '*';
                else if (c == '9') c = '(';
            }
        } else if (key >= SDLK_A && key <= SDLK_Z) {
            c = (shift ? 'A' : 'a') + (key - SDLK_A);
        } else if (key == SDLK_PERIOD) {
            c = shift ? '>' : '.';
        } else if (key == SDLK_MINUS) {
            c = shift ? '_' : '-';
        } else if (key == SDLK_SEMICOLON) {
            c = shift ? ':' : ';';
        }

        if (c != 0) {
            m_text += c;
        }
    }
}

void GuiTextField::mouseClicked(int mouseX, int mouseY, int button) {
    bool over = mouseX >= x && mouseX < x + width && mouseY >= y && mouseY < y + height;
    setFocused(over);
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
