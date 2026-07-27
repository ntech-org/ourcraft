#include "gui/GuiOptions.hpp"
#include "gui/GuiGraphicsSettings.hpp"
#include "gui/GuiSoundSettings.hpp"
#include "gui/GuiControls.hpp"
#include "Minecraft.hpp"

void GuiOptions::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(100, width / 2 - 100, height / 6 + 144, 200, 20, "Done"));

    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 6 + 0, 200, 20, "Video Settings..."));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 - 100, height / 6 + 24, 200, 20, "Sound Settings..."));
    controlList.push_back(std::make_unique<GuiButton>(3, width / 2 - 100, height / 6 + 48, 200, 20, "Controls Settings..."));
}

void GuiOptions::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "Options", (float)width / 2, 20, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiOptions::actionPerformed(GuiButton* button) {
    if (button->id == 100) {
        mc->getSettings().saveOptions();
        mc->displayGuiScreen(parentScreen);
    }
    if (button->id == 1) {
        auto graphicsScreen = std::make_shared<GuiGraphicsSettings>();
        graphicsScreen->parentScreen = std::static_pointer_cast<GuiScreen>(shared_from_this());
        mc->displayGuiScreen(graphicsScreen);
    }
    if (button->id == 2) {
        auto soundScreen = std::make_shared<GuiSoundSettings>(shared_from_this());
        soundScreen->parentScreen = shared_from_this();
        mc->displayGuiScreen(soundScreen);
    }
    if (button->id == 3) {
        auto controlsScreen = std::make_shared<GuiControls>(shared_from_this());
        controlsScreen->parentScreen = shared_from_this();
        mc->displayGuiScreen(controlsScreen);
    }
}
