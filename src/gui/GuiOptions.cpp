#include "gui/GuiOptions.hpp"
#include "gui/GuiSlider.hpp"
#include "Minecraft.hpp"

void GuiOptions::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(100, width / 2 - 100, height / 6 + 120, 200, 20, "Done"));

    // FOV Slider: 30 to 110
    float fovVal = (mc->getSettings().fov - 30.0f) / 80.0f;
    auto fovSlider = std::make_unique<GuiSlider>(1, width / 2 - 155, height / 6 + 0, fovVal, "FOV: ", [this](float val) {
        this->mc->getSettings().fov = 30.0f + val * 80.0f;
    });
    fovSlider->width = 150;
    controlList.push_back(std::move(fovSlider));

    // Sensitivity Slider: 0.0 to 1.0
    auto sensSlider = std::make_unique<GuiSlider>(2, width / 2 + 5, height / 6 + 0, mc->getSettings().mouseSensitivity, "Sensitivity: ", [this](float val) {
        this->mc->getSettings().mouseSensitivity = val;
    });
    sensSlider->width = 150;
    controlList.push_back(std::move(sensSlider));

    // GUI Scale Button
    std::string scaleText = "GUI Scale: ";
    if (mc->getSettings().guiScale == 0) scaleText += "Auto";
    else scaleText += std::to_string(mc->getSettings().guiScale);
    controlList.push_back(std::make_unique<GuiButton>(3, width / 2 - 100, height / 6 + 24, 200, 20, scaleText));
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
        mc->displayGuiScreen(parentScreen); // Go back to parent
    }
    if (button->id == 3) {
        mc->getSettings().guiScale = (mc->getSettings().guiScale + 1) % 9;
        std::string scaleText = "GUI Scale: ";
        if (mc->getSettings().guiScale == 0) scaleText += "Auto";
        else scaleText += std::to_string(mc->getSettings().guiScale);
        button->text = scaleText;
        
        // Trigger resize to update scaled resolution
        int w, h;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
        mc->getGameRenderer().resize(w, h);
        this->setWorldAndResolution(mc, mc->getGameRenderer().getScaledWidth(), mc->getGameRenderer().getScaledHeight());
    }
}
