#include "gui/GuiIngameMenu.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiOptions.hpp"
#include "Minecraft.hpp"

void GuiIngameMenu::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 4 + 24, 200, 20, "Back to Game"));
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 48, 200, 20, "Options..."));
    controlList.push_back(std::make_unique<GuiButton>(5, width / 2 - 100, height / 4 + 96, 200, 20, "Save and Quit to Title"));
}

void GuiIngameMenu::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    
    Shader& shader = mc->getGameRenderer().getUIShader();
    FontRenderer& font = mc->getGameRenderer().getFontRenderer();
    
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, shader, "Game Menu", (float)width / 2, 40, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiIngameMenu::actionPerformed(GuiButton* button) {
    if (button->id == 1) {
        mc->displayGuiScreen(nullptr);
    }
    if (button->id == 0) {
        mc->displayGuiScreen(std::make_shared<GuiOptions>());
    }
    if (button->id == 5) {
        mc->saveAndQuit();
    }
}
