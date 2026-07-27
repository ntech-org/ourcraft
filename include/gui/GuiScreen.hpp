#pragma once
#include "gui/Gui.hpp"
#include "gui/GuiButton.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>

class Minecraft;

class GuiScreen : public Gui, public std::enable_shared_from_this<GuiScreen> {
public:
    virtual ~GuiScreen() = default;

    virtual void initGui() {}
    virtual void onGuiClosed();
    virtual void updateScreen() {}
    virtual void drawScreen(int mouseX, int mouseY, float partialTicks);
    
    virtual bool doesGuiPauseGame() const { return true; }
    virtual bool wantsCursor() const { return true; }
    
    virtual void handleEvent(const SDL_Event& event);
    virtual void keyTyped(SDL_Keycode key, SDL_Scancode scancode, bool down);
    virtual void onTextInput(const char* text);
    virtual void mouseClicked(int mouseX, int mouseY, int button);
    virtual void mouseReleased(int mouseX, int mouseY, int button);
    
    void setWorldAndResolution(Minecraft* mc, float width, float height);
    
    static bool isCtrlKeyDown();
    static bool isShiftKeyDown();

    void drawPanel(Shader& shader, float x1, float y1, float x2, float y2, uint32_t color = 0xD0101010);

    void drawString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);
    void drawCenteredString(Font& font, Shader& shader, const std::string& text, float x, float y, uint32_t color);

protected:
    virtual void actionPerformed(GuiButton* button) {}
    
    void drawDefaultBackground();

    Minecraft* mc = nullptr;
    float width, height;
    std::vector<std::unique_ptr<GuiButton>> controlList;

public:
    std::shared_ptr<GuiScreen> parentScreen = nullptr;
};
