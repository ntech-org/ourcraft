#include "gui/GuiMultiplayer.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiConnecting.hpp"
#include "Minecraft.hpp"
#include <iostream>

GuiMultiplayer::GuiMultiplayer(std::shared_ptr<GuiScreen> parent) : m_parent(parent) {}

void GuiMultiplayer::initGui() {
    m_serverAddressField = std::make_unique<GuiTextField>(0, width / 2 - 100, height / 4 + 48, 200, 20, mc->getWindow());
    m_serverAddressField->setText("127.0.0.1");
    m_serverAddressField->setFocused(true, mc->getWindow());

    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 4 + 80, 200, 20, "Connect"));
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 120, 200, 20, "Back"));
}

void GuiMultiplayer::updateScreen() {
    if (m_serverAddressField) {
        m_serverAddressField->updateCursorCounter();
    }
}

void GuiMultiplayer::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "Multiplayer", (float)width / 2, 40, 0xFFFFFFFF);
    drawString(font, textShader, "Server Address:", (float)width / 2 - 100, (float)height / 4 + 35, 0xFFA0A0A0);

    if (m_serverAddressField) {
        m_serverAddressField->drawTextField(mc, font, textShader);
    }

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiMultiplayer::actionPerformed(GuiButton* button) {
    if (button->id == 1) {
        std::string address = m_serverAddressField->getText();
        size_t colonPos = address.find(':');
        std::string ip = address;
        int port = 25565;
        if (colonPos != std::string::npos) {
            ip = address.substr(0, colonPos);
            try {
                port = std::stoi(address.substr(colonPos + 1));
            } catch (...) {
                port = 25565;
            }
        }
        mc->displayGuiScreen(std::make_shared<GuiConnecting>(shared_from_this(), ip, port));
    }
    if (button->id == 0) {
        mc->displayGuiScreen(m_parent);
    }
}

void GuiMultiplayer::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (m_serverAddressField && m_serverAddressField->isFocused()) {
        m_serverAddressField->keyTyped(key, scancode, down);
        if (key == SDLK_RETURN && down) {
            actionPerformed(controlList[0].get()); // Click Connect
        }
    }
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiMultiplayer::mouseClicked(int mouseX, int mouseY, int button) {
    if (m_serverAddressField) {
        m_serverAddressField->mouseClicked(mouseX, mouseY, button);
    }
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}
