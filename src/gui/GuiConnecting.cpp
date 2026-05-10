#include "gui/GuiConnecting.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiErrorScreen.hpp"
#include "Minecraft.hpp"
#include <iostream>

GuiConnecting::GuiConnecting(std::shared_ptr<GuiScreen> parent, const std::string& address, int port, bool isSingleplayer)
    : m_parent(parent), m_address(address), m_port(port), m_isSingleplayer(isSingleplayer) {}

void GuiConnecting::initGui() {
    controlList.clear();
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 120, 200, 20, "Cancel"));
}

void GuiConnecting::updateScreen() {
    m_tickCount++;
    if (m_tickCount == 20) { // Wait 1 second (20 ticks)
        if (m_isSingleplayer) {
            mc->startSingleplayer();
        } else {
            mc->startMultiplayer(m_address, m_port);
        }
    }
}

void GuiConnecting::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), "Connecting to server...", width / 2, height / 2 - 20, 0xFFFFFFFF);
    drawCenteredString(mc->getFont(), mc->getGameRenderer().getTextShader(), m_address + ":" + std::to_string(m_port), width / 2, height / 2, 0xFFA0A0A0);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiConnecting::actionPerformed(GuiButton* button) {
    if (button->id == 0) {
        mc->saveAndQuit();
    }
}
