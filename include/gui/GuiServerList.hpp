#pragma once

#include "gui/GuiScreen.hpp"
#include <memory>
#include <string>

class GuiServerList : public GuiScreen {
public:
    GuiServerList(std::shared_ptr<GuiScreen> parent);

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    std::shared_ptr<GuiScreen> m_parent;
    int m_scrollOffset = 0;
    int m_selectedServer = -1;
    int m_lastClickTime = 0;
    int m_lastClickServer = -1;
    int getMaxScroll() const;
    bool handleMouseWheel(int x, int y);
    void joinSelectedServer();
};