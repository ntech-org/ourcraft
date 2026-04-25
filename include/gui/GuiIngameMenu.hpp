#pragma once
#include "gui/GuiScreen.hpp"

class GuiIngameMenu : public GuiScreen {
public:
    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;
};
