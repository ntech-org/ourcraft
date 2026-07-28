#include "gui/GuiWorldList.hpp"
#include "gui/GuiWorldSettings.hpp"
#include "Minecraft.hpp"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

GuiWorldList::GuiWorldList(std::shared_ptr<GuiScreen> parent) : m_parent(std::move(parent)) {}

void GuiWorldList::initGui() {
    controlList.clear();
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 155, height - 34, 100, 22, "Play Selected"));
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 50, height - 34, 100, 22, "Create New"));
    controlList.push_back(std::make_unique<GuiButton>(2, width / 2 + 55, height - 34, 100, 22, "Delete"));
    controlList.push_back(std::make_unique<GuiButton>(3, 12, 12, 78, 22, "Back"));
    refreshWorlds();
}

void GuiWorldList::refreshWorlds() {
    m_worlds.clear();
    const fs::path root("worlds");
    if (fs::exists(root)) {
        for (const auto& entry : fs::directory_iterator(root)) {
            if (entry.is_directory()) m_worlds.push_back(entry.path().filename().string());
        }
    }
    std::sort(m_worlds.begin(), m_worlds.end());
    if (m_selectedWorld >= (int)m_worlds.size()) m_selectedWorld = -1;
}

void GuiWorldList::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();
    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();
    glDisable(GL_DEPTH_TEST);

    drawCenteredString(font, textShader, "Select World", width / 2.0f, 16.0f, 0xFFFFFFFF);
    const int left = width / 2 - 180;
    const int right = width / 2 + 180;
    const int top = 50;
    const int bottom = height - 48;
    drawPanel(uiShader, left, top, right, bottom, 0xE0141416);

    if (m_worlds.empty()) {
        drawCenteredString(font, textShader, "No worlds yet", width / 2.0f, (top + bottom) / 2.0f - 6.0f, 0xFFB8B8B8);
        drawCenteredString(font, textShader, "Create a new world to begin playing.", width / 2.0f, (top + bottom) / 2.0f + 8.0f, 0xFF888888);
    }

    for (size_t i = 0; i < m_worlds.size(); ++i) {
        const int y = top + 8 + (int)i * 42;
        if (y + 36 > bottom) break;
        const bool selected = (int)i == m_selectedWorld;
        const bool hovered = mouseX >= left + 8 && mouseX < right - 8 && mouseY >= y && mouseY < y + 36;
        drawRect(uiShader, left + 8, y, right - 8, y + 36, selected ? 0xFF3E566D : hovered ? 0xFF292F35 : 0xFF202124);
        drawString(font, textShader, m_worlds[i], (float)left + 18, (float)y + 7, 0xFFFFFFFF);
        drawString(font, textShader, "Local world", (float)left + 18, (float)y + 21, 0xFF999999);
    }

    controlList[0]->enabled = m_selectedWorld >= 0;
    controlList[2]->enabled = m_selectedWorld >= 0;
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    glEnable(GL_DEPTH_TEST);
}

void GuiWorldList::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;
    if (key == SDLK_UP && m_selectedWorld > 0) --m_selectedWorld;
    else if (key == SDLK_DOWN && m_selectedWorld < (int)m_worlds.size() - 1) ++m_selectedWorld;
    else if ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && m_selectedWorld >= 0) playSelectedWorld();
    else if (key == SDLK_DELETE) deleteSelectedWorld();
    else GuiScreen::keyTyped(key, scancode, down);
}

void GuiWorldList::mouseClicked(int mouseX, int mouseY, int button) {
    if (button == SDL_BUTTON_LEFT) {
        const int top = 50;
        const int index = (mouseY - top - 8) / 42;
        if (mouseY >= top + 8 && index >= 0 && index < (int)m_worlds.size()) {
            m_selectedWorld = index;
        }
    }
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiWorldList::actionPerformed(GuiButton* button) {
    switch (button->id) {
        case 0: playSelectedWorld(); break;
        case 1: createWorld(); break;
        case 2: deleteSelectedWorld(); break;
        case 3: mc->displayGuiScreen(m_parent); break;
    }
}

void GuiWorldList::playSelectedWorld() {
    if (m_selectedWorld >= 0 && m_selectedWorld < (int)m_worlds.size()) {
        mc->startSingleplayer(m_worlds[m_selectedWorld]);
    }
}

void GuiWorldList::createWorld() {
    int number = 1;
    std::string name;
    do {
        name = "New World " + std::to_string(number++);
    } while (fs::exists(fs::path("worlds") / name));
    mc->displayGuiScreen(std::make_shared<GuiWorldSettings>(shared_from_this(), name));
}

void GuiWorldList::deleteSelectedWorld() {
    if (m_selectedWorld < 0 || m_selectedWorld >= (int)m_worlds.size()) return;
    fs::remove_all(fs::path("worlds") / m_worlds[m_selectedWorld]);
    refreshWorlds();
}
