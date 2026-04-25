#include "renderer/GameRenderer.hpp"
#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"
#include "renderer/TextureFX.hpp"
#include "entities/EntityLiving.hpp"
#include "InputHandler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

GameRenderer::GameRenderer(GLFWwindow* window, World& world, EntityPlayer& player)
    : m_window(window), m_world(world), m_player(player)
{
    glfwGetWindowSize(window, &m_width, &m_height);

    m_renderEngine = std::make_unique<RenderEngine>();
    m_worldRenderer = std::make_unique<WorldRenderer>(m_world);
    m_skyRenderer = std::make_unique<SkyRenderer>(*m_renderEngine);

    m_basicShader = std::make_unique<Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_basicShader->use();
    m_basicShader->setInt("texture1", 0);
    m_terrainTex = m_renderEngine->getTexture("/terrain.png");

    m_entityShader = std::make_unique<Shader>("assets/shaders/entity.vert", "assets/shaders/entity.frag");
    m_debugShader = std::make_unique<Shader>("assets/shaders/debug.vert", "assets/shaders/debug.frag");
    m_uiShader = std::make_unique<Shader>("assets/shaders/ui.vert", "assets/shaders/ui.frag");
    m_playerModel = std::make_unique<ModelBiped>();
    m_zombieModel = std::make_unique<ModelZombie>();
    m_fontRenderer = std::make_unique<FontRenderer>(m_renderEngine.get(), "/default.png");

    m_renderEngine->registerTextureFX(std::make_unique<TextureWaterFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureWaterFlowFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureLavaFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureLavaFlowFX>());
}

GameRenderer::~GameRenderer() {}

void GameRenderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

void GameRenderer::render(float partialTicks, int cameraMode, bool showDebug, bool showBoundaries, bool showProfiler, float fps) {
    double frameStart = glfwGetTime();
    m_world.pollGeneratedChunks();
    for (auto& newChunk : m_world.popNewChunks()) {
        m_worldRenderer->addSectionsForChunk(newChunk);
    }

    double px = m_player.prevPosX + (m_player.posX - m_player.prevPosX) * (double)partialTicks;
    double py = m_player.prevPosY + (m_player.posY - m_player.prevPosY) * (double)partialTicks + (double)m_player.yOffset;
    double pz = m_player.prevPosZ + (m_player.posZ - m_player.prevPosZ) * (double)partialTicks;

    float camYaw = m_player.rotationYaw;
    float camPitch = m_player.rotationPitch;

    // Update camera based on mode
    if (cameraMode == 0) {
        m_camera.yaw = camYaw + 90.0f;
        m_camera.pitch = camPitch;
        m_camera.position = glm::vec3(px, py, pz);
    } else {
        m_camera.yaw = camYaw + 90.0f + (cameraMode == 2 ? 180.0f : 0.0f);
        m_camera.pitch = (cameraMode == 2 ? -camPitch : camPitch);
        m_camera.updateCameraVectors();
        m_camera.position = glm::vec3(px, py, pz) - m_camera.front * 4.0f;
    }
    m_camera.updateCameraVectors();

    glm::mat4 view = glm::mat4(1.0f);
    if (cameraMode == 0) {
        float bobDist = m_player.prevDistanceWalkedModified + (m_player.distanceWalkedModified - m_player.prevDistanceWalkedModified) * partialTicks;
        float bobStr = m_player.prevCameraYaw + (m_player.cameraYaw - m_player.prevCameraYaw) * partialTicks;
        float bobPitch = m_player.prevCameraPitch + (m_player.cameraPitch - m_player.prevCameraPitch) * partialTicks;

        view = glm::translate(view, glm::vec3(std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f, -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr), 0.0f));
        view = glm::rotate(view, glm::radians(std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        view = glm::rotate(view, glm::radians(std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    view = view * m_camera.getViewMatrix();

    // Chunk management
    int playerCX = (int)std::floor(px / 16.0);
    int playerCZ = (int)std::floor(pz / 16.0);
    
    static int managementTimer = 0;
    if (managementTimer-- <= 0) {
        managementTimer = 10; // Every 10 frames
        m_worldRenderer->removeFarSections(playerCX, playerCZ, 10);
        m_world.unloadFarChunks(playerCX, playerCZ, 10);
    }

    std::vector<std::pair<int, int>> toRequest;
    m_world.getLoadedAndPendingChunks(playerCX, playerCZ, 8, toRequest);
    for (auto& pos : toRequest) {
        m_world.requestChunk(pos.first, pos.second);
    }

    // Fog and clear
    float voidDarkening = std::clamp((float)(py / m_world.getHorizon()), 0.0f, 1.0f);
    voidDarkening *= voidDarkening;

    glm::vec3 fogColor = (m_world.getFogColor() + (m_world.getSkyColor() - m_world.getFogColor()) * 0.29289321881f) * voidDarkening;
    bool inWater = false, inLava = false;
    uint8_t camBlockID = m_world.getBlockID((int)std::floor(m_camera.position.x), (int)std::floor(m_camera.position.y), (int)std::floor(m_camera.position.z));
    if (camBlockID > 0) {
        const Material& mat = Block::blocksList[camBlockID]->blockMaterial;
        if (mat == Material::water) { inWater = true; fogColor = glm::vec3(0.02f, 0.02f, 0.2f) * voidDarkening; }
        else if (mat == Material::lava) { inLava = true; fogColor = glm::vec3(0.6f, 0.1f, 0.0f) * voidDarkening; }
    }

    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspect = m_height > 0 ? (float)m_width / (float)m_height : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.05f, 1000.0f);

    double renderStart = glfwGetTime();
    m_skyRenderer->render(m_world, m_camera, projection, view, fogColor);
    
    double worldStart = glfwGetTime();
    renderWorld(partialTicks, projection, view, fogColor, voidDarkening);
    m_profiler.worldTime = (glfwGetTime() - worldStart) * 1000.0;

    // Debug boundaries
    m_debugShader->use();
    m_debugShader->setMat4("projection", projection);
    m_debugShader->setMat4("view", view);
    m_worldRenderer->renderDebug(m_frustum, *m_debugShader, showBoundaries);

    double entityStart = glfwGetTime();
    // Disable culling for entities and hand to ensure all faces are visible
    glDisable(GL_CULL_FACE);
    renderEntities(partialTicks, projection, view, cameraMode);
    if (cameraMode == 0) renderFirstPersonArm(partialTicks, projection);
    glEnable(GL_CULL_FACE);
    m_profiler.entityTime = (glfwGetTime() - entityStart) * 1000.0;

    m_profiler.renderTime = (glfwGetTime() - renderStart) * 1000.0;

    double uiStart = glfwGetTime();
    renderUI(showDebug, showProfiler, fps, cameraMode);
    m_profiler.uiTime = (glfwGetTime() - uiStart) * 1000.0;
    
    m_profiler.frameTime = (glfwGetTime() - frameStart) * 1000.0;
    m_profiler.frameTimeHistory[m_profiler.historyIndex] = m_profiler.frameTime;
    m_profiler.historyIndex = (m_profiler.historyIndex + 1) % 128;
}

void GameRenderer::renderWorld(float partialTicks, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor, float voidDarkening) {
    m_basicShader->use();
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("model", glm::mat4(1.0f));
    m_basicShader->setBool("hasTexture", true);
    m_basicShader->setVec3("fogColor", fogColor);
    m_basicShader->setVec3("cameraPos", m_camera.position);
    m_basicShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    m_basicShader->setVec3("sunDirection", m_world.getSunDirection());
    m_basicShader->setFloat("uTime", (float)glfwGetTime());

    // Simple fog distance based on state
    m_basicShader->setFloat("fogNear", 64.0f); m_basicShader->setFloat("fogFar", 256.0f);

    m_renderEngine->bindTexture(m_terrainTex);
    m_frustum.update(projection * view);
    m_worldRenderer->updateDirtyMeshes(64);
    m_worldRenderer->render(m_frustum, *m_basicShader);
}

void GameRenderer::renderEntities(float partialTicks, const glm::mat4& projection, const glm::mat4& view, int cameraMode) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_entityShader->use();
    m_entityShader->setMat4("projection", projection);
    m_entityShader->setMat4("view", view);

    auto getEntityBrightness = [&](int x, int y, int z) {
        auto light = m_world.getLightPair(x, y, z);
        float skyVar2 = 1.0f - std::clamp((float)light.first, 0.0f, 15.0f) / 15.0f;
        float skyBr = (1.0f - skyVar2) / (skyVar2 * 3.0f + 1.0f) * 0.95f + 0.05f;
        float blockVar2 = 1.0f - std::clamp((float)light.second, 0.0f, 15.0f) / 15.0f;
        float blockBr = (1.0f - blockVar2) / (blockVar2 * 3.0f + 1.0f) * 0.95f + 0.05f;
        return std::max(skyBr * m_world.getDaylightStrength(), blockBr);
    };

    auto renderOne = [&](Entity* entity, float pTicks) {
        float b = getEntityBrightness((int)std::floor(entity->posX), (int)std::floor(entity->posY), (int)std::floor(entity->posZ));
        m_entityShader->setVec3("colorTint", glm::vec3(b));

        m_renderEngine->bindTexture(m_renderEngine->getTexture(dynamic_cast<EntityPlayer*>(entity) ? "/char.png" : "/mob/zombie.png"));

        double ex = entity->prevPosX + (entity->posX - entity->prevPosX) * pTicks;
        double ey = entity->prevPosY + (entity->posY - entity->prevPosY) * pTicks;
        double ez = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * pTicks;

        float renderYaw = 0.0f, interpYaw = entity->rotationYaw, headPitch = entity->rotationPitch;
        if (auto living = dynamic_cast<EntityLiving*>(entity)) {
            renderYaw = living->prevRenderYawOffset + (living->renderYawOffset - living->prevRenderYawOffset) * pTicks;
            interpYaw = living->prevRotationYaw + (living->rotationYaw - living->prevRotationYaw) * pTicks;
            headPitch = living->prevRotationPitch + (living->rotationPitch - living->prevRotationPitch) * pTicks;
        }

        float netHeadYaw = std::clamp(interpYaw - renderYaw, -75.0f, 75.0f);
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(ex, ey, ez));
        modelMat = glm::rotate(modelMat, glm::radians(180.0f - renderYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(-1.0f, -1.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.5f, 0.0f));

        float limbSwing = 0.0f, limbSwingAmount = 0.0f, swing = 0.0f;
        if (auto living = dynamic_cast<EntityLiving*>(entity)) {
            limbSwing = living->prevLimbSwing + (living->limbSwing - living->prevLimbSwing) * pTicks;
            limbSwingAmount = living->prevLimbSwingAmount + (living->limbSwingAmount - living->prevLimbSwingAmount) * pTicks;
            if (living->isSwinging) swing = ((float)living->swingProgressInt + pTicks) / 8.0f;
        }

        if (dynamic_cast<EntityPlayer*>(entity)) {
            m_playerModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)glfwGetTime(), netHeadYaw, -headPitch, 0.0625f, swing);
        } else {
            m_zombieModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)glfwGetTime(), netHeadYaw, -headPitch, 0.0625f, swing);
        }
    };

    for (const auto& entity : m_world.getEntities()) {
        renderOne(entity.get(), partialTicks);
    }

    if (cameraMode != 0) {
        renderOne(&m_player, partialTicks);
    }
}

void GameRenderer::renderFirstPersonArm(float partialTicks, const glm::mat4& projection) {
    glClear(GL_DEPTH_BUFFER_BIT);
    m_entityShader->use();
    m_entityShader->setMat4("projection", projection);
    m_entityShader->setMat4("view", glm::mat4(1.0f));
    auto getEntityBrightness = [&](int x, int y, int z) {
        auto light = m_world.getLightPair(x, y, z);
        float skyVar2 = 1.0f - std::clamp((float)light.first, 0.0f, 15.0f) / 15.0f;
        float skyBr = (1.0f - skyVar2) / (skyVar2 * 3.0f + 1.0f) * 0.95f + 0.05f;
        float blockVar2 = 1.0f - std::clamp((float)light.second, 0.0f, 15.0f) / 15.0f;
        float blockBr = (1.0f - blockVar2) / (blockVar2 * 3.0f + 1.0f) * 0.95f + 0.05f;
        return std::max(skyBr * m_world.getDaylightStrength(), blockBr);
    };

    m_entityShader->setVec3("colorTint", glm::vec3(getEntityBrightness((int)m_player.posX, (int)m_player.posY, (int)m_player.posZ)));
    m_renderEngine->bindTexture(m_renderEngine->getTexture("/char.png"));

    glm::mat4 armBase = glm::mat4(1.0f);

    // Hand Bobbing
    float bobDist = m_player.prevDistanceWalkedModified + (m_player.distanceWalkedModified - m_player.prevDistanceWalkedModified) * partialTicks;
    float bobStr = m_player.prevCameraYaw + (m_player.cameraYaw - m_player.prevCameraYaw) * partialTicks;
    float bobPitch = m_player.prevCameraPitch + (m_player.cameraPitch - m_player.prevCameraPitch) * partialTicks;

    armBase = glm::translate(armBase, glm::vec3(std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f, -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr), 0.0f));
    armBase = glm::rotate(armBase, glm::radians(std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    armBase = glm::rotate(armBase, glm::radians(std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    armBase = glm::rotate(armBase, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));

    // Swing Progress
    float swingProgress = m_player.isSwinging ? ((float)m_player.swingProgressInt + partialTicks) / 8.0f : 0.0f;
    if (swingProgress > 0.0f) {
        float f1 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
        armBase = glm::translate(armBase, glm::vec3(-f1 * 0.3f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.4f, -std::sin(swingProgress * glm::pi<float>()) * 0.4f));
    }

    armBase = glm::translate(armBase, glm::vec3(0.64f, -0.6f, -0.72f));
    armBase = glm::rotate(armBase, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (swingProgress > 0.0f) {
        armBase = glm::rotate(armBase, glm::radians(std::sin(std::sqrt(swingProgress) * glm::pi<float>()) * 70.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        armBase = glm::rotate(armBase, glm::radians(-std::sin(swingProgress * swingProgress * glm::pi<float>()) * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    armBase = glm::translate(armBase, glm::vec3(-1.0f, 3.6f, 3.5f));
    armBase = glm::rotate(armBase, glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    armBase = glm::rotate(armBase, glm::radians(200.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    armBase = glm::rotate(armBase, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    armBase = glm::translate(armBase, glm::vec3(5.6f, 0.0f, 0.0f));

    m_playerModel->renderFirstPersonArm(*m_entityShader, armBase, 0.0625f);
}

void GameRenderer::renderUI(bool showDebug, bool showProfiler, float fps, int cameraMode) {
    if (!showDebug) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Ensure UI isn't culled
    m_uiShader->use();
    m_uiShader->setMat4("projection", glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f));
    m_uiShader->setMat4("view", glm::mat4(1.0f));
    m_uiShader->setBool("hasTexture", true);

    const auto& stats = m_worldRenderer->getStats();
    char buf[1024];
    std::snprintf(buf, sizeof(buf), 
        "OurCraft Infdev\n"
        "FPS: %.0f (%.2f ms)\n"
        "Pos: %.3f, %.3f, %.3f\n"
        "Chunk: %d, %d\n"
        "Sections: %zu / %zu visible\n"
        "Triangles: %zu\n"
        "Mesh Builds: %zu (%.2f ms)\n"
        "Camera: %s",
        (double)fps, m_profiler.frameTime,
        m_player.posX, m_player.posY, m_player.posZ, 
        (int)std::floor(m_player.posX / 16.0), (int)std::floor(m_player.posZ / 16.0),
        stats.visibleSections, stats.sectionCount,
        stats.triangles,
        stats.meshBuilds, stats.meshBuildMs,
        cameraMode == 0 ? "First Person" : (cameraMode == 1 ? "Third Person Back" : "Third Person Front"));
    
    m_fontRenderer->drawString(*m_uiShader, buf, 2.0f, 2.0f, 0xFFFFFFFF);

    if (showProfiler) {
        std::snprintf(buf, sizeof(buf),
            "--- Profiler ---\n"
            "Update: %.2f ms\n"
            "Render Total: %.2f ms\n"
            "  World: %.2f ms\n"
            "  Entities: %.2f ms\n"
            "  UI: %.2f ms",
            m_profiler.updateTime, m_profiler.renderTime,
            m_profiler.worldTime, m_profiler.entityTime, m_profiler.uiTime);
        m_fontRenderer->drawString(*m_uiShader, buf, 2.0f, (float)m_height - 80.0f, 0xFFFFFFFF);

        // Frame time graph
        Tessellator* t = Tessellator::instance;
        m_uiShader->use();
        m_uiShader->setBool("hasTexture", false);
        t->startDrawingQuads();
        float gx = (float)m_width - 130.0f;
        float gy = (float)m_height - 10.0f;
        for (int i = 0; i < 128; ++i) {
            float val = (float)m_profiler.frameTimeHistory[(m_profiler.historyIndex + i) % 128];
            float h = std::clamp(val, 0.0f, 60.0f);
            uint32_t c = val > 16.66f ? 0xFFFF0000 : 0xFF00FF00; // Red if over 16.6ms (60FPS), else Green
            t->setColorOpaque_I(c);
            t->addVertex(gx + i, gy, 0);
            t->addVertex(gx + i + 1, gy, 0);
            t->addVertex(gx + i + 1, gy - h, 0);
            t->addVertex(gx + i, gy - h, 0);
        }
        t->draw();
        m_uiShader->setBool("hasTexture", true);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}
