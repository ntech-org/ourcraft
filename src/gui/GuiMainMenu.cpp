#include "gui/GuiMainMenu.hpp"
#include "gui/GuiOptions.hpp"
#include "gui/GuiServerList.hpp"
#include "gui/GuiConnecting.hpp"
#include "gui/GuiMultiplayerWelcome.hpp"
#include "gui/GuiWorldList.hpp"
#include "Minecraft.hpp"
#include <filesystem>

void GuiMainMenu::initGui() {
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height / 4 + 48, 200, 20, "Singleplayer"));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 - 100, height / 4 + 72, 200, 20, "Multiplayer"));
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height / 4 + 96, 200, 20, "Options..."));
    controlList.push_back(std::make_unique<GuiButton>(4, width / 2 - 100, height / 4 + 120, 200, 20, "Quit Game"));
}

void GuiMainMenu::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    
    glDisable(GL_DEPTH_TEST);
    drawCenteredString(font, textShader, "OurCraft Infdev", (float)width / 2, 40, 0xFFFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiMainMenu::actionPerformed(GuiButton* button) {
    if (button->id == 1) {
        mc->displayGuiScreen(std::make_shared<GuiWorldList>(shared_from_this()));
    }
    if (button->id == 2) {
        // Check if first time opening multiplayer (no accounts.json)
        std::filesystem::path keyPath = std::filesystem::current_path() / "accounts.json";
        if (!std::filesystem::exists(keyPath)) {
            mc->displayGuiScreen(std::make_shared<GuiMultiplayerWelcome>(shared_from_this()));
        } else {
            mc->displayGuiScreen(std::make_shared<GuiServerList>(shared_from_this()));
        }
    }
    if (button->id == 0) {
        mc->displayGuiScreen(std::make_shared<GuiOptions>(shared_from_this()));
    }
    if (button->id == 4) {
        // Quit
        exit(0);
    }
}
