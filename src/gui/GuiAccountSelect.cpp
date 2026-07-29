#include "gui/GuiAccountSelect.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiAddAccount.hpp"
#include "Minecraft.hpp"

GuiAccountSelect::GuiAccountSelect(std::shared_ptr<GuiScreen> parent) : m_parent(parent) {}

void GuiAccountSelect::initGui() {
    controlList.clear();

    m_addAccountButton = std::make_unique<GuiButton>(1, width / 2 - 154, height - 52, 100, 20, "Add Account");
    m_removeAccountButton = std::make_unique<GuiButton>(2, width / 2 - 50, height - 52, 100, 20, "Remove");
    m_doneButton = std::make_unique<GuiButton>(0, width / 2 + 54, height - 52, 100, 20, "Done");

    controlList.push_back(std::make_unique<GuiButton>(*m_addAccountButton));
    controlList.push_back(std::make_unique<GuiButton>(*m_removeAccountButton));
    controlList.push_back(std::make_unique<GuiButton>(*m_doneButton));

    m_removeAccountButton->enabled = false;

    rebuildAccountList();
}

void GuiAccountSelect::rebuildAccountList() {
    m_accountRows.clear();

    auto& accounts = mc->getSettings().accounts;
    int rowHeight = 36;
    int startY = height / 4 - 30;
    int listHeight = height - 100 - startY;

    for (size_t i = 0; i < accounts.size(); ++i) {
        int y = startY + (int)i * rowHeight;
        AccountRow row;
        row.y = y;
        row.height = rowHeight;
        row.index = (int)i;
        m_accountRows.push_back(row);
    }

    m_removeAccountButton->enabled = m_selectedAccount >= 0;
}

void GuiAccountSelect::updateScreen() {
    m_removeAccountButton->enabled = m_selectedAccount >= 0;
}

void GuiAccountSelect::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);

    drawCenteredString(font, textShader, "Select Account", (float)width / 2, 20, 0xFFFFFFFF);

    int listTop = height / 4 - 30;
    int listBottom = height - 80;
    drawRect(uiShader, width / 2 - 156, listTop - 2, width / 2 + 156, listBottom + 2, 0xFF000000);

    int rowHeight = 36;

    for (const auto& row : m_accountRows) {
        if (row.y + row.height < listTop || row.y > listBottom) continue;

        const auto& acc = mc->getSettings().accounts[row.index];
        bool isSelected = row.index == m_selectedAccount;
        bool isActive = row.index == mc->getSettings().activeAccountIndex;

        uint32_t rowColor = isSelected ? 0xFF3A3A5A : (isActive ? 0xFF2A3A2A : 0xFF1A1A2A);
        drawRect(uiShader, width / 2 - 154, row.y, width / 2 + 154, row.y + rowHeight - 2, rowColor);

        // Account name
        drawString(font, textShader, acc.name, (float)width / 2 - 150, (float)row.y + 4, 0xFFFFFFFF);
        // UUID (truncated)
        drawString(font, textShader, "UUID: " + acc.uuid.substr(0, 8) + "...", (float)width / 2 - 150, (float)row.y + 18, 0xFFA0A0A0);
        // Key status
        drawString(font, textShader, acc.key.empty() ? "Key: (none)" : "Key: set", (float)width / 2 + 20, (float)row.y + 4, 0xFF80FF80);
        if (isActive) {
            drawString(font, textShader, "(Active)", (float)width / 2 + 20, (float)row.y + 18, 0xFFFFFF00);
        }
    }

    for (auto& btn : controlList) {
        btn->drawButton(mc, font, uiShader, mouseX, mouseY);
    }

    glEnable(GL_DEPTH_TEST);
}

int GuiAccountSelect::getAccountAtMouse(int mouseY) {
    int startY = height / 4 - 30;
    int rowHeight = 36;

    for (const auto& row : m_accountRows) {
        if (mouseY >= row.y && mouseY < row.y + row.height) {
            return row.index;
        }
    }
    return -1;
}

void GuiAccountSelect::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (key == SDLK_ESCAPE && down) {
        mc->displayGuiScreen(m_parent);
    } else if (key == SDLK_UP && down) {
        if (m_selectedAccount > 0) m_selectedAccount--;
    } else if (key == SDLK_DOWN && down) {
        if (m_selectedAccount < (int)mc->getSettings().accounts.size() - 1) m_selectedAccount++;
    } else if (key == SDLK_RETURN && down && m_selectedAccount >= 0) {
        mc->getSettings().activeAccountIndex = m_selectedAccount;
        mc->getSettings().saveOptions();
        // Update player name/uuid
        mc->getPlayer().username = mc->getSettings().accounts[m_selectedAccount].name;
        mc->getPlayer().uuid = mc->getSettings().accounts[m_selectedAccount].uuid;
    }
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiAccountSelect::mouseClicked(int mouseX, int mouseY, int button) {
    if (button == SDL_BUTTON_LEFT) {
        int accountIdx = getAccountAtMouse(mouseY);
        if (accountIdx >= 0) {
            m_selectedAccount = accountIdx;
        }
    }
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiAccountSelect::actionPerformed(GuiButton* button) {
    switch (button->id) {
        case 0: // Done
            if (m_selectedAccount >= 0) {
                mc->getSettings().activeAccountIndex = m_selectedAccount;
                mc->getSettings().saveOptions();
                // Update player name/uuid
                mc->getPlayer().username = mc->getSettings().accounts[m_selectedAccount].name;
                mc->getPlayer().uuid = mc->getSettings().accounts[m_selectedAccount].uuid;
            }
            mc->displayGuiScreen(m_parent);
            break;
        case 1: // Add Account
            mc->displayGuiScreen(std::make_shared<GuiAddAccount>(shared_from_this()));
            break;
        case 2: // Remove Account
            if (m_selectedAccount >= 0 && m_selectedAccount < (int)mc->getSettings().accounts.size()) {
                mc->getSettings().accounts.erase(
                    mc->getSettings().accounts.begin() + m_selectedAccount);
                if (mc->getSettings().activeAccountIndex >= (int)mc->getSettings().accounts.size()) {
                    mc->getSettings().activeAccountIndex = 0;
                }
                mc->getSettings().saveOptions();
                m_selectedAccount = -1;
                rebuildAccountList();
            }
            break;
    }
}
