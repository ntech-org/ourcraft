#include "gui/GuiDeathScreen.hpp"
#include "Minecraft.hpp"

void GuiDeathScreen::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 72, 200, 20, "Respawn"));
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 4 + 96, 200, 20, "Title Menu"));
}

void GuiDeathScreen::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "You died!", (float)width / 2, (float)height / 4 + 20, 0xFFFF0000);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiDeathScreen::actionPerformed(GuiButton* button) {
    if (button->id == 0) {
        mc->getPlayer().health = mc->getPlayer().maxHealth;
        mc->getPlayer().deathTime = 0;
        mc->displayGuiScreen(nullptr);
    }
    if (button->id == 1) {
        mc->saveAndQuit();
    }
}
