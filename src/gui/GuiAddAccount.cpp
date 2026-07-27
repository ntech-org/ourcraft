#include "gui/GuiAddAccount.hpp"
#include "gui/GuiAccountSelect.hpp"
#include "Minecraft.hpp"

GuiAddAccount::GuiAddAccount(std::shared_ptr<GuiScreen> parent) : m_parent(parent) {}

void GuiAddAccount::initGui() {
    controlList.clear();

    const int left = width / 2 - 140;
    m_nameField = std::make_unique<GuiTextField>(0, left, height / 4 + 34, 280, 22, mc->getWindow());
    m_nameField->setFocused(true, mc->getWindow());
    m_nameField->maxStringLength = 16;

    m_keyField = std::make_unique<GuiTextField>(1, left, height / 4 + 82, 280, 22, mc->getWindow());
    m_keyField->maxStringLength = 64;

    controlList.push_back(std::make_unique<GuiButton>(0, left, height / 4 + 130, 135, 22, "Done"));
    controlList.push_back(std::make_unique<GuiButton>(1, left + 145, height / 4 + 130, 135, 22, "Cancel"));
}

void GuiAddAccount::updateScreen() {
    m_nameField->updateCursorCounter();
    m_keyField->updateCursorCounter();
}

void GuiAddAccount::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawPanel(uiShader, width / 2 - 160, 10, width / 2 + 160, height / 4 + 170, 0xE0141416);
    drawCenteredString(font, textShader, "Add Account", (float)width / 2, 22, 0xFFFFFFFF);
    drawString(font, textShader, "Account Name", (float)width / 2 - 140, height / 4 + 20, 0xFFB8B8B8);
    m_nameField->drawTextField(mc, font, textShader);
    drawString(font, textShader, "Key (optional)", (float)width / 2 - 140, height / 4 + 68, 0xFFB8B8B8);
    m_keyField->drawTextField(mc, font, textShader);
    drawString(font, textShader, "Leave blank to generate a key automatically", (float)width / 2 - 140, height / 4 + 112, 0xFF888888);

    for (auto& btn : controlList) {
        btn->drawButton(mc, font, uiShader, mouseX, mouseY);
    }
    glEnable(GL_DEPTH_TEST);
}

void GuiAddAccount::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (key == SDLK_ESCAPE && down) {
        mc->displayGuiScreen(m_parent);
    } else if (key == SDLK_TAB && down) {
        if (m_nameField->isFocused()) {
            m_nameField->setFocused(false, mc->getWindow());
            m_keyField->setFocused(true, mc->getWindow());
        } else if (m_keyField->isFocused()) {
            m_keyField->setFocused(false, mc->getWindow());
            m_nameField->setFocused(true, mc->getWindow());
        }
    } else if (key == SDLK_RETURN && down) {
        actionPerformed(controlList[0].get());
    }

    m_nameField->keyTyped(key, scancode, down);
    m_keyField->keyTyped(key, scancode, down);
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiAddAccount::mouseClicked(int mouseX, int mouseY, int button) {
    m_nameField->mouseClicked(mouseX, mouseY, button);
    m_keyField->mouseClicked(mouseX, mouseY, button);
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiAddAccount::onTextInput(const char* text) {
    if (m_nameField && m_nameField->isFocused()) {
        m_nameField->appendText(text);
    } else if (m_keyField && m_keyField->isFocused()) {
        m_keyField->appendText(text);
    }
}

void GuiAddAccount::actionPerformed(GuiButton* button) {
    if (button->id == 0) { // Done
        std::string name = m_nameField->getText();
        std::string key = m_keyField->getText();

        if (!name.empty()) {
            auto& accounts = mc->getSettings().accounts;
            accounts.push_back({name, "00000000-0000-0000-0000-000000000000", key});
            mc->getSettings().activeAccountIndex = (int)accounts.size() - 1;
            mc->getSettings().saveOptions();

            // Update player
            mc->getPlayer().username = name;
            mc->getPlayer().uuid = accounts.back().uuid;
        }
        mc->displayGuiScreen(m_parent);
    } else if (button->id == 1) { // Cancel
        mc->displayGuiScreen(m_parent);
    }
}
