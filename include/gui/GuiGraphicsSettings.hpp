#pragma once
#include "gui/GuiScreen.hpp"
#include "util/GameSettings.hpp"
#include <memory>

class GuiGraphicsSettings : public GuiScreen {
public:
    GuiGraphicsSettings() = default;

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void actionPerformed(GuiButton* button) override;

private:
    void applySettings();
    void resetDefaults();

    GameSettings m_pendingSettings;
};
