#include <glm/geometric.hpp>
#include "Minecraft.hpp"
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
    if (m_ft) FT_Done_FreeType(m_ft);
}

void Minecraft::init() {
    Block::init();
    Item::init();

    m_world = std::make_unique<World>();
    m_world->isRemote = true;

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->setPosition(0.0, 128.0, 0.0);
    m_player->onOpenCraftingTable = [this]() {
        displayGuiScreen(std::make_shared<GuiCrafting>());
    };

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

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
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->setPosition(0.0, 128.0, 0.0);
    m_player->onOpenCraftingTable = [this]() {
        displayGuiScreen(std::make_shared<GuiCrafting>());
    };

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

    m_gameState = GameState::MainMenu;
    displayGuiScreen(std::make_shared<GuiMainMenu>());
}

void Minecraft::startSingleplayer() {
    m_networkHandler = std::make_unique<NetworkHandler>(*m_world, *m_player);
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
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->setPosition(0.0, 128.0, 0.0);
    m_player->onOpenCraftingTable = [this]() {
        displayGuiScreen(std::make_shared<GuiCrafting>());
    };

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
        resetBlockBreaking(true);
        if (m_inputHandler) m_inputHandler->releaseAllButtons();
        SDL_SetWindowRelativeMouseMode(m_window, false);
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
        double updateStart = (double)SDL_GetTicksNS() / 1e9;
        for (int i = 0; i < m_timer.elapsedTicks; ++i) tick();
        m_gameRenderer->getProfiler().updateTime = ((double)SDL_GetTicksNS() / 1e9 - updateStart) * 1000.0;

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
        // Never allow ticking world logic while in MainMenu
        return;
    }

    if (m_networkHandler) {
        m_networkHandler->update();
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

            if (m_hitDelayTimer > 0) m_hitDelayTimer--;
            if (m_rightClickDelayTimer > 0) m_rightClickDelayTimer--;

            if (m_inputHandler->isEscPressed()) {
                displayGuiScreen(std::make_shared<GuiIngameMenu>());
            }
            if (m_inputHandler->shouldToggleInventory()) {
                displayGuiScreen(std::make_shared<GuiInventory>());
                return;
            }

            float reach = 5.0f;
            glm::dvec3 eyePos = glm::dvec3(m_player->posX, m_player->posY + 1.62f, m_player->posZ);
            float yaw = glm::radians(m_player->rotationYaw);
            float pitch = glm::radians(m_player->rotationPitch);
            glm::dvec3 lookDir = glm::dvec3(
                -std::sin(yaw) * std::cos(pitch),
                std::sin(pitch),
                std::cos(yaw) * std::cos(pitch)
            );

            glm::dvec3 endPos = eyePos + lookDir * (double)reach;
            HitResult hit = m_world->rayTraceBlocks(eyePos, endPos, true);
            
            double dist = reach;
            if (hit.type == HitType::BLOCK) {
                dist = glm::distance(eyePos, hit.hitVec);
            }

            AxisAlignedBB reachBB = AxisAlignedBB(
                std::min(eyePos.x, endPos.x), std::min(eyePos.y, endPos.y), std::min(eyePos.z, endPos.z),
                std::max(eyePos.x, endPos.x), std::max(eyePos.y, endPos.y), std::max(eyePos.z, endPos.z)
            ).expand(1.0, 1.0, 1.0);

            std::vector<Entity*> entities = m_world->getEntitiesWithinAABB(reachBB);
            for (Entity* entity : entities) {
                if (entity == m_player.get() || dynamic_cast<EntityItem*>(entity)) continue;
                
                float border = 0.1f;
                AxisAlignedBB entityBB = entity->boundingBox.expand(border, border, border);
                auto intercept = entityBB.calculateIntercept(eyePos, endPos);
                if (intercept) {
                    double d = glm::distance(eyePos, intercept->hitVec);
                    if (d < dist) {
                        hit.type = HitType::ENTITY;
                        hit.entity = entity;
                        hit.hitVec = intercept->hitVec;
                        dist = d;
                    }
                }
            }
            m_objectMouseOver = hit;

            const bool leftDown = m_inputHandler->isLeftMouseDown();
            const bool leftClick = m_inputHandler->isLeftClick();

            if (leftDown && m_hitDelayTimer <= 0) {
                if (hit.type == HitType::ENTITY && hit.entity) {
                    m_player->swing();
                    if (m_networkHandler) {
                        PacketUseEntity packet;
                        packet.userEntityID = m_player->entityID;
                        packet.targetEntityID = hit.entity->entityID;
                        packet.leftClick = 1;
                        m_networkHandler->sendPacket(packet);
                    }
                    m_hitDelayTimer = 10;
                } else if (hit.type == HitType::NONE && leftClick) {
                    m_player->swing();
                    m_hitDelayTimer = 10;
                }
            }

            if ((!leftDown || hit.type != HitType::BLOCK) && m_hitDelayTimer <= 0) {
                resetBlockBreaking(true);
            } else if (leftDown && hit.type == HitType::BLOCK && m_hitDelayTimer <= 0) {
                const bool sameTarget = m_isBreakingBlock &&
                                        m_breakX == hit.x &&
                                        m_breakY == hit.y &&
                                        m_breakZ == hit.z;

                if (!sameTarget) {
                    resetBlockBreaking(true);
                    const uint8_t targetID = m_world->getBlockID(hit.x, hit.y, hit.z);
                    if (targetID > 0 && Block::getHardness(targetID) >= 0.0f) {
                        m_isBreakingBlock = true;
                        m_breakX = hit.x;
                        m_breakY = hit.y;
                        m_breakZ = hit.z;
                        m_breakFace = hit.sideHit;
                        m_breakProgress = 0.0f;
                        m_breakSwingTick = 0;
                        m_networkHandler->sendDigging(DiggingAction::START, hit.x, hit.y, hit.z, hit.sideHit);
                        m_player->swing();
                    }
                }

                if (m_isBreakingBlock) {
                    const uint8_t targetID = m_world->getBlockID(m_breakX, m_breakY, m_breakZ);
                    if (targetID == 0 || Block::getHardness(targetID) < 0.0f) {
                        resetBlockBreaking(true);
                    } else if (m_player->gameMode == GameMode::CREATIVE) {
                        m_world->setBlockWithNotify(m_breakX, m_breakY, m_breakZ, 0);
                        m_networkHandler->sendDigging(DiggingAction::FINISH, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
                        m_player->swing();
                        resetBlockBreaking(false);
                        m_hitDelayTimer = 5;
                    } else {
                        m_breakProgress = std::min(1.0f, m_breakProgress + getBreakDeltaForBlock(targetID));
                        if ((++m_breakSwingTick % 4) == 0) {
                            m_player->swing();
                        }
                        if (m_breakProgress >= 1.0f) {
                            finishBreakingCurrentBlock();
                        }
                    }
                }
            }

            if (m_isBreakingBlock) {
                m_gameRenderer->setBlockBreakingOverlay(true, m_breakX, m_breakY, m_breakZ, m_breakProgress);
            } else {
                m_gameRenderer->setBlockBreakingOverlay(false, 0, 0, 0, 0.0f);
            }

            if (m_inputHandler->isRightMouseDown() && m_rightClickDelayTimer <= 0) {
                resetBlockBreaking(true);

                ItemStack& currentStack = m_player->inventory.getCurrentStack();
                if (!currentStack.isEmpty() && currentStack.itemID >= 256) {
                    Item* item = Item::itemsList[currentStack.itemID];
                    if (item) {
                        int oldCount = currentStack.count;
                        currentStack = item->onItemRightClick(currentStack, *m_world, *m_player);
                        if (currentStack.count != oldCount || currentStack.isEmpty()) {
                            m_rightClickDelayTimer = 4;
                            return;
                        }
                    }
                }

                if (hit.type == HitType::BLOCK) {
                    uint8_t targetID = m_world->getBlockID(hit.x, hit.y, hit.z);
                    if (targetID > 0 && Block::blocksList[targetID]->onBlockActivated(*m_world, hit.x, hit.y, hit.z, m_player.get())) {
                        m_rightClickDelayTimer = 4;
                    } else {
                        int x = hit.x, y = hit.y, z = hit.z;
                        int face = hit.sideHit;
                        if (face == 0) y--; else if (face == 1) y++;
                        else if (face == 2) z--; else if (face == 3) z++;
                        else if (face == 4) x--; else if (face == 5) x++;

                        AxisAlignedBB blockBB((double)x, (double)y, (double)z, (double)x + 1.0, (double)y + 1.0, (double)z + 1.0);
                        if (!m_player->boundingBox.intersectsWith(blockBB)) {
                            int itemID = m_player->inventory.getCurrentItemID();
                            if (itemID > 0) {
                                const bool shouldConsume = m_player->gameMode == GameMode::SURVIVAL;
                                if (!shouldConsume || m_player->inventory.consumeCurrentItem(1)) {
                                    m_world->setBlockWithNotify(x, y, z, (uint8_t)itemID);
                                    m_player->swing();
                                    m_networkHandler->sendPlacement(hit.x, hit.y, hit.z, hit.sideHit, itemID, 0);
                                    m_rightClickDelayTimer = 4;
                                }
                            }
                        }
                    }
                }
            }

            if (m_inputHandler->shouldReloadChunks()) {
                m_gameRenderer->getWorldRenderer().rebuildSectionList();
            }
        }

        m_player->onUpdate();
        m_gameRenderer->updateItemEquippedProgress();
        m_gameRenderer->getRenderEngine().updateTextureFX();
        if (m_networkHandler) {
            m_networkHandler->sendPlayerPosition(*m_player);
        }
    }
}


void Minecraft::resetBlockBreaking(bool sendStopPacket) {
    if (sendStopPacket && m_isBreakingBlock && m_networkHandler) {
        m_networkHandler->sendDigging(DiggingAction::STOP, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
    }
    m_isBreakingBlock = false;
    m_breakFace = -1;
    m_breakProgress = 0.0f;
    m_breakSwingTick = 0;
    if (m_gameRenderer) {
        m_gameRenderer->setBlockBreakingOverlay(false, 0, 0, 0, 0.0f);
    }
}

float Minecraft::getBreakDeltaForBlock(uint8_t blockID) const {
    const float hardness = Block::getHardness(blockID);
    if (hardness <= 0.0f) {
        return 1.0f;
    }
    // Faster base speed for better game feel, closer to original Infdev
    constexpr float baseSpeed = 1.0f / 15.0f; 
    return baseSpeed / hardness;
}

bool Minecraft::finishBreakingCurrentBlock() {
    if (!m_isBreakingBlock) {
        return false;
    }

    const uint8_t targetID = m_world->getBlockID(m_breakX, m_breakY, m_breakZ);
    if (targetID == 0 || Block::getHardness(targetID) < 0.0f) {
        resetBlockBreaking(false);
        return false;
    }

    m_world->setBlockWithNotify(m_breakX, m_breakY, m_breakZ, 0);
    m_objectMouseOver.type = HitType::NONE;
    m_networkHandler->sendDigging(DiggingAction::FINISH, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
    m_player->swing();
    resetBlockBreaking(false);
    return true;
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
