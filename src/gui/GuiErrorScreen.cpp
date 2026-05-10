#include "gui/GuiErrorScreen.hpp"
#include "Minecraft.hpp"

GuiErrorScreen::GuiErrorScreen(const std::string& title, const std::string& message)
    : m_title(title), m_message(message) {}

void GuiErrorScreen::initGui() {
    controlList.clear();
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 120, 200, 20, "Back to title screen"));
}

void GuiErrorScreen::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), m_title, width / 2, height / 4, 0xFFFFFFFF);
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), m_message, width / 2, height / 4 + 24, 0xFFA0A0A0);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiErrorScreen::actionPerformed(GuiButton* button) {
    if (button->id == 0) {
        mc->saveAndQuit();
    }
}
