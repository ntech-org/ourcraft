#include "gui/GuiChat.hpp"
#include "Minecraft.hpp"
#include "net/NetworkHandler.hpp"
#include "net/Packets.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/World.hpp"
#include "renderer/ChatRenderer.hpp"
#include <algorithm>

GuiChat::GuiChat() {}
GuiChat::~GuiChat() { delete m_inputField; }

void GuiChat::initGui() {
    delete m_inputField;
    m_inputField = new GuiTextField(0, 2, height - 14, width - 4, 12, mc->getWindow());
    m_inputField->maxStringLength = 256;
    m_inputField->setFocused(true, mc->getWindow());
    mc->getChatRenderer().setChatOpen(true);
}

void GuiChat::onGuiClosed() {
    if (m_inputField) {
        m_inputField->setFocused(false, mc->getWindow());
    }
    mc->getChatRenderer().setChatOpen(false);
}

void GuiChat::updateScreen() {
    if (m_inputField) {
        m_inputField->updateCursorCounter();
    }
}

void GuiChat::drawScreen(int mouseX, int mouseY, float partialTicks) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_inputField) {
        m_inputField->drawTextField(mc, mc->getFont(), mc->getGameRenderer().getTextShader());
    }

    if (!m_tabCompletions.empty()) {
        float x = 2.0f;
        float y = (float)height - 28.0f;
        for (size_t i = 0; i < m_tabCompletions.size() && i < 8; ++i) {
            std::string text = m_tabCompletions[i];
            uint32_t bg = (m_tabIndex >= 0 && (int)i == m_tabIndex) ? 0xCC333399 : 0xCC000000;
            Shader& uiShader = mc->getGameRenderer().getUIShader();
            Gui::drawRect(uiShader, x - 1.0f, y - 1.0f, x + 200.0f, y + 10.0f, bg);
            mc->getFont().drawString(mc->getGameRenderer().getTextShader(), text, x, y, 0xFFFFFFFF, false);
            y -= 11.0f;
        }
    }
}

void GuiChat::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_TEXT_INPUT && m_inputField && m_inputField->isFocused()) {
        m_tabCompletions.clear();
        m_tabIndex = -1;
        m_inputField->appendText(event.text.text);
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        if (event.wheel.y > 0) {
            mc->getChatRenderer().scroll(3);
        } else if (event.wheel.y < 0) {
            mc->getChatRenderer().scroll(-3);
        }
        return;
    }

    GuiScreen::handleEvent(event);
}

void GuiChat::keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down) {
    if (!down) return;

    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        if (m_inputField && !m_inputField->getText().empty()) {
            std::string text = m_inputField->getText();

            if (mc->getNetworkHandler()) {
                PacketChatMessage packet;
                packet.sender = mc->getPlayer().username;
                packet.message = text;
                packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                mc->getNetworkHandler()->sendPacket(packet);
            }

            if (!text.empty()) {
                m_history.push_back(text);
            }
            m_historyIndex = -1;
        }
        mc->displayGuiScreen(nullptr);
        return;
    }

    if (key == SDLK_ESCAPE) {
        mc->displayGuiScreen(nullptr);
        return;
    }

    if (key == SDLK_TAB && m_inputField) {
        std::string text = m_inputField->getText();
        if (text.empty()) return;

        if (!m_tabCompletions.empty()) {
            m_tabIndex = (m_tabIndex + 1) % (int)m_tabCompletions.size();
            std::string completion = m_tabCompletions[m_tabIndex];
            if (completion[0] == '/') {
                auto space = text.find(' ');
                if (space != std::string::npos) {
                    m_inputField->setText(completion + text.substr(space));
                } else {
                    m_inputField->setText(completion + " ");
                }
            } else {
                auto lastSpace = text.find_last_of(' ');
                if (lastSpace != std::string::npos) {
                    m_inputField->setText(text.substr(0, lastSpace + 1) + completion);
                } else {
                    m_inputField->setText(completion + " ");
                }
            }
            return;
        }

        m_tabCompletions.clear();
        m_tabIndex = -1;

        if (mc->getNetworkHandler() && mc->getNetworkHandler()->isSingleplayer()) {
            std::string prefix;
            auto lastSpace = text.find_last_of(' ');
            if (lastSpace != std::string::npos) {
                prefix = text.substr(lastSpace + 1);
            } else {
                prefix = text;
                if (prefix[0] == '/') prefix = prefix.substr(1);
            }

            if (text[0] == '/') {
                auto cmdHandler = mc->getNetworkHandler()->getCommandHandler();
                if (cmdHandler) {
                    m_tabCompletions = cmdHandler->getCompletions(prefix);
                }
            } else {
                for (const auto& entity : mc->getWorld()->getEntities()) {
                    if (auto* player = dynamic_cast<EntityPlayer*>(entity.get())) {
                        if (player->entityID != mc->getPlayer().entityID &&
                            player->username.size() >= prefix.size() &&
                            player->username.compare(0, prefix.size(), prefix) == 0) {
                            m_tabCompletions.push_back(player->username);
                        }
                    }
                }
            }
        }

        if (!m_tabCompletions.empty()) {
            m_tabIndex = 0;
            std::string completion = m_tabCompletions[0];
            if (text[0] == '/') {
                auto space = text.find(' ');
                if (space != std::string::npos) {
                    m_inputField->setText(completion + text.substr(space));
                } else {
                    m_inputField->setText(completion + " ");
                }
            } else {
                auto lastSpace = text.find_last_of(' ');
                if (lastSpace != std::string::npos) {
                    m_inputField->setText(text.substr(0, lastSpace + 1) + completion);
                } else {
                    m_inputField->setText(completion + " ");
                }
            }
        }
        return;
    }

    if (key == SDLK_UP) {
        if (!m_history.empty()) {
            if (m_historyIndex == -1) {
                m_currentInput = m_inputField->getText();
                m_historyIndex = (int)m_history.size() - 1;
            } else if (m_historyIndex > 0) {
                m_historyIndex--;
            }
            m_inputField->setText(m_history[m_historyIndex]);
        }
        return;
    }

    if (key == SDLK_DOWN) {
        if (m_historyIndex != -1) {
            if (m_historyIndex < (int)m_history.size() - 1) {
                m_historyIndex++;
                m_inputField->setText(m_history[m_historyIndex]);
            } else {
                m_historyIndex = -1;
                m_inputField->setText(m_currentInput);
            }
        }
        return;
    }

    if (m_inputField) {
        m_inputField->keyTyped(key, scancode, true);
    }
}
