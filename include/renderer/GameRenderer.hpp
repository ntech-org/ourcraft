#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <glm/glm.hpp>

#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Frustum.hpp"
#include "renderer/ModelBiped.hpp"
#include "renderer/ModelZombie.hpp"
#include "renderer/FontRenderer.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

class GameRenderer {
public:
    GameRenderer(GLFWwindow* window, World& world, EntityPlayer& player);
    ~GameRenderer();

    void render(float partialTicks, int cameraMode, bool showDebug, bool showBoundaries, bool showProfiler, float fps);
    void resize(int width, int height);

    struct ProfilerResult {
        double frameTime = 0;
        double updateTime = 0;
        double renderTime = 0;
        double worldTime = 0;
        double entityTime = 0;
        double uiTime = 0;
        
        double frameTimeHistory[128] = { 0 };
        int historyIndex = 0;
    };

    RenderEngine& getRenderEngine() { return *m_renderEngine; }
    WorldRenderer& getWorldRenderer() { return *m_worldRenderer; }
    Camera& getCamera() { return m_camera; }
    ProfilerResult& getProfiler() { return m_profiler; }

private:
    void setupFog(const glm::vec3& fogColor, float py, bool inWater, bool inLava);
    void renderWorld(float partialTicks, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor, float voidDarkening);
    void renderEntities(float partialTicks, const glm::mat4& projection, const glm::mat4& view, int cameraMode);
    void renderFirstPersonArm(float partialTicks, const glm::mat4& projection);
    void renderUI(bool showDebug, bool showProfiler, float fps, int cameraMode);

    GLFWwindow* m_window;
    World& m_world;
    EntityPlayer& m_player;

    int m_width;
    int m_height;

    std::unique_ptr<RenderEngine> m_renderEngine;
    std::unique_ptr<WorldRenderer> m_worldRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_entityShader;
    std::unique_ptr<Shader> m_debugShader;
    std::unique_ptr<Shader> m_uiShader;
    std::unique_ptr<ModelBiped> m_playerModel;
    std::unique_ptr<ModelZombie> m_zombieModel;
    std::unique_ptr<FontRenderer> m_fontRenderer;

    ProfilerResult m_profiler;
    
    Camera m_camera;
    Frustum m_frustum;
    int m_terrainTex = 0;
};
