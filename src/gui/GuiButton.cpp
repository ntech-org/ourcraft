#include "gui/GuiButton.hpp"
#include "Minecraft.hpp"
#include "renderer/GameRenderer.hpp"

GuiButton::GuiButton(int id, int x, int y, int width, int height, const std::string& text)
    : id(id), x(x), y(y), width(width), height(height), text(text) {}

void GuiButton::drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    bool hovered = isMouseOver(mouseX, mouseY);
    uint32_t background = !enabled ? 0xFF252525 : hovered ? 0xFF4B6078 : 0xFF343434;
    uint32_t border = !enabled ? 0xFF303030 : hovered ? 0xFF9DB9D5 : 0xFF555555;
    drawRect(shader, (float)x - 1.0f, (float)y - 1.0f,
             (float)x + (float)width + 1.0f, (float)y + (float)height + 1.0f, border);
    drawRect(shader, (float)x, (float)y, (float)x + (float)width,
             (float)y + (float)height, background);

    uint32_t textColor = 0xFFE0E0E0;
    if (!enabled) textColor = 0xFFA0A0A0;
    else if (hovered) textColor = 0xFFFFFFA0;

    drawCenteredString(font, mc->getGameRenderer().getTextShader(), text, (float)x + (float)width / 2.0f,
                       (float)y + (float)(height - 8) / 2.0f, textColor);
}

bool GuiButton::mousePressed(int mouseX, int mouseY) {
    return enabled && visible && isMouseOver(mouseX, mouseY);
}

bool GuiButton::isMouseOver(int mouseX, int mouseY) {
    return mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
}
