#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Frustum.hpp"
#include "world/World.hpp"

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void init();
    void cleanup();
    void mainLoop();
    void update(float deltaTime);
    void render();
    void handleInput();

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* m_window = nullptr;
    int m_width = 854;
    int m_height = 480;

    std::unique_ptr<RenderEngine> m_renderEngine;
    std::unique_ptr<WorldRenderer> m_worldRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<World> m_world;
    Camera m_camera;
    Frustum m_frustum;

    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;
    bool m_firstMouse = true;
    float m_lastX = 427.0f;
    float m_lastY = 240.0f;

    int m_terrainTex = 0;
    double m_titleTimer = 0.0;
};
