#pragma once
#include "gui/Gui.hpp"
#include "gui/GuiButton.hpp"
#include <vector>
#include <memory>

class Minecraft;

class GuiScreen : public Gui, public std::enable_shared_from_this<GuiScreen> {
public:
    virtual ~GuiScreen() = default;

    virtual void initGui() {}
    virtual void onGuiClosed() {}
    virtual void updateScreen() {}
    virtual void drawScreen(int mouseX, int mouseY, float partialTicks);
    
    virtual bool doesGuiPauseGame() const { return true; }
    
    virtual void keyTyped(int key, int scancode, int action, int mods);
    virtual void mouseClicked(int mouseX, int mouseY, int button);
    
    void setWorldAndResolution(Minecraft* mc, float width, float height);
    
    static bool isCtrlKeyDown();
    static bool isShiftKeyDown();

    void drawString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    void drawCenteredString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);

    std::shared_ptr<GuiScreen> parentScreen = nullptr;

protected:
    virtual void actionPerformed(GuiButton* button) {}
    
    void drawDefaultBackground();

    Minecraft* mc = nullptr;
    float width, height;
    std::vector<std::unique_ptr<GuiButton>> controlList;
};
