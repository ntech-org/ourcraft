#include "renderer/ChatRenderer.hpp"
#include "renderer/GameRenderer.hpp"
#include "renderer/Shader.hpp"
#include "gui/Gui.hpp"
#include "Minecraft.hpp"
#include "util/UTF8.hpp"
#include <algorithm>
#include <cmath>

ChatRenderer::ChatRenderer() {}

void ChatRenderer::addMessage(const std::string& sender, const std::string& message, int64_t timestamp) {
    ChatMessage msg;
    msg.sender = sender;
    msg.message = message;
    msg.timestamp = timestamp;
    msg.opacity = 1.0f;
    m_messages.push_back(msg);

    while ((int)m_messages.size() > m_maxMessages) {
        m_messages.erase(m_messages.begin());
    }

    if (!m_chatOpen) {
        m_scrollOffset = 0;
    }
}

void ChatRenderer::addSystemMessage(const std::string& message) {
    ChatMessage msg;
    msg.sender = "";
    msg.message = message;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    msg.opacity = 1.0f;
    m_messages.push_back(msg);

    while ((int)m_messages.size() > m_maxMessages) {
        m_messages.erase(m_messages.begin());
    }

    if (!m_chatOpen) {
        m_scrollOffset = 0;
    }
}

void ChatRenderer::tick() {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto& msg : m_messages) {
        float age = (float)(now - msg.timestamp) / 50.0f;
        if (age > m_messageStayTicks) {
            msg.opacity -= 1.0f / m_fadeOutTicks;
            if (msg.opacity < 0.0f) msg.opacity = 0.0f;
        }
    }

    while (!m_messages.empty() && m_messages.front().opacity <= 0.0f) {
        m_messages.erase(m_messages.begin());
    }
}

void ChatRenderer::scroll(int lines) {
    m_scrollOffset += lines;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

void ChatRenderer::setChatOpen(bool open) {
    m_chatOpen = open;
    if (!open) {
        m_scrollOffset = 0;
    }
}

std::vector<ChatVisualLine> ChatRenderer::buildVisualLines(Font& font, float maxWidth) const {
    std::vector<ChatVisualLine> lines;

    for (const auto& msg : m_messages) {
        std::string fullText;
        if (!msg.sender.empty()) {
            fullText = "<" + msg.sender + "> " + msg.message;
        } else {
            fullText = msg.message;
        }

        uint32_t baseColor = 0x00FFFFFF;
        float alpha = msg.opacity;

        auto wrapLine = [&](const std::string& line) {
            std::vector<std::string> todo = {line};
            while (!todo.empty()) {
                std::string current = std::move(todo.back());
                todo.pop_back();

                if (current.empty()) {
                    lines.push_back({"", baseColor, alpha});
                    continue;
                }

                if (font.getStringWidth(current) <= (int)maxWidth) {
                    lines.push_back({current, baseColor, alpha});
                    continue;
                }

                std::u32string u32 = UTF8::toUTF32(current);
                int lastSpaceByte = -1;
                size_t prevByte = 0;
                size_t bytePos = 0;
                bool split = false;

                for (size_t i = 0; i < u32.size(); ++i) {
                    prevByte = bytePos;
                    if (u32[i] < 0x80) bytePos += 1;
                    else if (u32[i] < 0x800) bytePos += 2;
                    else if (u32[i] < 0x10000) bytePos += 3;
                    else bytePos += 4;

                    if (u32[i] == ' ') {
                        lastSpaceByte = (int)bytePos;
                    }

                    if (font.getStringWidth(current.substr(0, bytePos)) > (int)maxWidth) {
                        if (lastSpaceByte > 0) {
                            lines.push_back({current.substr(0, lastSpaceByte), baseColor, alpha});
                            todo.push_back(current.substr(lastSpaceByte + 1));
                        } else {
                            lines.push_back({current.substr(0, prevByte), baseColor, alpha});
                            todo.push_back(current.substr(prevByte));
                        }
                        split = true;
                        break;
                    }
                }

                if (!split) {
                    lines.push_back({current, baseColor, alpha});
                }
            }
        };

        size_t pos = 0;
        while (pos <= fullText.size()) {
            size_t nl = fullText.find('\n', pos);
            if (nl == std::string::npos) {
                wrapLine(fullText.substr(pos));
                break;
            }
            wrapLine(fullText.substr(pos, nl - pos));
            pos = nl + 1;
        }
    }

    return lines;
}

void ChatRenderer::render(Minecraft& mc, Font& font, Shader& textShader, Shader& uiShader, float scaledWidth, float scaledHeight) {
    if (m_messages.empty()) return;

    int maxLines = m_chatOpen ? m_openLines : m_defaultLines;
    float maxWidth = scaledWidth - 10.0f;

    std::vector<ChatVisualLine> allLines = buildVisualLines(font, maxWidth);
    if (allLines.empty()) return;

    int totalLines = (int)allLines.size();

    int scrollMax = std::max(0, totalLines - maxLines);
    if (m_scrollOffset > scrollMax) m_scrollOffset = scrollMax;

    int firstLine = std::max(0, totalLines - maxLines - m_scrollOffset);
    int lineCount = std::min(maxLines, totalLines - firstLine);

    if (lineCount <= 0) return;

    float x = 2.0f;
    float bgTop = scaledHeight - 48.0f - (float)(lineCount - 1) * m_lineHeight - 2.0f;
    float bgBottom = scaledHeight - 48.0f + m_lineHeight + 2.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t bgColor = ((uint32_t)(m_bgAlpha * 255.0f) << 24);
    Gui::drawRect(uiShader, x - 2.0f, bgTop, x + 322.0f, bgBottom, bgColor);

    for (int i = 0; i < lineCount; ++i) {
        const ChatVisualLine& line = allLines[firstLine + i];
        if (line.opacity <= 0.01f) continue;

        float y = bgTop + 2.0f + (float)i * m_lineHeight;

        uint32_t color = ((uint32_t)(line.opacity * 255.0f) << 24) | (line.color & 0x00FFFFFF);
        font.drawString(textShader, line.text, x, y, color, false);
    }

    glDisable(GL_BLEND);
}
