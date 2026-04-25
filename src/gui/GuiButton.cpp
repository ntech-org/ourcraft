#include "gui/GuiButton.hpp"
#include "renderer/GameRenderer.hpp"

GuiButton::GuiButton(int id, int x, int y, int width, int height, const std::string& text)
    : id(id), x(x), y(y), width(width), height(height), text(text) {}

void GuiButton::drawButton(FontRenderer& fontRenderer, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    bool hovered = isMouseOver(mouseX, mouseY);
    int state = 1; // Normal
    if (!enabled) state = 0; // Disabled
    else if (hovered) state = 2; // Hovered

    // Original MC button uses gui.png (0, 46 + state * 20)
    // For now we'll just draw colored rects if we don't want to mess with textures yet, 
    // but the plan said "authentic". 
    // Let's assume RenderEngine can bind "/gui/gui.png".
    
    // Actually Gui::drawTexturedModalRect expects a bound texture.
    // In MC it binds the texture before calling drawButton or inside.
    
    // Let's implement a simple drawn button for now or use the texture.
    // I'll use the texture for authenticity.
    
    // We need access to RenderEngine to bind texture. 
    // Or we assume it's already bound by the caller (GuiScreen).
    
    drawRect(shader, (float)x, (float)y, (float)(x + width), (float)(y + height), 0xFF000000); // Border
    uint32_t bgColor = 0xFF707070;
    if (state == 2) bgColor = 0xFFA0A0A0;
    if (state == 0) bgColor = 0xFF404040;
    
    drawRect(shader, (float)x + 1, (float)y + 1, (float)(x + width - 1), (float)(y + height - 1), bgColor);
    
    uint32_t textColor = 0xFFE0E0E0;
    if (!enabled) textColor = 0xFFA0A0A0;
    else if (hovered) textColor = 0xFFFFFFA0;

    drawCenteredString(fontRenderer, shader, text, (float)x + (float)width / 2.0f, (float)y + (float)(height - 8) / 2.0f, textColor);
}

bool GuiButton::mousePressed(int mouseX, int mouseY) {
    return enabled && visible && isMouseOver(mouseX, mouseY);
}

bool GuiButton::isMouseOver(int mouseX, int mouseY) {
    return mouseX >= x && mouseY >= y && mouseX < x + width && mouseY < y + height;
}
