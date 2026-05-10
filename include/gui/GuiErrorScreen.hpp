#pragma once
#include "gui/GuiScreen.hpp"
#include <string>

class GuiErrorScreen : public GuiScreen {
public:
    GuiErrorScreen(const std::string& title, const std::string& message);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::string m_title;
    std::string m_message;
};
