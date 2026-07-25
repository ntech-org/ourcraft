#include "gui/GuiSoundSettings.hpp"
#include "gui/GuiSlider.hpp"
#include "Minecraft.hpp"

void GuiSoundSettings::initGui() {
    m_pendingSettings = mc->getSettings();
    int midX = width / 2;

    float svVal = m_pendingSettings.soundVolume;
    auto svSlider = std::make_unique<GuiSlider>(0, midX - 155, height / 6 + 0, svVal, "Sound: ", [this](float val) {
        this->m_pendingSettings.soundVolume = val;
    });
    svSlider->width = 310;
    controlList.push_back(std::move(svSlider));

    float mvVal = m_pendingSettings.musicVolume;
    auto mvSlider = std::make_unique<GuiSlider>(10, midX - 155, height / 6 + 24, mvVal, "Music: ", [this](float val) {
        this->m_pendingSettings.musicVolume = val;
    });
    mvSlider->width = 310;
    controlList.push_back(std::move(mvSlider));

    controlList.push_back(std::make_unique<GuiButton>(100, midX - 100, height / 6 + 60, 200, 20, "Done"));
}

void GuiSoundSettings::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "Sound Settings", (float)width / 2, 15, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiSoundSettings::actionPerformed(GuiButton* button) {
    if (button->id == 100) {
        mc->getSettings() = m_pendingSettings;
        mc->getSettings().saveOptions();
        mc->displayGuiScreen(parentScreen);
    }
}
