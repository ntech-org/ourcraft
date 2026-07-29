#pragma once

#include "gui/GuiScreen.hpp"
#include <memory>
#include <vector>

class GuiAccountSelect : public GuiScreen {
public:
    GuiAccountSelect(std::shared_ptr<GuiScreen> parent);

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    struct AccountRow {
        int y;
        int height;
        int index;
    };

    std::shared_ptr<GuiScreen> m_parent;
    std::vector<AccountRow> m_accountRows;
    int m_selectedAccount = -1;

    std::unique_ptr<GuiButton> m_addAccountButton;
    std::unique_ptr<GuiButton> m_removeAccountButton;
    std::unique_ptr<GuiButton> m_doneButton;

    void rebuildAccountList();
    int getAccountAtMouse(int mouseY);
};