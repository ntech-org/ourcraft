#include "gui/GuiMainMenu.hpp"
#include "gui/GuiOptions.hpp"
#include "Minecraft.hpp"

void GuiMainMenu::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 4 + 48, 200, 20, "Singleplayer"));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 - 100, height / 4 + 72, 200, 20, "Multiplayer"));
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 96, 200, 20, "Options..."));
    controlList.push_back(std::make_unique<GuiButton>(4, width / 2 - 100, height / 4 + 120, 200, 20, "Quit Game"));
}

void GuiMainMenu::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    
    Shader& shader = mc->getGameRenderer().getUIShader();
    FontRenderer& font = mc->getGameRenderer().getFontRenderer();
    
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, shader, "OurCraft Infdev", (float)width / 2, 40, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiMainMenu::actionPerformed(GuiButton* button) {
    if (button->id == 1) {
        mc->startSingleplayer();
    }
    if (button->id == 0) {
        mc->displayGuiScreen(std::make_shared<GuiOptions>());
    }
    if (button->id == 4) {
        // Quit
        exit(0);
    }
}
