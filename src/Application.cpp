#include "Application.hpp"
#include <stdexcept>

Application::Application() {
    init();
}

Application::~Application() {
    cleanup();
}

void Application::init() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "OurCraft - Infdev Port", NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetKeyCallback(m_window, key_callback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    m_game = std::make_unique<Minecraft>(m_window, m_width, m_height);
}

void Application::cleanup() {
    m_game.reset();
    glfwTerminate();
}

void Application::run() {
    m_game->run();
}

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_width = width;
    app->m_height = height;
    if (app->m_game) {
        app->m_game->resize(width, height);
    }
}

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app->m_game) {
        app->mouseCallback(xpos, ypos);
    }
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app->m_game) {
        app->scrollCallback(xoffset, yoffset);
    }
}

void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app->m_game) {
        app->mouseButtonCallback(button, action, mods);
    }
}

void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app->m_game) {
        app->keyCallback(key, scancode, action, mods);
    }
}

void Application::mouseCallback(double xpos, double ypos) {
    if (m_game) m_game->mouseCallback(xpos, ypos);
}

void Application::scrollCallback(double xoffset, double yoffset) {
    if (m_game) m_game->scrollCallback(xoffset, yoffset);
}

void Application::mouseButtonCallback(int button, int action, int mods) {
    if (m_game) m_game->mouseButtonCallback(button, action, mods);
}

void Application::keyCallback(int key, int scancode, int action, int mods) {
    if (m_game) m_game->keyCallback(key, scancode, action, mods);
}
