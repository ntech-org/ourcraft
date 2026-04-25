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
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    void mouseCallback(double xpos, double ypos);
    void scrollCallback(double xoffset, double yoffset);
    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;

    int m_width = 854;
    int m_height = 480;

    std::unique_ptr<Minecraft> m_game;
};
