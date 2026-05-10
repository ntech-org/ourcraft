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
    void startSingleplayer();
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
    const HitResult& getObjectMouseOver() const { return m_objectMouseOver; }

private:

    void init();
    void tick();
    void resetBlockBreaking(bool sendStopPacket);
    float getBreakDeltaForBlock(uint8_t blockID) const;
    bool finishBreakingCurrentBlock();

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

    bool m_isBreakingBlock = false;
    int m_breakX = 0;
    int m_breakY = 0;
    int m_breakZ = 0;
    int m_breakFace = -1;
    float m_breakProgress = 0.0f;
    int m_breakSwingTick = 0;

    int m_hitDelayTimer = 0;
    int m_rightClickDelayTimer = 0;
    HitResult m_objectMouseOver;
};
