#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "Minecraft.hpp"

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void init();
    void cleanup();

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* m_window = nullptr;
    int m_width = 854;
    int m_height = 480;

    std::unique_ptr<Minecraft> m_game;
};
