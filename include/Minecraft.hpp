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
#include "util/GameSettings.hpp"

enum class GameState {
    MainMenu,
    InGame,
    Paused
};

class GuiScreen;

class Minecraft {
public:
    Minecraft(GLFWwindow* window, int width, int height);
    ~Minecraft();

    void run();
    void resize(int width, int height);
    void mouseCallback(double xpos, double ypos);
    void scrollCallback(double xoffset, double yoffset);
    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    void displayGuiScreen(std::shared_ptr<GuiScreen> screen);
    void saveAndQuit();
    void startSingleplayer();
    void setGameState(GameState state) { m_gameState = state; }
    GameState getGameState() const { return m_gameState; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    GameRenderer& getGameRenderer() { return *m_gameRenderer; }
    GameSettings& getSettings() { return m_settings; }
    std::shared_ptr<GuiScreen> getCurrentScreen() { return m_currentScreen; }

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

    GameSettings m_settings;
    std::shared_ptr<GuiScreen> m_currentScreen;
    GameState m_gameState = GameState::MainMenu;

    Timer m_timer;
    double m_lastFrameTime = 0.0;
    float m_fps = 0.0f;
    bool m_running = true;
};
