#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Frustum.hpp"
#include "world/World.hpp"
#include "util/Timer.hpp"
#include "entities/EntityPlayer.hpp"
#include "renderer/ModelBiped.hpp"
#include "renderer/ModelZombie.hpp"
#include "renderer/FontRenderer.hpp"
#include "net/NetworkManager.hpp"
#include "net/Client.hpp"
#include "net/IntegratedServer.hpp"
#include "net/Packets.hpp"

class Minecraft {
public:
    Minecraft(GLFWwindow* window, int width, int height);
    ~Minecraft();

    void run();
    void resize(int width, int height);
    void mouseCallback(double xpos, double ypos);

private:
    void init();
    void tick();
    void render(float partialTicks);
    void handleInput();
    void onPacketReceived(const uint8_t* data, size_t size);

    GLFWwindow* m_window;
    int m_width;
    int m_height;

    std::unique_ptr<RenderEngine> m_renderEngine;
    std::unique_ptr<WorldRenderer> m_worldRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_entityShader;
    std::unique_ptr<World> m_world;
    std::unique_ptr<EntityPlayer> m_player;
    std::unique_ptr<ModelBiped> m_playerModel;
    std::unique_ptr<ModelZombie> m_zombieModel;
    std::unique_ptr<FontRenderer> m_fontRenderer;
    
    std::unique_ptr<IntegratedServer> m_server;
    std::unique_ptr<Client> m_client;

    Timer m_timer;
    Camera m_camera;
    Frustum m_frustum;

    bool m_firstMouse = true;
    float m_lastX;
    float m_lastY;

    int m_terrainTex = 0;
    int32_t m_playerID = -1;
    double m_titleTimer = 0.0;
    double m_lastFrameTime = 0.0;
    float m_fps = 0.0f;
    bool m_running = true;

    bool m_showDebug = false;
    int m_cameraMode = 0; // 0 = 1st person, 1 = 3rd person back, 2 = 3rd person front
    bool m_f3Pressed = false;
    bool m_f5Pressed = false;
    bool m_leftMousePressed = false;
};
