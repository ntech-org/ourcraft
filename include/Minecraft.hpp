#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "world/World.hpp"
#include "util/Timer.hpp"
#include "entities/EntityPlayer.hpp"
#include "renderer/GameRenderer.hpp"
#include "InputHandler.hpp"
#include "net/NetworkHandler.hpp"

class Minecraft {
public:
    Minecraft(GLFWwindow* window, int width, int height);
    ~Minecraft();

    void run();
    void resize(int width, int height);
    void mouseCallback(double xpos, double ypos);
    void scrollCallback(double xoffset, double yoffset);

private:

    void init();
    void tick();

    GLFWwindow* m_window;
    int m_width;
    int m_height;

    std::unique_ptr<World> m_world;
    std::unique_ptr<EntityPlayer> m_player;
    std::unique_ptr<GameRenderer> m_gameRenderer;
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<NetworkHandler> m_networkHandler;

    Timer m_timer;
    double m_lastFrameTime = 0.0;
    float m_fps = 0.0f;
    bool m_running = true;
};
