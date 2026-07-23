#pragma once

#include <SDL3/SDL.h>
#include "entities/EntityPlayer.hpp"
#include "util/GameSettings.hpp"

class InputHandler {
public:
    InputHandler(SDL_Window* window, EntityPlayer& player, GameSettings& settings);

    void update();
    void handleEvent(const SDL_Event& event);
    void releaseAllButtons() { m_leftMousePressed = false; m_rightMousePressed = false; m_leftClick = false; m_rightClick = false; }

    bool isDebugVisible() const { return m_showDebug; }

    bool isChunkBoundariesVisible() const { return m_showChunkBoundaries; }
    bool isProfilerVisible() const { return m_showProfiler; }
    bool shouldReloadChunks() { bool r = m_reloadChunks; m_reloadChunks = false; return r; }
    int getCameraMode() const { return m_cameraMode; }

    bool isLeftClick() { bool r = m_leftClick; m_leftClick = false; return r; }
    bool isRightClick() { bool r = m_rightClick; m_rightClick = false; return r; }
    bool isLeftMouseDown() const { return m_leftMousePressed; }
    bool isRightMouseDown() const { return m_rightMousePressed; }
    bool shouldToggleInventory() { bool r = m_inventoryPressed; m_inventoryPressed = false; return r; }
    bool shouldOpenChat() { bool r = m_chatPressed; m_chatPressed = false; return r; }
    bool isEscPressed() { bool r = m_escPressed; m_escPressed = false; return r; }

private:
    SDL_Window* m_window;
    EntityPlayer& m_player;
    GameSettings& m_settings;

    bool m_showDebug = false;
    bool m_showChunkBoundaries = false;
    bool m_showProfiler = false;
    bool m_reloadChunks = false;
    int m_cameraMode = 0; // 0 = 1st person, 1 = 3rd person back, 2 = 3rd person front
    
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    bool m_leftClick = false;
    bool m_rightClick = false;
    bool m_escPressed = false;
    bool m_inventoryPressed = false;
    bool m_chatPressed = false;

    int m_spaceTapTicks = 0;
    bool m_spaceHeld = false;
};
