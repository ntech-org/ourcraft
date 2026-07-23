#pragma once
#include "gui/GuiButton.hpp"
#include <vector>
#include <string>

class GuiOptionButton : public GuiButton {
public:
    GuiOptionButton(int id, int x, int y, int width, int height,
                    const std::string& label, const std::vector<std::string>& options, int currentIndex = 0);

    void drawButton(Minecraft* mc, Font& font, Shader& shader, int mouseX, int mouseY) override;
    bool mousePressed(int mouseX, int mouseY) override;

    int getCurrentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int idx);

    std::function<void(int)> onValueChange;

private:
    void updateText();
    std::string m_label;
    std::vector<std::string> m_options;
    int m_currentIndex = 0;
};
