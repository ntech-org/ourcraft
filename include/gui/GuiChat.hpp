#pragma once

#include "gui/GuiScreen.hpp"
#include "gui/GuiTextField.hpp"
#include <string>
#include <vector>

class GuiChat : public GuiScreen {
public:
    GuiChat();
    virtual ~GuiChat();

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    bool doesGuiPauseGame() const override { return false; }
    bool wantsCursor() const override { return true; }

    void handleEvent(const SDL_Event& event) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;

    void addSentMessage(const std::string& message);

private:
    GuiTextField* m_inputField = nullptr;
    std::vector<std::string> m_history;
    int m_historyIndex = -1;
    std::string m_currentInput;

    std::vector<std::string> m_tabCompletions;
    int m_tabIndex = -1;
    int m_tabCount = 0;
};
