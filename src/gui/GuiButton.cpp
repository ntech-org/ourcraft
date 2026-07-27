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
    int state = !enabled ? 0 : hovered ? 2 : 1;
    const int textureHeight = std::min(height, 20);
    const int leftWidth = width / 2;
    const int rightWidth = width - leftWidth;
    drawTexturedModalRect(shader, (float)x, (float)y, 0, 46 + state * 20,
                          leftWidth, textureHeight);
    drawTexturedModalRect(shader, (float)x + leftWidth - 1.0f, (float)y,
                          200 - rightWidth - 1, 46 + state * 20,
                          rightWidth + 1, textureHeight);

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
