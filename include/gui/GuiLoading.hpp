#pragma once
#include "gui/GuiScreen.hpp"
#include <string>

class GuiLoading : public GuiScreen {
public:
    GuiLoading();

    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    bool doesGuiPauseGame() const override { return false; }
};
