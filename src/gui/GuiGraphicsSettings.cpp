#include "gui/GuiGraphicsSettings.hpp"
#include "gui/GuiSlider.hpp"
#include "gui/GuiOptionButton.hpp"
#include "gui/GuiOptions.hpp"
#include "Minecraft.hpp"

void GuiGraphicsSettings::initGui() {
    m_pendingSettings = mc->getSettings();

    int midX = width / 2;

    // Render Distance Slider (2 to 128 chunks, mapped to 0-1)
    float rdVal = (float)(m_pendingSettings.renderDistanceChunks - 2) / 126.0f;
    auto rdSlider = std::make_unique<GuiSlider>(1, midX - 155, height / 6 + 4, rdVal, "Render Distance: ", [this](float val) {
        this->m_pendingSettings.renderDistanceChunks = 2 + (int)(val * 126.0f);
    });
    rdSlider->width = 310;
    rdSlider->height = 20;
    controlList.push_back(std::move(rdSlider));

    // Graphics: Fast / Fancy
    auto graphicsBtn = std::make_unique<GuiOptionButton>(2, midX - 155, height / 6 + 42, 150, 20,
        "Graphics", std::vector<std::string>{"Fast", "Fancy"}, m_pendingSettings.fancyGraphics ? 1 : 0);
    graphicsBtn->onValueChange = [this](int idx) {
        this->m_pendingSettings.fancyGraphics = (idx == 1);
    };
    controlList.push_back(std::move(graphicsBtn));

    // Clouds: Off / Fast / Fancy
    auto cloudsBtn = std::make_unique<GuiOptionButton>(3, midX + 5, height / 6 + 42, 150, 20,
        "Clouds", std::vector<std::string>{"Off", "Fast", "Fancy"}, m_pendingSettings.cloudLevel);
    cloudsBtn->onValueChange = [this](int idx) {
        this->m_pendingSettings.cloudLevel = idx;
    };
    controlList.push_back(std::move(cloudsBtn));

    // View Bobbing: ON / OFF
    auto bobbingBtn = std::make_unique<GuiOptionButton>(4, midX - 155, height / 6 + 66, 150, 20,
        "View Bobbing", std::vector<std::string>{"OFF", "ON"}, m_pendingSettings.viewBobbing ? 1 : 0);
    bobbingBtn->onValueChange = [this](int idx) {
        this->m_pendingSettings.viewBobbing = (idx == 1);
    };
    controlList.push_back(std::move(bobbingBtn));

    // AO: OFF / ON
    auto aoBtn = std::make_unique<GuiOptionButton>(5, midX + 5, height / 6 + 66, 150, 20,
        "AO", std::vector<std::string>{"OFF", "ON"}, m_pendingSettings.ambientOcclusion ? 1 : 0);
    aoBtn->onValueChange = [this](int idx) {
        this->m_pendingSettings.ambientOcclusion = (idx == 1);
    };
    controlList.push_back(std::move(aoBtn));

    // FOV Slider (30 to 110)
    float fovVal = (m_pendingSettings.fov - 30.0f) / 80.0f;
    auto fovSlider = std::make_unique<GuiSlider>(6, midX - 155, height / 6 + 104, fovVal, "FOV: ", [this](float val) {
        this->m_pendingSettings.fov = 30.0f + val * 80.0f;
    });
    fovSlider->width = 150;
    fovSlider->height = 20;
    controlList.push_back(std::move(fovSlider));

    // GUI Scale Button
    std::string scaleText = "GUI Scale: ";
    if (m_pendingSettings.guiScale == 0) scaleText += "Auto";
    else scaleText += std::to_string(m_pendingSettings.guiScale);
    auto scaleBtn = std::make_unique<GuiButton>(7, midX + 5, height / 6 + 104, 150, 20, scaleText);
    controlList.push_back(std::move(scaleBtn));

    // Apply / Cancel / Reset buttons
    controlList.push_back(std::make_unique<GuiButton>(100, midX - 155, height / 6 + 140, 100, 20, "Reset"));
    controlList.push_back(std::make_unique<GuiButton>(101, midX - 50, height / 6 + 140, 100, 20, "Cancel"));
    controlList.push_back(std::make_unique<GuiButton>(102, midX + 55, height / 6 + 140, 100, 20, "Apply"));
}

void GuiGraphicsSettings::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "Video Settings", (float)width / 2, 15, 0xFFFFFFFF);

    // Draw section headers
    auto drawHeader = [&](const std::string& label, float y) {
        drawString(font, textShader, label, (float)width / 2 - 155, y, 0xFFAAAAAA);
    };

    float headerY = (float)height / 6;
    drawHeader("-- General --", headerY - 10);
    drawHeader("-- Quality --", headerY + 28);
    drawHeader("-- Details --", headerY + 90);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiGraphicsSettings::actionPerformed(GuiButton* button) {
    if (button->id == 100) {
        resetDefaults();
        mc->displayGuiScreen(std::make_shared<GuiGraphicsSettings>());
        return;
    }
    if (button->id == 101) {
        // Cancel must only replace this screen; never fall through into gameplay.
        auto destination = parentScreen;
        if (destination) mc->displayGuiScreen(destination);
        else mc->displayGuiScreen(std::make_shared<GuiOptions>());
        return;
    }
    if (button->id == 102) {
        applySettings();
        if (parentScreen) mc->displayGuiScreen(parentScreen);
        else mc->displayGuiScreen(std::make_shared<GuiOptions>());
        return;
    }
    if (button->id == 7) {
        m_pendingSettings.guiScale = (m_pendingSettings.guiScale + 1) % 9;
        std::string scaleText = "GUI Scale: ";
        if (m_pendingSettings.guiScale == 0) scaleText += "Auto";
        else scaleText += std::to_string(m_pendingSettings.guiScale);
        button->text = scaleText;
    }
}

void GuiGraphicsSettings::applySettings() {
    mc->getSettings() = m_pendingSettings;
    mc->getSettings().saveOptions();

    int w, h;
    SDL_GetWindowSizeInPixels(mc->getWindow(), &w, &h);
    mc->getGameRenderer().resize(w, h);
}

void GuiGraphicsSettings::resetDefaults() {
    m_pendingSettings = GameSettings();
    m_pendingSettings.setDefaults();
}
