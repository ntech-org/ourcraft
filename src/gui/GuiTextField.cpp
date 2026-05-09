#include "gui/GuiTextField.hpp"
#include <GLFW/glfw3.h>
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
}

bool GuiTextField::isFocused() const {
    return m_isFocused;
}

void GuiTextField::keyTyped(int key, int scancode, int action, int mods) {
    if (!m_isFocused || action == GLFW_RELEASE) return;

    if (key == GLFW_KEY_BACKSPACE) {
        if (!m_text.empty()) {
            m_text.pop_back();
        }
    } else if (m_text.length() < (size_t)maxStringLength) {
        // Simple character handling for ASCII/IP addresses
        // In a real implementation we'd use glfwSetCharCallback
        bool shift = (mods & GLFW_MOD_SHIFT);
        char c = 0;
        
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            c = '0' + (key - GLFW_KEY_0);
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
        } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            c = (shift ? 'A' : 'a') + (key - GLFW_KEY_A);
        } else if (key == GLFW_KEY_PERIOD) {
            c = shift ? '>' : '.';
        } else if (key == GLFW_KEY_MINUS) {
            c = shift ? '_' : '-';
        } else if (key == GLFW_KEY_SEMICOLON) {
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

void GuiTextField::drawTextField(Minecraft* mc, FontRenderer& fontRenderer, Shader& shader) {
    if (!visible) return;

    // Draw border and background
    drawRect(shader, (float)x - 1, (float)y - 1, (float)x + (float)width + 1, (float)y + (float)height + 1, 0xFFA0A0A0);
    drawRect(shader, (float)x, (float)y, (float)x + (float)width, (float)y + (float)height, 0xFF000000);

    int color = 0xFFE0E0E0;
    std::string display = m_text;
    if (m_isFocused && (m_cursorCounter / 6) % 2 == 0) {
        display += "_";
    }

    fontRenderer.drawString(shader, display, (float)x + 4, (float)y + ((float)height - 8) / 2.0f, color);
}
