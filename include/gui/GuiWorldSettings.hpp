#pragma once

#include "gui/GuiScreen.hpp"
#include "gui/GuiTextField.hpp"
#include <memory>
#include <string>

class GuiWorldSettings : public GuiScreen {
public:
    GuiWorldSettings(std::shared_ptr<GuiScreen> parent, const std::string& worldName);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void onTextInput(const char* text) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;
    std::string m_worldName;
    std::unique_ptr<GuiTextField> m_seedField;
    bool m_farLands = false;
};
