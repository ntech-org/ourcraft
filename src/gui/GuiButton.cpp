#include "gui/GuiButton.hpp"
#include "Minecraft.hpp"
#include "renderer/GameRenderer.hpp"
#include <algorithm>

GuiButton::GuiButton(int id, int x, int y, int width, int height, const std::string& text)
    : id(id), x(x), y(y), width(width), height(height), text(text) {}

void GuiButton::drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    mc->getGameRenderer().getRenderEngine().bindTexture(mc->getGameRenderer().getRenderEngine().getTexture(TEX_GUI));
    shader.setBool("hasTexture", true);

    bool hovered = isMouseOver(mouseX, mouseY);
    int state = 1; // Normal
    if (!enabled) state = 0; // Disabled
    else if (hovered) state = 2; // Hovered

    // Original MC button uses gui.png (0, 46 + state * 20).
    int texH = std::min(height, 20);
    int leftWidth = width / 2;
    int rightWidth = width - leftWidth;
    // Draw left half
    drawTexturedModalRect(shader, (float)x, (float)y, 0, 46 + state * 20, leftWidth, texH);
    // Overlap the halves by one source pixel so fractional GUI scaling cannot expose a seam.
    drawTexturedModalRect(shader, (float)x + (float)leftWidth - 1.0f, (float)y,
                          200 - rightWidth - 1, 46 + state * 20, rightWidth + 1, texH);

    uint32_t textColor = 0xFFE0E0E0;
    if (!enabled) textColor = 0xFFA0A0A0;
    else if (hovered) textColor = 0xFFFFFFA0;

    drawCenteredString(font, mc->getGameRenderer().getTextShader(), text, (float)x + (float)width / 2.0f, (float)y + (float)(height - 8) / 2.0f, textColor);
}

bool GuiButton::mousePressed(int mouseX, int mouseY) {
    return enabled && visible && isMouseOver(mouseX, mouseY);
}

bool GuiButton::isMouseOver(int mouseX, int mouseY) {
    return mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
}
