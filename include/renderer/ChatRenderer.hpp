#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Font;
class Shader;
class Minecraft;

struct ChatMessage {
    std::string sender;
    std::string message;
    int64_t timestamp;
    float opacity = 1.0f;
};

struct ChatVisualLine {
    std::string text;
    uint32_t color;
    float opacity;
};

class ChatRenderer {
public:
    ChatRenderer();

    void addMessage(const std::string& sender, const std::string& message, int64_t timestamp);
    void addSystemMessage(const std::string& message);
    void render(Minecraft& mc, Font& font, Shader& textShader, Shader& uiShader, float scaledWidth, float scaledHeight);
    void tick();

    void scroll(int lines);
    void setChatOpen(bool open);

    int getMessageCount() const { return (int)m_messages.size(); }

private:
    std::vector<ChatVisualLine> buildVisualLines(Font& font, float maxWidth) const;

    std::vector<ChatMessage> m_messages;
    int m_maxMessages = 200;
    int m_defaultLines = 10;
    int m_openLines = 20;
    float m_lineHeight = 9.0f;
    float m_messageStayTicks = 200.0f;
    float m_fadeOutTicks = 100.0f;
    float m_bgAlpha = 0.5f;

    int m_scrollOffset = 0;
    bool m_chatOpen = false;
};
