#pragma once

#include "gui/GuiScreen.hpp"
#include "gui/GuiTextField.hpp"
#include <memory>

class GuiAddAccount : public GuiScreen {
public:
    GuiAddAccount(std::shared_ptr<GuiScreen> parent);

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void onTextInput(const char* text) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;

    std::unique_ptr<GuiTextField> m_nameField;
    std::unique_ptr<GuiTextField> m_keyField;
};