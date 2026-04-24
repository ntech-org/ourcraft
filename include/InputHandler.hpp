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

    bool isDebugVisible() const { return m_showDebug; }
    int getCameraMode() const { return m_cameraMode; }

private:
    GLFWwindow* m_window;
    EntityPlayer& m_player;

    bool m_firstMouse = true;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;

    bool m_showDebug = false;
    int m_cameraMode = 0; // 0 = 1st person, 1 = 3rd person back, 2 = 3rd person front
    
    bool m_f3Pressed = false;
    bool m_f5Pressed = false;
    bool m_leftMousePressed = false;
};
