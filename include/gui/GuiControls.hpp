#pragma once
#include "gui/GuiScreen.hpp"

class GuiControls : public GuiScreen {
public:
    GuiControls(std::shared_ptr<GuiScreen> parent) { parentScreen = parent; }
    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;
};
