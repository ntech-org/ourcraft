#pragma once

#include "gui/GuiScreen.hpp"
#include <memory>
#include <string>
#include <filesystem>

class GuiMultiplayerWelcome : public GuiScreen {
public:
    GuiMultiplayerWelcome(std::shared_ptr<GuiScreen> parent);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;

    void openKeyLocation();
};