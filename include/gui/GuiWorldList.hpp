#pragma once

#include "gui/GuiScreen.hpp"
#include <string>
#include <vector>

class GuiWorldList : public GuiScreen {
public:
    explicit GuiWorldList(std::shared_ptr<GuiScreen> parent);

    void initGui() override;
    void drawScreen(int mouseX, int mouseY, float partialTicks) override;
    void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
    void actionPerformed(GuiButton* button) override;

private:
    void refreshWorlds();
    void playSelectedWorld();
    void createWorld();
    void deleteSelectedWorld();

    std::shared_ptr<GuiScreen> m_parent;
    std::vector<std::string> m_worlds;
    int m_selectedWorld = -1;
};
