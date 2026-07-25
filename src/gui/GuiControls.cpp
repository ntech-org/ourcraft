#include "gui/GuiControls.hpp"
#include "Minecraft.hpp"

void GuiControls::initGui() {
    int midX = width / 2;
    controlList.push_back(std::make_unique<GuiButton>(0, midX - 155, height / 6 + 0, 70, 20, "Forward: W"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(1, midX - 80, height / 6 + 0, 70, 20, "Back: S"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(2, midX - 5, height / 6 + 0, 70, 20, "Left: A"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(3, midX + 70, height / 6 + 0, 70, 20, "Right: D"));
    controlList.back()->enabled = false;

    controlList.push_back(std::make_unique<GuiButton>(4, midX - 155, height / 6 + 24, 70, 20, "Jump: Space"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(5, midX - 80, height / 6 + 24, 70, 20, "Sneak: LShift"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(6, midX - 5, height / 6 + 24, 70, 20, "Sprint: LCtrl"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(7, midX + 70, height / 6 + 24, 70, 20, "Drop: Q"));
    controlList.back()->enabled = false;

    controlList.push_back(std::make_unique<GuiButton>(8, midX - 155, height / 6 + 48, 70, 20, "Inv: E"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(9, midX - 80, height / 6 + 48, 70, 20, "Chat: T"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(10, midX - 5, height / 6 + 48, 70, 20, "Cam: F5"));
    controlList.back()->enabled = false;
    controlList.push_back(std::make_unique<GuiButton>(11, midX + 70, height / 6 + 48, 70, 20, "Debug: F3"));
    controlList.back()->enabled = false;

    controlList.push_back(std::make_unique<GuiButton>(100, midX - 100, height / 6 + 84, 200, 20, "Done"));
}

void GuiControls::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "Controls", (float)width / 2, 15, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiControls::actionPerformed(GuiButton* button) {
    if (button->id == 100) {
        mc->displayGuiScreen(parentScreen);
    }
}
