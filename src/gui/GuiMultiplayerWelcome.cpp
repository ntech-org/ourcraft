#include "gui/GuiMultiplayerWelcome.hpp"
#include "gui/GuiServerList.hpp"
#include "gui/GuiMainMenu.hpp"
#include "Minecraft.hpp"
#include <filesystem>
#include <cstdlib>

GuiMultiplayerWelcome::GuiMultiplayerWelcome(std::shared_ptr<GuiScreen> parent) : m_parent(parent) {}

void GuiMultiplayerWelcome::initGui() {
    controlList.clear();
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height - 80, 200, 20, "I've backed up my keys"));
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height - 50, 200, 20, "Open Key Location"));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 - 100, height - 20, 200, 20, "Back"));
}

void GuiMultiplayerWelcome::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);

    // Title
    drawCenteredString(font, textShader, "Welcome to Multiplayer!", (float)width / 2, 40, 0xFFFFFFFF);

    // Warning text
    std::string warning = "OurCraft uses a key-based identity system. Your key proves who you are on each server.";
    drawCenteredString(font, textShader, warning, (float)width / 2, 80, 0xFFFFFFFF);

    warning = "If you lose your key, you lose your username and progress on that server.";
    drawCenteredString(font, textShader, warning, (float)width / 2, 100, 0xFFFF0000);

    // Key location
    std::filesystem::path keyPath = std::filesystem::current_path() / "accounts.json";
    std::string pathStr = "Your key file is at: " + keyPath.string();
    drawCenteredString(font, textShader, pathStr, (float)width / 2, 140, 0xFFA0A0A0);

    // Instructions
    drawCenteredString(font, textShader, "Please back up this file (accounts.json) to a safe location.", (float)width / 2, 170, 0xFFFFFFFF);
    drawCenteredString(font, textShader, "You can copy it to a USB drive, cloud storage, or another folder.", (float)width / 2, 190, 0xFFFFFFFF);
    drawCenteredString(font, textShader, "In the future, cloud sync may be available.", (float)width / 2, 220, 0xFFA0A0A0);

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiMultiplayerWelcome::actionPerformed(GuiButton* button) {
    switch (button->id) {
        case 0: // I've backed up my keys
            // Save a flag so this doesn't show again
            mc->getSettings().saveOptions(); // This will save the accounts file existence
            mc->displayGuiScreen(std::make_shared<GuiServerList>(m_parent));
            break;
        case 1: // Open Key Location
            openKeyLocation();
            break;
        case 2: // Back
            mc->displayGuiScreen(m_parent);
            break;
    }
}

void GuiMultiplayerWelcome::openKeyLocation() {
    std::filesystem::path keyPath = std::filesystem::current_path() / "accounts.json";
    std::string folder = keyPath.parent_path().string();

    #ifdef _WIN32
        std::string cmd = "explorer \"" + folder + "\"";
        system(cmd.c_str());
    #elif __APPLE__
        std::string cmd = "open \"" + folder + "\"";
        system(cmd.c_str());
    #else
        std::string cmd = "xdg-open \"" + folder + "\"";
        system(cmd.c_str());
    #endif
}