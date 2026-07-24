#include "InputHandler.hpp"
#include "world/World.hpp"
#include <algorithm>
#include <iostream>

namespace {
constexpr int kCreativeFlyDoubleTapWindowTicks = 5;
}

InputHandler::InputHandler(SDL_Window* window, EntityPlayer& player, GameSettings& settings)
    : m_window(window), m_player(player), m_settings(settings)
{
}

void InputHandler::update() {
    m_player.moveForward = 0.0f;
    m_player.moveStrafe = 0.0f;

    const bool* state = SDL_GetKeyboardState(nullptr);

    if (state[SDL_SCANCODE_W]) m_player.moveForward += 1.0f;
    if (state[SDL_SCANCODE_S]) m_player.moveForward -= 1.0f;
    if (state[SDL_SCANCODE_A]) m_player.moveStrafe += 1.0f;
    if (state[SDL_SCANCODE_D]) m_player.moveStrafe -= 1.0f;

    for (int i = 0; i < 9; ++i) {
        if (state[SDL_SCANCODE_1 + i]) m_player.inventory.setSlot(i);
    }

    m_player.jumping = state[SDL_SCANCODE_SPACE];
    m_player.sneaking = state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
    m_player.sprinting = state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];

    if (m_spaceTapTicks > 0) m_spaceTapTicks--;
}

void InputHandler::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        bool f3 = (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3]);

        switch (event.key.scancode) {
            case SDL_SCANCODE_ESCAPE:
                m_escPressed = true;
                break;
            case SDL_SCANCODE_E:
                m_inventoryPressed = true;
                break;
            case SDL_SCANCODE_F5:
                m_cameraMode = (m_cameraMode + 1) % 3;
                break;
            case SDL_SCANCODE_SPACE:
                if (m_player.gameMode == GameMode::CREATIVE) {
                    if (!m_spaceHeld && m_spaceTapTicks > 0) {
                        m_player.isFlying = !m_player.isFlying;
                        if (m_player.isFlying) m_player.motionY = 0;
                        m_spaceTapTicks = 0;
                    } else if (!m_spaceHeld) {
                        m_spaceTapTicks = kCreativeFlyDoubleTapWindowTicks;
                    }
                }
                m_spaceHeld = true;
                break;
            case SDL_SCANCODE_F3:
                // Just pressing F3 alone toggles debug
                m_showDebug = !m_showDebug;
                break;
            case SDL_SCANCODE_G:
                if (f3) {
                    m_showChunkBoundaries = !m_showChunkBoundaries;
                    std::cout << "[Debug] Chunk boundaries: " << (m_showChunkBoundaries ? "ON" : "OFF") << std::endl;
                }
                break;
            case SDL_SCANCODE_P:
                if (f3) {
                    m_showProfiler = !m_showProfiler;
                    std::cout << "[Debug] Profiler: " << (m_showProfiler ? "ON" : "OFF") << std::endl;
                }
                break;
            case SDL_SCANCODE_A:
                if (f3) {
                    m_reloadChunks = true;
                    std::cout << "[Debug] Reloading all chunks..." << std::endl;
                }
                break;
            case SDL_SCANCODE_F4:
                if (f3) {
                    m_player.gameMode = (m_player.gameMode == GameMode::SURVIVAL) ? GameMode::CREATIVE : GameMode::SURVIVAL;
                    if (m_player.gameMode == GameMode::SURVIVAL) m_player.isFlying = false;
                    std::cout << "[Debug] Gamemode: " << (m_player.gameMode == GameMode::SURVIVAL ? "SURVIVAL" : "CREATIVE") << std::endl;
                }
                break;
            case SDL_SCANCODE_T:
                if (f3) {
                    m_player.worldObj.setWorldTime(m_player.worldObj.getWorldTime() + 1000.0);
                    std::cout << "[Debug] Advanced time +1000" << std::endl;
                } else {
                    m_chatPressed = true;
                }
                break;
            case SDL_SCANCODE_Q:
                if (!f3) {
                    m_dropItemPressed = true;
                }
                break;
            case SDL_SCANCODE_Y:
                if (f3) {
                    m_player.worldObj.setWorldTime(0.0);
                    std::cout << "[Debug] Reset time to 0" << std::endl;
                }
                break;
            default:
                break;
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_leftClick = true;
            m_leftMousePressed = true;
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_rightClick = true;
            m_rightMousePressed = true;
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            m_leftMousePressed = false;
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            m_rightMousePressed = false;
        }
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        if (event.wheel.y > 0) m_player.inventory.prevSlot();
        else if (event.wheel.y < 0) m_player.inventory.nextSlot();
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        float xoffset = event.motion.xrel;
        float yoffset = -event.motion.yrel; // Invert y as SDL has origin at top-left

        float sensitivity = m_settings.mouseSensitivity * 0.5f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        m_player.rotationYaw += xoffset;
        m_player.rotationPitch += yoffset;

        m_player.rotationPitch = std::clamp(m_player.rotationPitch, -89.9f, 89.9f);
    } else if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.scancode == SDL_SCANCODE_SPACE) {
            m_spaceHeld = false;
        }
    }
}
