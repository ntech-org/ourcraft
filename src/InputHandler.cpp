#include "InputHandler.hpp"
#include "world/World.hpp"
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
    
    for (int i = 0; i < 9; ++i) {
        if (glfwGetKey(m_window, GLFW_KEY_1 + i) == GLFW_PRESS) m_player.inventory.setSlot(i);
    }
    
    bool space = (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (space && !m_spacePressed) {
        if (m_player.gameMode == GameMode::CREATIVE) {
            if (m_spaceTapTicks > 0) {
                m_player.isFlying = !m_player.isFlying;
                if (m_player.isFlying) m_player.motionY = 0;
                m_spaceTapTicks = 0;
            } else {

                m_spaceTapTicks = 10; // 0.5s at 20tps
            }
        }
    }
    if (m_spaceTapTicks > 0) m_spaceTapTicks--;
    m_spacePressed = space;

    m_player.jumping = space;
    m_player.sneaking = (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    m_player.sprinting = (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);



    // F3 and F3 + Keys

    bool f3 = glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS;
    bool g = glfwGetKey(m_window, GLFW_KEY_G) == GLFW_PRESS;
    bool p = glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS;
    bool a = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
    bool f4 = glfwGetKey(m_window, GLFW_KEY_F4) == GLFW_PRESS;
    bool t = glfwGetKey(m_window, GLFW_KEY_T) == GLFW_PRESS;
    bool y = glfwGetKey(m_window, GLFW_KEY_Y) == GLFW_PRESS;

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
        if (f4 && !m_f4Pressed) {
            m_player.gameMode = (m_player.gameMode == GameMode::SURVIVAL) ? GameMode::CREATIVE : GameMode::SURVIVAL;
            if (m_player.gameMode == GameMode::SURVIVAL) m_player.isFlying = false;
            std::cout << "[Debug] Gamemode: " << (m_player.gameMode == GameMode::SURVIVAL ? "SURVIVAL" : "CREATIVE") << std::endl;
        }

        if (t && !m_tPressed) {
            m_player.worldObj.setWorldTime(m_player.worldObj.getWorldTime() + 1000.0);
            std::cout << "[Debug] Advanced time +1000" << std::endl;
        }
        if (y && !m_yPressed) {
            m_player.worldObj.setWorldTime(0.0);
            std::cout << "[Debug] Reset time to 0" << std::endl;
        }
    } else {
        if (m_f3Pressed) {
            if (!m_gPressed && !m_pPressed && !m_aPressed && !m_f4Pressed && !m_tPressed && !m_yPressed) {
                m_showDebug = !m_showDebug;
            }
        }
    }
    
    m_f3Pressed = f3;
    m_gPressed = g;
    m_pPressed = p;
    m_aPressed = a;
    m_f4Pressed = f4;
    m_tPressed = t;
    m_yPressed = y;

    // F5 Toggle
    bool f5 = glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_PRESS;
    if (f5 && !m_f5Pressed) {
        m_cameraMode = (m_cameraMode + 1) % 3;
    }
    m_f5Pressed = f5;

    // Interaction logic
    bool leftMouse = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftMouse && !m_leftMousePressed) {
        m_leftClick = true;
    }
    m_leftMousePressed = leftMouse;


    bool rightMouse = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (rightMouse && !m_rightMousePressed) {
        m_rightClick = true;
    }
    m_rightMousePressed = rightMouse;
}


void InputHandler::handleKey(int key, int scancode, int action, int mods) {
    // Optional: add more robust key handling here if needed
}

void InputHandler::handleMouseButton(int button, int action, int mods) {
}

void InputHandler::handleScroll(double xoffset, double yoffset) {
    if (yoffset > 0) m_player.inventory.prevSlot();
    else if (yoffset < 0) m_player.inventory.nextSlot();
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
