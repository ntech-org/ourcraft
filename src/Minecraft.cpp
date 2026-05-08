#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "renderer/Tessellator.hpp"
#include "gui/GuiMainMenu.hpp"
#include "gui/GuiIngameMenu.hpp"
#include "gui/GuiInventory.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <stdexcept>

Minecraft::Minecraft(GLFWwindow* window, int width, int height)
    : m_window(window), m_width(width), m_height(height), m_timer(20.0f)
{
    Tessellator::init();
    init();
}

Minecraft::~Minecraft() {
    if (m_gameState != GameState::MainMenu) {
        saveAndQuit();
    }
}

void Minecraft::init() {
    Block::init();

    m_world = std::make_unique<World>();
    m_world->isRemote = true;

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->setPosition(0.0, 128.0, 0.0);

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);

    displayGuiScreen(std::make_shared<GuiMainMenu>());
}

void Minecraft::saveAndQuit() {
    if (m_networkHandler) m_networkHandler->stopServer();
    m_networkHandler.reset();
    
    // Re-initialize client to clean state
    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setMinecraft(this);
    m_player->isLocalPlayer = true;
    m_player->setPosition(0.0, 128.0, 0.0);
    
    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player, m_settings);
    
    m_gameState = GameState::MainMenu;
    displayGuiScreen(std::make_shared<GuiMainMenu>());
}

void Minecraft::startSingleplayer() {
    // In case we were already in a world, the above ensures we are clean.
    // Note: World/Renderer/Player are already initialized by init() or saveAndQuit().
    
    m_networkHandler = std::make_unique<NetworkHandler>(*m_world, *m_player);

    if (!m_networkHandler->connect("127.0.0.1", 25565)) {
        // Failed
    }

    m_gameRenderer->getWorldRenderer().rebuildSectionList();
    
    m_gameState = GameState::InGame;
    displayGuiScreen(nullptr);
}

void Minecraft::displayGuiScreen(std::shared_ptr<GuiScreen> screen) {
    if (m_currentScreen) m_currentScreen->onGuiClosed();
    m_currentScreen = screen;
    if (m_currentScreen) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_currentScreen->setWorldAndResolution(this, m_gameRenderer->getScaledWidth(), m_gameRenderer->getScaledHeight());
    } else {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void Minecraft::run() {
    m_lastFrameTime = glfwGetTime();
    while (m_running && !glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        double frameDelta = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_fps = frameDelta > 0.0 ? (float)(1.0 / frameDelta) : 0.0f;

        m_timer.updateTimer();
        double updateStart = glfwGetTime();
        for (int i = 0; i < m_timer.elapsedTicks; ++i) tick();
        m_gameRenderer->getProfiler().updateTime = (glfwGetTime() - updateStart) * 1000.0;

        m_gameRenderer->render(m_timer.renderPartialTicks,
                               m_inputHandler->getCameraMode(),
                               m_inputHandler->isDebugVisible(),
                               m_inputHandler->isChunkBoundariesVisible(),
                               m_inputHandler->isProfilerVisible(),
                               m_fps,
                               m_currentScreen);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Minecraft::tick() {
    if (m_currentScreen) {
        m_currentScreen->updateScreen();
    } else {
        m_world->update(0.05f);
        m_networkHandler->update();
        m_inputHandler->update();

        if (m_inputHandler->isEscPressed()) {
            displayGuiScreen(std::make_shared<GuiIngameMenu>());
        }
        if (m_inputHandler->shouldToggleInventory()) {
            displayGuiScreen(std::make_shared<GuiInventory>());
            return;
        }

        // Raycast for block picking
        float reach = 5.0f;
        glm::vec3 eyePos = glm::vec3(m_player->posX, m_player->posY + 1.62f, m_player->posZ);
        float yaw = glm::radians(m_player->rotationYaw);
        float pitch = glm::radians(m_player->rotationPitch);
        glm::vec3 lookDir = glm::vec3(
            -std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)
        );

        glm::vec3 endPos = eyePos + lookDir * reach;
        HitResult hit = m_world->rayTraceBlocks(eyePos, endPos, true);

        const bool leftDown = m_inputHandler->isLeftMouseDown();
        if (!leftDown || hit.type != HitType::BLOCK) {
            resetBlockBreaking(true);
        } else {
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
                    m_networkHandler->sendDigging(DiggingAction::STOP, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
                    m_player->swing();
                    resetBlockBreaking(false);
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


        if (m_inputHandler->isRightClick()) {
            resetBlockBreaking(true);
            if (hit.type == HitType::BLOCK) {
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
                        }
                    }
                }
            }
        }



        if (m_inputHandler->shouldReloadChunks()) {
            m_gameRenderer->getWorldRenderer().rebuildSectionList();
        }

        m_player->onUpdate();
        m_gameRenderer->getRenderEngine().updateTextureFX();
        m_networkHandler->sendPlayerPosition(*m_player);
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

    // Infdev-feel survival mining speed: slower than creative, no tools yet.
    constexpr float baseSpeed = 1.0f / 30.0f;
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
}

void Minecraft::mouseCallback(double xpos, double ypos) {
    if (m_currentScreen) return;
    m_inputHandler->handleMouse(xpos, ypos);
}

void Minecraft::scrollCallback(double xoffset, double yoffset) {
    if (m_currentScreen) return;
    m_inputHandler->handleScroll(xoffset, yoffset);
}

void Minecraft::mouseButtonCallback(int button, int action, int mods) {
    if (m_currentScreen && action == GLFW_PRESS) {
        auto screen = m_currentScreen; // Hold reference to prevent crash if screen is changed
        
        double mx, my;
        glfwGetCursorPos(m_window, &mx, &my);
        
        // Convert screen units to framebuffer pixels
        int ww, wh, fw, fh;
        glfwGetWindowSize(m_window, &ww, &wh);
        glfwGetFramebufferSize(m_window, &fw, &fh);
        
        mx *= (double)fw / (double)ww;
        my *= (double)fh / (double)wh;
        
        mx /= (double)m_gameRenderer->getGuiScale();
        my /= (double)m_gameRenderer->getGuiScale();
        
        screen->mouseClicked((int)mx, (int)my, button);
    }
}

void Minecraft::keyCallback(int key, int scancode, int action, int mods) {
    if (m_currentScreen) {
        m_currentScreen->keyTyped(key, scancode, action, mods);
    }
}
