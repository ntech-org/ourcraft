#include "gui/GuiOptions.hpp"
#include "Minecraft.hpp"

void GuiOptions::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(100, width / 2 - 100, height / 6 + 120, 200, 20, "Done"));
    
    // Simple options buttons
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 101, height / 6 + 0, 100, 20, "FOV: " + std::to_string((int)mc->getSettings().fov)));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 + 1, height / 6 + 0, 100, 20, "Sens: " + std::to_string((int)(mc->getSettings().mouseSensitivity * 100)) + "%"));
}

void GuiOptions::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    
    Shader& shader = mc->getGameRenderer().getUIShader();
    FontRenderer& font = mc->getGameRenderer().getFontRenderer();
    
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, shader, "Options", (float)width / 2, 20, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiOptions::actionPerformed(GuiButton* button) {
    if (button->id == 100) {
        mc->getSettings().saveOptions();
        mc->displayGuiScreen(nullptr); // Go back
    }
    if (button->id == 1) {
        mc->getSettings().fov += 10.0f;
        if (mc->getSettings().fov > 110.0f) mc->getSettings().fov = 30.0f;
        button->text = "FOV: " + std::to_string((int)mc->getSettings().fov);
    }
}
