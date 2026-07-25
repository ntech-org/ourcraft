#include "Application.hpp"
#include <stdexcept>
#include <iostream>

Application::Application() {
    init();
}

Application::~Application() {
    cleanup();
}

void Application::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow("OurCraft - Infdev Port", m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to create SDL GL context: " + std::string(SDL_GetError()));
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1); // VSync ON

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    int w, h;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    m_width = w;
    m_height = h;
    glViewport(0, 0, m_width, m_height);

    m_game = std::make_unique<Minecraft>(m_window, m_width, m_height);
}

void Application::cleanup() {
    m_game.reset();
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Application::run() {
    m_game->run();
}
