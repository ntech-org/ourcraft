#pragma once
#include "gui/GuiScreen.hpp"
#include <string>
#include <atomic>

class GuiConnecting : public GuiScreen {
public:
    GuiConnecting(std::shared_ptr<GuiScreen> parent, const std::string& address, int port, bool isSingleplayer = false);

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;

    bool doesGuiPauseGame() const override { return false; }

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;
    std::string m_address;
    int m_port;
    std::string m_status = "Connecting...";
    bool m_connected = false;
    bool m_failed = false;
    int m_tickCount = 0;
    bool m_isSingleplayer = false;
};
