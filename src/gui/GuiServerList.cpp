#include "gui/GuiServerList.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiConnecting.hpp"
#include "gui/GuiAddServer.hpp"
#include "gui/GuiAccountSelect.hpp"
#include "Minecraft.hpp"
#include <algorithm>

GuiServerList::GuiServerList(std::shared_ptr<GuiScreen> parent) : m_parent(parent) {}

void GuiServerList::initGui() {
    controlList.clear();
    controlList.push_back(std::make_unique<GuiButton>(1, width / 2 - 100, height - 35, 200, 20, "Add Server"));
    controlList.push_back(std::make_unique<GuiButton>(0, width / 2 - 100, height - 60, 200, 20, "Back"));
    controlList.push_back(std::make_unique<GuiButton>(2, width - 110, 15, 95, 20, "Account"));
    controlList.push_back(std::make_unique<GuiButton>(3, 15, 15, 95, 20, "Edit"));
    controlList.push_back(std::make_unique<GuiButton>(4, 15, 45, 95, 20, "Delete"));
    m_scrollOffset = 0;
    m_selectedServer = -1;
}

void GuiServerList::updateScreen() {
    int maxScroll = getMaxScroll();
    m_scrollOffset = std::max(0, std::min(m_scrollOffset, maxScroll));
}

void GuiServerList::drawScreen(int mouseX, int mouseY, float partialTicks) {
    drawDefaultBackground();

    Shader& uiShader = mc->getGameRenderer().getUIShader();
    Shader& textShader = mc->getGameRenderer().getTextShader();
    Font& font = mc->getFont();

    glDisable(GL_DEPTH_TEST);
    
    // Top bar
    drawRect(uiShader, 0, 0, width, 50, 0xFF101010);
    drawCenteredString(font, textShader, "Multiplayer", (float)width / 2, 15, 0xFFFFFFFF);

    // Bottom bar
    drawRect(uiShader, 0, height - 70, width, height, 0xFF101010);

    // Server list area with margins
    int listLeft = width / 4;
    int listRight = width * 3 / 4;
    int listTop = 60;
    int listBottom = height - 80;
    int listHeight = listBottom - listTop;
    int entryHeight = 40;
    int visibleEntries = listHeight / entryHeight;

    auto& serverList = mc->getSettings().serverList;

    // Draw list background
    drawRect(uiShader, listLeft - 4, listTop - 4, listRight + 4, listBottom + 4, 0xFF101010);
    drawRect(uiShader, listLeft, listTop, listRight, listBottom, 0xFF000000);

    for (int i = 0; i < visibleEntries; ++i) {
        int idx = m_scrollOffset + i;
        if (idx >= (int)serverList.size()) break;

        int y = listTop + i * entryHeight;
        const auto& server = serverList[idx];

        bool isSelected = (idx == m_selectedServer);
        bool isHovered = mouseX >= listLeft && mouseX <= listRight && mouseY >= y && mouseY <= y + entryHeight;

        // Selection highlight
        if (isSelected || isHovered) {
            drawRect(uiShader, listLeft, y, listRight, y + entryHeight, isSelected ? 0xFF404060 : 0xFF202030);
        }

        // Server name
        drawString(font, textShader, server.name, (float)listLeft + 12, (float)y + 6, 0xFFFFFFFF);

        // Address and alias
        std::string detail = server.address + ":" + std::to_string(server.port);
        if (!server.alias.empty()) {
            detail += "  (as " + server.alias + ")";
        }
        drawString(font, textShader, detail, (float)listLeft + 12, (float)y + 22, 0xFFA0A0A0);
    }

    // Draw scrollbar if needed
    int maxScroll = getMaxScroll();
    if (maxScroll > 0) {
        int trackTop = listTop;
        int trackBottom = listBottom;
        int trackHeight = trackBottom - trackTop;
        int thumbHeight = std::max(30, trackHeight * visibleEntries / (visibleEntries + maxScroll));
        int thumbTop = trackTop + (trackHeight - thumbHeight) * m_scrollOffset / maxScroll;

        int scrollbarX = listRight + 8;
        drawRect(uiShader, scrollbarX, trackTop, scrollbarX + 6, trackBottom, 0xFF202020);
        drawRect(uiShader, scrollbarX, thumbTop, scrollbarX + 6, thumbTop + thumbHeight, 0xFF606060);
    }

    // Draw buttons
    for (auto& btn : controlList) {
        btn->drawButton(mc, font, uiShader, mouseX, mouseY);
    }

    glEnable(GL_DEPTH_TEST);
}

void GuiServerList::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (key == SDLK_UP && down) {
        if (m_selectedServer > 0) m_selectedServer--;
        if (m_selectedServer < m_scrollOffset) m_scrollOffset = m_selectedServer;
    } else if (key == SDLK_DOWN && down) {
        if (m_selectedServer < (int)mc->getSettings().serverList.size() - 1) m_selectedServer++;
        if (m_selectedServer >= m_scrollOffset + (height - 140) / 40) m_scrollOffset = m_selectedServer - (height - 140) / 40 + 1;
    } else if (key == SDLK_RETURN && down) {
        if (m_selectedServer >= 0) joinSelectedServer();
    } else if (key == SDLK_DELETE && down) {
        if (m_selectedServer >= 0) {
            mc->getSettings().serverList.erase(mc->getSettings().serverList.begin() + m_selectedServer);
            mc->getSettings().saveOptions();
            m_selectedServer = -1;
        }
    } else if (key == SDLK_E && down) {
        if (m_selectedServer >= 0) {
            mc->displayGuiScreen(std::make_shared<GuiAddServer>(shared_from_this(), m_selectedServer));
        }
    } else {
        GuiScreen::keyTyped(key, scancode, down);
    }
}

void GuiServerList::mouseClicked(int mouseX, int mouseY, int button) {
    int listLeft = width / 4;
    int listRight = width * 3 / 4;
    int listTop = 60;
    int entryHeight = 40;

    if (button == SDL_BUTTON_LEFT && mouseX >= listLeft && mouseX <= listRight && mouseY >= listTop) {
        int idx = m_scrollOffset + (mouseY - listTop) / entryHeight;
        if (idx >= 0 && idx < (int)mc->getSettings().serverList.size()) {
            int now = (int)SDL_GetTicks();
            if (now - m_lastClickTime < 300 && idx == m_lastClickServer) {
                // Double click
                joinSelectedServer();
            }
            m_selectedServer = idx;
            m_lastClickTime = now;
            m_lastClickServer = idx;
        }
    }

    // Scrollbar click
    int maxScroll = getMaxScroll();
    if (maxScroll > 0 && button == SDL_BUTTON_LEFT) {
        int trackTop = 60;
        int trackBottom = height - 80;
        int trackHeight = trackBottom - trackTop;
        int scrollbarX = listRight + 8;
        if (mouseX >= scrollbarX && mouseX <= scrollbarX + 6 && mouseY >= trackTop && mouseY <= trackBottom) {
            m_scrollOffset = std::max(0, std::min(maxScroll, (mouseY - trackTop) * maxScroll / trackHeight));
        }
    }

    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiServerList::actionPerformed(GuiButton* button) {
    switch (button->id) {
        case 0: // Back
            mc->displayGuiScreen(m_parent);
            break;
        case 1: // Add Server
            mc->displayGuiScreen(std::make_shared<GuiAddServer>(shared_from_this(), -1));
            break;
        case 2: // Account
            mc->displayGuiScreen(std::make_shared<GuiAccountSelect>(shared_from_this()));
            break;
        case 3: // Edit
            if (m_selectedServer >= 0) {
                mc->displayGuiScreen(std::make_shared<GuiAddServer>(shared_from_this(), m_selectedServer));
            }
            break;
        case 4: // Delete
            if (m_selectedServer >= 0) {
                mc->getSettings().serverList.erase(mc->getSettings().serverList.begin() + m_selectedServer);
                mc->getSettings().saveOptions();
                m_selectedServer = -1;
            }
            break;
    }
}

int GuiServerList::getMaxScroll() const {
    int listHeight = height - 140;
    int entryHeight = 40;
    int visibleEntries = listHeight / entryHeight;
    return std::max(0, (int)mc->getSettings().serverList.size() - visibleEntries);
}

void GuiServerList::joinSelectedServer() {
    if (m_selectedServer >= 0 && m_selectedServer < (int)mc->getSettings().serverList.size()) {
        const auto& server = mc->getSettings().serverList[m_selectedServer];
        mc->displayGuiScreen(std::make_shared<GuiConnecting>(shared_from_this(), server.address, server.port));
    }
}
