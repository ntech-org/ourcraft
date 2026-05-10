#pragma once

#include <glad/glad.h>
#include <SDL3/SDL.h>
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

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    int m_width = 854;
    int m_height = 480;

    std::unique_ptr<Minecraft> m_game;
};
