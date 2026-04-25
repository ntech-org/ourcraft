#pragma once

#include <GLFW/glfw3.h>
#include "entities/EntityPlayer.hpp"

class InputHandler {
public:
    InputHandler(GLFWwindow* window, EntityPlayer& player);

    void update();
    void handleKey(int key, int scancode, int action, int mods);
    void handleMouse(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);
    void handleScroll(double xoffset, double yoffset);

    bool isDebugVisible() const { return m_showDebug; }

    bool isChunkBoundariesVisible() const { return m_showChunkBoundaries; }
    bool isProfilerVisible() const { return m_showProfiler; }
    bool shouldReloadChunks() { bool r = m_reloadChunks; m_reloadChunks = false; return r; }
    int getCameraMode() const { return m_cameraMode; }

    bool isLeftClick() { bool r = m_leftClick; m_leftClick = false; return r; }
    bool isRightClick() { bool r = m_rightClick; m_rightClick = false; return r; }
    bool isEscPressed() { bool r = m_escPressed; m_escPressed = false; return r; }

private:
    GLFWwindow* m_window;
    EntityPlayer& m_player;

    bool m_firstMouse = true;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;

    bool m_showDebug = false;
    bool m_showChunkBoundaries = false;
    bool m_showProfiler = false;
    bool m_reloadChunks = false;
    int m_cameraMode = 0; // 0 = 1st person, 1 = 3rd person back, 2 = 3rd person front
    
    bool m_f3Pressed = false;
    bool m_f5Pressed = false;
    bool m_f4Pressed = false;
    bool m_tPressed = false;
    bool m_yPressed = false;
    bool m_gPressed = false;
    bool m_pPressed = false;
    bool m_aPressed = false;
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    bool m_leftClick = false;
    bool m_rightClick = false;
    bool m_escPressed = false;
    bool m_escWasPressed = false; // To handle press/release

    bool m_spacePressed = false;
    int m_spaceTapTicks = 0;
};


