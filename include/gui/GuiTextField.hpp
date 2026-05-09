#pragma once
#include "gui/Gui.hpp"
#include "renderer/ModernFont.hpp"
#include <string>

class Minecraft;
class Shader;

class GuiTextField : public Gui {
public:
    GuiTextField(int id, int x, int y, int width, int height);
    virtual ~GuiTextField() = default;

    void updateCursorCounter();
    void setText(const std::string& text);
    std::string getText() const;
    
    void setFocused(bool focused);
    bool isFocused() const;

    void keyTyped(int key, int scancode, int action, int mods);
    void mouseClicked(int mouseX, int mouseY, int button);
    void drawTextField(Minecraft* mc, Font& font, Shader& shader);

    int id;
    int x, y;
    int width, height;
    bool visible = true;
    int maxStringLength = 32;

private:
    std::string m_text;
    bool m_isFocused = false;
    int m_cursorCounter = 0;
};
