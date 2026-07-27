#include <glm/geometric.hpp>
#include "Minecraft.hpp"
#include "PlayerTick.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "renderer/Tessellator.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiIngameMenu.hpp"
#include "gui/GuiInventory.hpp"
#include "gui/GuiErrorScreen.hpp"
#include "gui/GuiConnecting.hpp"
#include "gui/GuiLoading.hpp"
#include "items/ItemFood.hpp"
#include "entities/EntityItem.hpp"
#include "gui/GuiCrafting.hpp"
#include "gui/GuiFurnace.hpp"
#include "gui/GuiChat.hpp"
#include "gui/GuiCreativeInventory.hpp"
#include "gui/GuiDeathScreen.hpp"
#include "world/TileEntityFurnace.hpp"
#include "net/Packets.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <stdexcept>

Minecraft::Minecraft(SDL_Window* window, int width, int height)
    : m_window(window), m_width(width), m_height(height), m_timer(20.0f)
{
    if (FT_Init_FreeType(&m_ft)) {
        std::cerr << "[Minecraft] Failed to initialize FreeType" << std::endl;
    }
    Tessellator::init();
    init();
}

Minecraft::~Minecraft() {
    if (m_gameState != GameState::MainMenu) {
        saveAndQuit();
    }
    m_font.reset();
    if (m_ft) FT_Done_FreeType(m_ft);
    if (m_soundSystem) {
        m_soundSystem->shutdown();
        m_soundSystem.reset();
    }
}

void Minecraft::setupPlayerCallbacks() {
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->onPlaySound = [this](const std::string& name, float vol, float pitch) {
        if (auto* snd = m_soundPool.getRandom(name, *m_soundSystem))
            m_soundSystem->play3D(snd, (float)m_player->posX, (float)m_player->posY, (float)m_player->posZ, m_settings.soundVolume * vol, pitch);
    };
    m_player->onOpenCraftingTable = [this]() {
        displayGuiScreen(std::make_shared<GuiCrafting>());
    };
    m_player->onOpenFurnace = [this](int x, int y, int z) {
        TileEntity* te = m_world->getTileEntity(x, y, z);
        if (auto* furnace = dynamic_cast<TileEntityFurnace*>(te)) {
            auto gui = std::make_shared<GuiFurnace>(*furnace);
            gui->parentScreen = m_currentScreen;
            displayGuiScreen(gui);
        }
    };
    m_player->onHurt = [this]() {
        if (auto* snd = m_soundPool.getRandom("damage.hit", *m_soundSystem))
            m_soundSystem->play(snd, m_settings.soundVolume * 1.0f, 1.0f);
    };
}

void Minecraft::init() {
    Block::init();
    Item::init();

    m_soundSystem = std::make_unique<SoundSystem>();
    m_soundSystem->init();
    buildSoundPool(m_soundPool, "assets/resources/sound/");
    buildSoundPool(m_soundPool, "assets/resources/music/");
    buildSoundPool(m_soundPool, "assets/resources/menumusic/");

    auto preload = [this](const std::string& pool) {
        m_soundPool.getRandom(pool, *m_soundSystem);
    };

    for (const auto& name : m_soundPool.getPoolNames()) {
        if (name.find("menu") == 0)
            m_soundMgr.menuMusicPools.push_back(name);
        else if (name.find("calm") == 0 || name.find("hal") == 0 ||
            name.find("nuance") == 0 || name.find("piano") == 0)
            m_soundMgr.musicPools.push_back(name);
    }

    m_world = std::make_unique<World>();
    m_world->isRemote = true;

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(0.0, 66.0, 0.0);
    setupPlayerCallbacks();

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

    // Initialize player from active account
    if (!m_settings.accounts.empty() && m_settings.activeAccountIndex < m_settings.accounts.size()) {
        const auto& acc = m_settings.accounts[m_settings.activeAccountIndex];
        m_player->username = acc.name;
        m_player->uuid = acc.uuid;
    }

    auto loadFile = [](const std::string& p) -> std::vector<uint8_t> {
        std::string path = "assets/" + p;
        if (p[0] == '/') path = "assets" + p;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        file.read((char*)buffer.data(), size);
        return buffer;
    };

    auto fontSet = std::make_unique<FontSet>(m_gameRenderer->getRenderEnginePtr(), 10.0f);

    auto primaryData = loadFile("minecraft.ttf");
    if (!primaryData.empty()) {
        fontSet->addProvider(std::make_unique<FreeTypeGlyphProvider>(m_ft, primaryData, 10.0f));
    }

    auto fallbackData = loadFile("unifont.otf");
    if (!fallbackData.empty()) {
        fontSet->addProvider(std::make_unique<FreeTypeGlyphProvider>(m_ft, fallbackData, 10.0f));
    }
    m_font = std::make_unique<Font>(std::move(fontSet));

    displayGuiScreen(std::make_shared<GuiMainMenu>());
}

void Minecraft::saveAndQuit() {
    if (m_networkHandler) m_networkHandler->stopServer();
    m_networkHandler.reset();

    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(0.0, 66.0, 0.0);
    setupPlayerCallbacks();

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

    m_gameState = GameState::MainMenu;
    displayGuiScreen(std::make_shared<GuiMainMenu>());
}

void Minecraft::startSingleplayer(const std::string& worldName) {
    m_networkHandler = std::make_unique<NetworkHandler>(*m_world, *m_player, true, worldName);
    m_networkHandler->setRenderDistance(m_settings.renderDistanceChunks);
    m_networkHandler->onDisconnected = [this](bool timeout, const std::string& reason) {
        if (m_gameState != GameState::MainMenu) {
            displayGuiScreen(std::make_shared<GuiErrorScreen>("Disconnected", reason));
        }
    };

    if (!m_networkHandler->connect("127.0.0.1", 25565)) {
        displayGuiScreen(std::make_shared<GuiErrorScreen>("Connection Failed", "Failed to connect to internal server"));
        return;
    }

    m_gameRenderer->getWorldRenderer().rebuildSectionList();
    m_gameState = GameState::InGame;
    displayGuiScreen(std::make_shared<GuiLoading>());
}

void Minecraft::startMultiplayer(const std::string& address, int port) {
    if (m_networkHandler) m_networkHandler->stopServer();
    m_networkHandler.reset();

    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_player = std::make_unique<EntityPlayer>(*m_world);
    
    // Use active account
    if (!m_settings.accounts.empty() && m_settings.activeAccountIndex < m_settings.accounts.size()) {
        const auto& acc = m_settings.accounts[m_settings.activeAccountIndex];
        m_player->username = acc.name;
        m_player->uuid = acc.uuid;
    }
    
    m_player->setPosition(0.0, 66.0, 0.0);
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    setupPlayerCallbacks();

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

    m_networkHandler = std::make_unique<NetworkHandler>(*m_world, *m_player, false);
    m_networkHandler->onDisconnected = [this](bool timeout, const std::string& reason) {
        if (m_gameState != GameState::MainMenu) {
            displayGuiScreen(std::make_shared<GuiErrorScreen>("Disconnected", reason));
        }
    };

    if (!m_networkHandler->connect(address, port)) {
        displayGuiScreen(std::make_shared<GuiErrorScreen>("Connection Failed", "Failed to connect to " + address + ":" + std::to_string(port)));
        return;
    }

    m_gameRenderer->getWorldRenderer().rebuildSectionList();
    m_gameState = GameState::InGame;
    displayGuiScreen(std::make_shared<GuiLoading>());
}

void Minecraft::displayGuiScreen(std::shared_ptr<GuiScreen> screen) {
    if (m_currentScreen) m_currentScreen->onGuiClosed();
    m_currentScreen = screen;
    if (m_currentScreen) {
        m_blockBreaking.resetBlockBreaking(true, *this, m_networkHandler.get(), *m_gameRenderer);
        if (m_inputHandler) m_inputHandler->releaseAllButtons();
        if (m_currentScreen->wantsCursor()) {
            SDL_SetWindowRelativeMouseMode(m_window, false);
        }
        m_currentScreen->setWorldAndResolution(this, m_gameRenderer->getScaledWidth(), m_gameRenderer->getScaledHeight());
    } else {
        if (m_inputHandler) m_inputHandler->releaseAllButtons();
        SDL_SetWindowRelativeMouseMode(m_window, true);
    }
}

void Minecraft::run() {
    m_lastFrameTime = (double)SDL_GetTicksNS() / 1e9;
    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            }
            handleEvent(event);
        }

        std::shared_ptr<GuiScreen> currentScreen = m_currentScreen;

        double now = (double)SDL_GetTicksNS() / 1e9;
        double frameDelta = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_fps = frameDelta > 0.0 ? (float)(1.0 / frameDelta) : 0.0f;

        m_timer.updateTimer();

        if (m_soundSystem && m_player) {
            m_soundSystem->update((float)m_player->posX, (float)m_player->posY + 1.6f, (float)m_player->posZ,
                                  m_player->rotationYaw, m_player->rotationPitch,
                                  m_settings.soundVolume, m_settings.musicVolume);
        }

        double updateStart = (double)SDL_GetTicksNS() / 1e9;
        for (int i = 0; i < m_timer.elapsedTicks; ++i) tick();
        m_gameRenderer->getProfiler().updateTime = ((double)SDL_GetTicksNS() / 1e9 - updateStart) * 1000.0;

        if (m_soundSystem && m_settings.musicVolume > 0.0f) {
            bool isMainMenu = (m_gameState == GameState::MainMenu);
            m_soundMgr.tick(*m_soundSystem, m_soundPool, m_settings.musicVolume, isMainMenu);
        }

        float renderPartialTicks = m_timer.renderPartialTicks;
        if (m_gameState == GameState::Paused) {
            renderPartialTicks = 1.0f;
        }

        m_gameRenderer->render(renderPartialTicks,
                               m_inputHandler->getCameraMode(),
                               m_inputHandler->isDebugVisible(),
                               m_inputHandler->isChunkBoundariesVisible(),
                               m_inputHandler->isProfilerVisible(),
                               m_fps,
                               currentScreen);

        SDL_GL_SwapWindow(m_window);
    }
}

void Minecraft::tick() {
    if (m_gameState == GameState::MainMenu) {
        if (m_currentScreen) {
            m_currentScreen->updateScreen();
        }
        return;
    }

    if (m_networkHandler) {
        m_networkHandler->update();
    }
    m_chatRenderer.tick();
    if (m_gameRenderer) {
        m_gameRenderer->tickClouds();
    }

    bool shouldPause = false;
    if (m_currentScreen && m_currentScreen->doesGuiPauseGame()) {
        if (m_networkHandler && m_networkHandler->isSingleplayer()) {
            if (m_networkHandler->getPlayerCount() <= 1) {
                shouldPause = true;
            }
        }
    }

    if (shouldPause) {
        setGameState(GameState::Paused);
        if (m_networkHandler) m_networkHandler->setPaused(true);
        m_timer.elapsedPartialTicks = 0.0f;
    } else {
        setGameState(GameState::InGame);
        if (m_networkHandler) m_networkHandler->setPaused(false);
    }

    if (getGameState() == GameState::Paused) {
        if (m_currentScreen) m_currentScreen->updateScreen();
    } else {
        m_world->update(0.05f);

        if (m_currentScreen) {
            m_currentScreen->updateScreen();
        } else {
            m_inputHandler->update();

            if (m_inputHandler->shouldOpenChat()) {
                m_player->moveForward = 0.0f;
                m_player->moveStrafe = 0.0f;
                displayGuiScreen(std::make_shared<GuiChat>());
            } else if (m_inputHandler->isEscPressed()) {
                displayGuiScreen(std::make_shared<GuiIngameMenu>());
            } else if (m_inputHandler->shouldToggleInventory()) {
                if (m_player->gameMode == GameMode::CREATIVE) {
                    displayGuiScreen(std::make_shared<GuiCreativeInventory>());
                } else {
                    displayGuiScreen(std::make_shared<GuiInventory>());
                }
            } else if (m_inputHandler->shouldDropItem()) {
                if (m_player->gameMode != GameMode::CREATIVE && m_networkHandler) {
                    PacketPlayerDigging packet;
                    packet.action = DiggingAction::DROP_ITEM;
                    packet.x = 0; packet.y = 0; packet.z = 0; packet.face = 0;
                    m_networkHandler->sendPacket(packet);
                }
            } else {
                if (m_blockBreaking.getHitDelayTimer() > 0) m_blockBreaking.setHitDelayTimer(m_blockBreaking.getHitDelayTimer() - 1);
                if (m_blockBreaking.getRightClickDelayTimer() > 0) m_blockBreaking.setRightClickDelayTimer(m_blockBreaking.getRightClickDelayTimer() - 1);

                m_blockBreaking.updateMouseOver(*m_player, *m_world);

                const bool leftDown = m_inputHandler->isLeftMouseDown();
                const bool leftClick = m_inputHandler->isLeftClick();

                if (leftDown && m_blockBreaking.getHitDelayTimer() <= 0) {
                    const HitResult& hit = m_blockBreaking.getObjectMouseOver();
                    if (hit.type == HitType::ENTITY && hit.entity) {
                        m_player->swing();
                        if (m_networkHandler) {
                            PacketUseEntity packet;
                            packet.userEntityID = m_player->entityID;
                            packet.targetEntityID = hit.entity->entityID;
                            packet.leftClick = 1;
                            m_networkHandler->sendPacket(packet);
                        }
                        m_blockBreaking.setHitDelayTimer(10);
                    } else if (hit.type == HitType::NONE && leftClick) {
                        m_player->swing();
                        m_blockBreaking.setHitDelayTimer(10);
                    }
                }

                m_blockBreaking.tick(*this, *m_player, *m_world, *m_gameRenderer, *m_inputHandler,
                                    m_networkHandler.get(), m_soundSystem.get(), m_soundPool,
                                    m_settings.soundVolume, m_timer.renderPartialTicks, m_player->gameMode);
                handleBlockPlacement(*this, *m_player, *m_world);

                if (m_inputHandler->shouldReloadChunks()) {
                    m_gameRenderer->getWorldRenderer().rebuildSectionList();
                }
            }
        }

        m_player->onUpdate();

        // Void protection
        if (m_player->posY < -64.0) {
            m_player->setPosition(m_player->posX, 66.0, m_player->posZ);
            m_player->motionY = 0.0;
            m_player->fallDistance = 0.0f;
            if (m_player->health < m_player->maxHealth / 2) {
                m_player->health = m_player->maxHealth;
            }
        }

        m_soundMgr.lastHealth = m_player->health;
        m_soundMgr.lastFallDistance = m_player->fallDistance;

        m_gameRenderer->updateItemEquippedProgress();
        m_gameRenderer->getRenderEngine().updateTextureFX();
        if (m_networkHandler) {
            m_networkHandler->sendPlayerPosition(*m_player);
        }
    }
}

void Minecraft::resize(int width, int height) {
    m_width = width; m_height = height;
    m_gameRenderer->resize(width, height);
    if (m_currentScreen) {
        m_currentScreen->setWorldAndResolution(this, m_gameRenderer->getScaledWidth(), m_gameRenderer->getScaledHeight());
    }
    glViewport(0, 0, width, height);
}

void Minecraft::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        resize(event.window.data1, event.window.data2);
    }

    std::shared_ptr<GuiScreen> currentScreen = m_currentScreen;
    if (currentScreen) {
        currentScreen->handleEvent(event);
    } else {
        m_inputHandler->handleEvent(event);
    }
}
