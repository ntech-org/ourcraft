#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Frustum.hpp"
#include "world/World.hpp"
#include "util/Timer.hpp"
#include "entities/EntityPlayer.hpp"

class Minecraft {
public:
    Minecraft(GLFWwindow* window, int width, int height);
    ~Minecraft();

    void run();
    void resize(int width, int height);
    void mouseCallback(double xpos, double ypos);

private:
    void init();
    void tick();
    void render(float partialTicks);
    void handleInput();

    GLFWwindow* m_window;
    int m_width;
    int m_height;

    std::unique_ptr<RenderEngine> m_renderEngine;
    std::unique_ptr<WorldRenderer> m_worldRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<World> m_world;
    std::unique_ptr<EntityPlayer> m_player;
    
    Timer m_timer;
    Camera m_camera;
    Frustum m_frustum;

    bool m_firstMouse = true;
    float m_lastX;
    float m_lastY;

    int m_terrainTex = 0;
    double m_titleTimer = 0.0;
    double m_lastFrameTime = 0.0;
    bool m_running = true;
};
