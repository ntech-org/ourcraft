#include "gui/GuiWorldSettings.hpp"
#include "gui/GuiWorldList.hpp"
#include "Minecraft.hpp"
#include "world/storage/SaveHandler.hpp"
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

GuiWorldSettings::GuiWorldSettings(std::shared_ptr<GuiScreen> parent, const std::string& worldName)
    : m_parent(std::move(parent)), m_worldName(worldName) {}

void GuiWorldSettings::initGui() {
    controlList.clear();

    m_seedField = std::make_unique<GuiTextField>(0, width / 2 - 140, height / 4 + 24, 280, 22, mc->getWindow());
    m_seedField->setFocused(true, mc->getWindow());

    // Random seed button
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 + 145, height / 4 + 24, 80, 22, "Random"));

    // Far Lands toggle
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 - 140, height / 4 + 60, 280, 22, "Far Lands: OFF"));

    // Create and Cancel
    controlList.push_back(std::make_unique<GuiButton>(3, width / 2 - 140, height / 4 + 110, 135, 22, "Create"));
    controlList.push_back(std::make_unique<GuiButton>(4, width / 2 + 5, height / 4 + 110, 135, 22, "Cancel"));
}

void GuiWorldSettings::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    glDisable(GL_DEPTH_TEST);

    drawPanel(uiShader, width / 2 - 160, 10, width / 2 + 160, height / 4 + 150, 0xE0141416);
    drawCenteredString(font, textShader, "Create World: " + m_worldName, (float)width / 2, 22, 0xFFFFFFFF);
    drawString(font, textShader, "Seed", (float)width / 2 - 140, (float)height / 4 + 10, 0xFFB8B8B8);
    m_seedField->drawTextField(mc, font, textShader);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiWorldSettings::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;
    if (m_seedField && m_seedField->isFocused()) {
        m_seedField->keyTyped(key, scancode, down);
    }
    GuiScreen::keyTyped(key, scancode, down);
}

void GuiWorldSettings::onTextInput(const char* text) {
    if (m_seedField && m_seedField->isFocused()) {
        m_seedField->appendText(text);
    }
}

void GuiWorldSettings::actionPerformed(GuiButton* button) {
    switch (button->id) {
        case 1: { // Random seed
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<int64_t> dist;
            m_seedField->setText(std::to_string(dist(gen)));
            break;
        }
        case 2: // Toggle Far Lands
            m_farLands = !m_farLands;
            button->text = m_farLands ? "Far Lands: ON" : "Far Lands: OFF";
            break;
        case 3: { // Create
            fs::path worldDir = fs::path("worlds") / m_worldName;
            if (!fs::exists(worldDir)) fs::create_directories(worldDir);

            int64_t seed = 1772835215;
            try {
                seed = std::stoll(m_seedField->getText());
            } catch (...) {}

            SaveHandler save(worldDir.string());
            LevelData levelData;
            levelData.seed = seed;
            levelData.spawnX = 0; levelData.spawnY = 66; levelData.spawnZ = 0;
            levelData.time = 6000;
            levelData.farLands = m_farLands;
            save.saveLevelData(levelData);

            mc->displayGuiScreen(m_parent);
            break;
        }
        case 4: // Cancel
            mc->displayGuiScreen(m_parent);
            break;
    }
}
