#include "gui/GuiAddServer.hpp"
#include "gui/GuiServerList.hpp"
#include "Minecraft.hpp"
#include <iostream>

GuiAddServer::GuiAddServer(std::shared_ptr<GuiScreen> parent, int editIndex)
    : m_parent(parent), m_editIndex(editIndex) {}

void GuiAddServer::initGui() {
    const int left = width / 2 - 140;
    m_nameField = std::make_unique<GuiTextField>(0, left, height / 4 + 34, 280, 22, mc->getWindow());
    m_addressField = std::make_unique<GuiTextField>(1, left, height / 4 + 82, 280, 22, mc->getWindow());
    m_aliasField = std::make_unique<GuiTextField>(2, left, height / 4 + 130, 280, 22, mc->getWindow());

    m_nameField->setFocused(true, mc->getWindow());

    if (m_editIndex >= 0 && m_editIndex < (int)mc->getSettings().serverList.size()) {
        const auto& server = mc->getSettings().serverList[m_editIndex];
        m_nameField->setText(server.name);
        m_addressField->setText(server.address + ":" + std::to_string(server.port));
        m_aliasField->setText(server.alias);
    } else {
        m_addressField->setText("127.0.0.1:25565");
    }

    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 140, height / 4 + 174, 135, 22, m_editIndex >= 0 ? "Edit Server" : "Add Server"));
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 + 5, height / 4 + 174, 135, 22, "Cancel"));
}

void GuiAddServer::updateScreen() {
    m_nameField->updateCursorCounter();
    m_addressField->updateCursorCounter();
    m_aliasField->updateCursorCounter();
}

void GuiAddServer::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawPanel(uiShader, width / 2 - 160, 10, width / 2 + 160, height / 4 + 210, 0xE0141416);
    drawCenteredString(font, textShader, m_editIndex >= 0 ? "Edit Server" : "Add Server", (float)width / 2, 22, 0xFFFFFFFF);

    drawString(font, textShader, "Server Name", (float)width / 2 - 140, (float)height / 4 + 20, 0xFFB8B8B8);
    m_nameField->drawTextField(mc, font, textShader);

    drawString(font, textShader, "Server Address", (float)width / 2 - 140, (float)height / 4 + 68, 0xFFB8B8B8);
    m_addressField->drawTextField(mc, font, textShader);

    drawString(font, textShader, "Account Alias (optional)", (float)width / 2 - 140, (float)height / 4 + 116, 0xFFB8B8B8);
    m_aliasField->drawTextField(mc, font, textShader);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiAddServer::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    m_nameField->keyTyped(key, scancode, down);
    m_addressField->keyTyped(key, scancode, down);
    m_aliasField->keyTyped(key, scancode, down);

    if (key == SDLK_TAB && down) {
        if (m_nameField->isFocused()) {
            m_nameField->setFocused(false, mc->getWindow());
            m_addressField->setFocused(true, mc->getWindow());
        } else if (m_addressField->isFocused()) {
            m_addressField->setFocused(false, mc->getWindow());
            m_aliasField->setFocused(true, mc->getWindow());
        } else if (m_aliasField->isFocused()) {
            m_aliasField->setFocused(false, mc->getWindow());
            m_nameField->setFocused(true, mc->getWindow());
        }
    } else if (key == SDLK_RETURN && down) {
        actionPerformed(controlList[0].get());
    }

    GuiScreen::keyTyped(key, scancode, down);
}

void GuiAddServer::mouseClicked(int mouseX, int mouseY, int button) {
    m_nameField->mouseClicked(mouseX, mouseY, button);
    m_addressField->mouseClicked(mouseX, mouseY, button);
    m_aliasField->mouseClicked(mouseX, mouseY, button);
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiAddServer::onTextInput(const char* text) {
    if (m_nameField && m_nameField->isFocused()) {
        m_nameField->appendText(text);
    } else if (m_addressField && m_addressField->isFocused()) {
        m_addressField->appendText(text);
    } else if (m_aliasField && m_aliasField->isFocused()) {
        m_aliasField->appendText(text);
    }
}

void GuiAddServer::actionPerformed(GuiButton* button) {
    if (button->id == 0) {
        std::string name = m_nameField->getText();
        std::string address = m_addressField->getText();
        std::string alias = m_aliasField->getText();

        if (name.empty()) return;

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

        if (m_editIndex >= 0 && m_editIndex < (int)mc->getSettings().serverList.size()) {
            mc->getSettings().serverList[m_editIndex] = {name, ip, port, alias};
        } else {
            mc->getSettings().serverList.push_back({name, ip, port, alias});
        }
        mc->getSettings().saveOptions();
        mc->displayGuiScreen(m_parent);
    } else if (button->id == 1) {
        mc->displayGuiScreen(m_parent);
    }
}
