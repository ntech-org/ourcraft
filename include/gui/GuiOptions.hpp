#pragma once
#include "gui/GuiScreen.hpp"

class GuiOptions : public GuiScreen {
public:
    GuiOptions(std::shared_ptr<GuiScreen> parent = nullptr) { parentScreen = parent; }
    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;
};
