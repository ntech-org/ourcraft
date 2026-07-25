#pragma once
#include "gui/GuiScreen.hpp"
#include "util/GameSettings.hpp"

class GuiSoundSettings : public GuiScreen {
public:
    GuiSoundSettings(std::shared_ptr<GuiScreen> parent) { parentScreen = parent; }
    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;
private:
    GameSettings m_pendingSettings;
};
