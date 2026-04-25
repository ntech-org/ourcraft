#include "InputHandler.hpp"
#include <algorithm>
#include <iostream>

InputHandler::InputHandler(GLFWwindow* window, EntityPlayer& player)
    : m_window(window), m_player(player) 
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    m_lastX = width / 2.0f;
    m_lastY = height / 2.0f;
}

void InputHandler::update() {
    m_player.moveForward = 0.0f;
    m_player.moveStrafe = 0.0f;
    
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_player.moveForward += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_player.moveForward -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_player.moveStrafe += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_player.moveStrafe -= 1.0f;
    
    m_player.jumping = (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // F3 and F3 + Keys
    bool f3 = glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS;
    bool g = glfwGetKey(m_window, GLFW_KEY_G) == GLFW_PRESS;
    bool p = glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS;
    bool a = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;

    if (f3) {
        if (g && !m_gPressed) {
            m_showChunkBoundaries = !m_showChunkBoundaries;
            std::cout << "[Debug] Chunk boundaries: " << (m_showChunkBoundaries ? "ON" : "OFF") << std::endl;
        }
        if (p && !m_pPressed) {
            m_showProfiler = !m_showProfiler;
            std::cout << "[Debug] Profiler: " << (m_showProfiler ? "ON" : "OFF") << std::endl;
        }
        if (a && !m_aPressed) {
            m_reloadChunks = true;
            std::cout << "[Debug] Reloading all chunks..." << std::endl;
        }
    } else {
        if (m_f3Pressed) {
            if (!m_gPressed && !m_pPressed && !m_aPressed) {
                m_showDebug = !m_showDebug;
            }
        }
    }
    
    m_f3Pressed = f3;
    m_gPressed = g;
    m_pPressed = p;
    m_aPressed = a;

    // F5 Toggle
    bool f5 = glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_PRESS;
    if (f5 && !m_f5Pressed) {
        m_cameraMode = (m_cameraMode + 1) % 3;
    }
    m_f5Pressed = f5;

    // Swing logic
    bool leftMouse = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftMouse && !m_leftMousePressed) {
        m_player.swing();
    }
    m_leftMousePressed = leftMouse;
}

void InputHandler::handleKey(int key, int scancode, int action, int mods) {
    // Optional: add more robust key handling here if needed
}

void InputHandler::handleMouseButton(int button, int action, int mods) {
}

void InputHandler::handleMouse(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastX;
    float yoffset = m_lastY - ypos;

    m_lastX = xpos;
    m_lastY = ypos;

    float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_player.rotationYaw += xoffset;
    m_player.rotationPitch += yoffset;

    m_player.rotationPitch = std::clamp(m_player.rotationPitch, -89.9f, 89.9f);
}
