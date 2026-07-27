#pragma once

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <memory>
#include "world/World.hpp"
#include "util/Timer.hpp"
#include "entities/EntityPlayer.hpp"
#include "renderer/GameRenderer.hpp"
#include "renderer/ModernFont.hpp"
#include "InputHandler.hpp"
#include "net/NetworkHandler.hpp"
#include "util/GameSettings.hpp"
#include "sound/SoundSystem.hpp"
#include "sound/SoundPool.hpp"
#include "renderer/ChatRenderer.hpp"
#include "simulation/BlockBreakingSystem.hpp"
#include "simulation/SoundManager.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

enum class GameState {
    MainMenu,
    InGame,
    Paused
};

class GuiScreen;

class Minecraft {
public:
    Minecraft(SDL_Window* window, int width, int height);
    ~Minecraft();

    void run();
    void resize(int width, int height);
    void handleEvent(const SDL_Event& event);

    void displayGuiScreen(std::shared_ptr<GuiScreen> screen);
    void saveAndQuit();
    void startSingleplayer(const std::string& worldName = "world");
    void startMultiplayer(const std::string& address, int port);
    void setGameState(GameState state) { m_gameState = state; }
    GameState getGameState() const { return m_gameState; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    SDL_Window* getWindow() { return m_window; }
    World* getWorld() { return m_world.get(); }
    EntityPlayer& getPlayer() { return *m_player; }
    NetworkHandler* getNetworkHandler() { return m_networkHandler.get(); }
    GameRenderer& getGameRenderer() { return *m_gameRenderer; }
    GameSettings& getSettings() { return m_settings; }
    Font& getFont() { return *m_font; }
    std::shared_ptr<GuiScreen> getCurrentScreen() { return m_currentScreen; }
    InputHandler& getInputHandler() { return *m_inputHandler; }

    SoundSystem& getSoundSystem() { return *m_soundSystem; }
    bool hasSoundSystem() const { return m_soundSystem != nullptr; }
    SoundPool& getSoundPool() { return m_soundPool; }
    ChatRenderer& getChatRenderer() { return m_chatRenderer; }

    BlockBreakingSystem& getBlockBreaking() { return m_blockBreaking; }
    SoundManager& getSoundMgr() { return m_soundMgr; }

private:
    void init();
    void tick();
    void setupPlayerCallbacks();

    SDL_Window* m_window;
    int m_width;
    int m_height;

    std::unique_ptr<World> m_world;
    std::unique_ptr<EntityPlayer> m_player;
    std::unique_ptr<GameRenderer> m_gameRenderer;
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<NetworkHandler> m_networkHandler;
    std::unique_ptr<Font> m_font;
    FT_Library m_ft = nullptr;

    GameSettings m_settings;
    std::shared_ptr<GuiScreen> m_currentScreen;
    GameState m_gameState = GameState::MainMenu;

    Timer m_timer;
    double m_lastFrameTime = 0.0;
    float m_fps = 0.0f;
    bool m_running = true;

    BlockBreakingSystem m_blockBreaking;
    SoundManager m_soundMgr;

    std::unique_ptr<SoundSystem> m_soundSystem;
    SoundPool m_soundPool;
    ChatRenderer m_chatRenderer;
};
