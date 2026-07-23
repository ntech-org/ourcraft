#include "gui/GuiOptionButton.hpp"
#include "Minecraft.hpp"

GuiOptionButton::GuiOptionButton(int id, int x, int y, int width, int height,
                                 const std::string& label, const std::vector<std::string>& options, int currentIndex)
    : GuiButton(id, x, y, width, height, ""), m_label(label), m_options(options), m_currentIndex(currentIndex) {
    updateText();
}

void GuiOptionButton::setCurrentIndex(int idx) {
    m_currentIndex = idx;
    if (m_currentIndex < 0) m_currentIndex = 0;
    if (m_currentIndex >= (int)m_options.size()) m_currentIndex = (int)m_options.size() - 1;
    updateText();
}

void GuiOptionButton::updateText() {
    if (m_currentIndex >= 0 && m_currentIndex < (int)m_options.size()) {
        this->text = m_label + ": " + m_options[m_currentIndex];
    }
}

void GuiOptionButton::drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) {
    if (!visible) return;

    mc->getGameRenderer().getRenderEngine().bindTexture(
        mc->getGameRenderer().getRenderEngine().getTexture(TEX_GUI));

    bool hovered = isMouseOver(mouseX, mouseY);
    int state = 1;
    if (!enabled) state = 0;
    else if (hovered) state = 2;

    drawTexturedModalRect(shader, (float)x, (float)y, 0, 46 + state * 20, width / 2, height);
    drawTexturedModalRect(shader, (float)x + (float)width / 2.0f, (float)y,
                          200 - (float)(width - width / 2), 46 + state * 20,
                          width - width / 2, height);

    uint32_t textColor = 0xFFE0E0E0;
    if (!enabled) textColor = 0xFFA0A0A0;
    else if (hovered) textColor = 0xFFFFFFA0;

    drawCenteredString(font, mc->getGameRenderer().getTextShader(), text,
                       (float)x + (float)width / 2.0f, (float)y + (float)(height - 8) / 2.0f, textColor);
}

bool GuiOptionButton::mousePressed(int mouseX, int mouseY) {
    if (!enabled || !visible) return false;
    if (isMouseOver(mouseX, mouseY)) {
        m_currentIndex = (m_currentIndex + 1) % (int)m_options.size();
        updateText();
        if (onValueChange) onValueChange(m_currentIndex);
        return true;
    }
    return false;
}
