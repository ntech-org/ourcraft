#include "renderer/GameRenderer.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include "renderer/TextureFX.hpp"
#include "gui/GuiScreen.hpp"

#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include "InputHandler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
float interpAngle(float prev, float current, float pTicks) {
    float diff = current - prev;
    while (diff < -180.0f) diff += 360.0f;
    while (diff >= 180.0f) diff -= 360.0f;
    return prev + diff * pTicks;
}
}

GameRenderer::GameRenderer(GLFWwindow* window, World& world, EntityPlayer& player)
    : m_window(window), m_world(world), m_player(player)
{
    glfwGetFramebufferSize(window, &m_width, &m_height);
    resize(m_width, m_height);

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
    m_textShader = std::make_unique<Shader>("assets/shaders/text.vert", "assets/shaders/text.frag");
    m_playerModel = std::make_unique<ModelBiped>();
    m_zombieModel = std::make_unique<ModelZombie>();

    m_renderEngine->registerTextureFX(std::make_unique<TextureWaterFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureWaterFlowFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureLavaFX>());
    m_renderEngine->registerTextureFX(std::make_unique<TextureLavaFlowFX>());
}

GameRenderer::~GameRenderer() {}

void GameRenderer::setBlockBreakingOverlay(bool active, int x, int y, int z, float progress) {
    m_breakOverlayActive = active;
    m_breakOverlayX = x;
    m_breakOverlayY = y;
    m_breakOverlayZ = z;
    m_breakOverlayProgress = std::clamp(progress, 0.0f, 1.0f);
}

void GameRenderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);

    m_guiScale = 1;
    int targetScale = m_player.getMinecraft().getSettings().guiScale;
    if (targetScale == 0) targetScale = 8; // Auto/Max

    while (m_guiScale < targetScale && m_width / (m_guiScale + 1) >= 320 && m_height / (m_guiScale + 1) >= 240) {
        m_guiScale++;
    }

    m_scaledWidth = (float)m_width / (float)m_guiScale;
    m_scaledHeight = (float)m_height / (float)m_guiScale;
}



void GameRenderer::render(float partialTicks, int cameraMode, bool showDebug, bool showBoundaries, bool showProfiler, float fps, std::shared_ptr<GuiScreen> currentScreen) {
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
        m_camera.position = glm::dvec3(px, py, pz);
    } else {
        m_camera.yaw = camYaw + 90.0f + (cameraMode == 2 ? 180.0f : 0.0f);
        m_camera.pitch = (cameraMode == 2 ? -camPitch : camPitch);
        m_camera.updateCameraVectors();
        m_camera.position = glm::dvec3(px, py, pz) - glm::dvec3(m_camera.front) * 4.0;
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

    float fov = m_player.getMinecraft().getSettings().fov;
    float armFov = 70.0f;
    if (m_player.isInsideOfMaterial(Material::water)) {
        fov = 60.0f;
        armFov = 60.0f;
    }

    const float aspect = m_height > 0 ? (float)m_width / (float)m_height : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.05f, 1000.0f);
    glm::mat4 armProjection = glm::perspective(glm::radians(armFov), aspect, 0.05f, 1000.0f);


    double renderStart = glfwGetTime();
    m_skyRenderer->render(m_world, m_camera, projection, view, fogColor);

    double worldStart = glfwGetTime();
    renderWorld(partialTicks, projection, view, fogColor, voidDarkening);
    m_profiler.worldTime = (glfwGetTime() - worldStart) * 1000.0;

    // Debug boundaries
    m_debugShader->use();
    m_debugShader->setMat4("projection", projection);
    m_debugShader->setMat4("view", view);
    m_worldRenderer->renderDebug(m_frustum, *m_debugShader, showBoundaries, m_camera.position);

    double entityStart = glfwGetTime();
    // Disable culling for entities and hand to ensure all faces are visible
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    renderEntities(partialTicks, projection, view, cameraMode, fogColor);
    glEnable(GL_CULL_FACE);
    m_profiler.entityTime = (glfwGetTime() - entityStart) * 1000.0;


    // Pass 2: Translucent world (water)
    m_renderEngine->bindTexture(m_terrainTex);
    m_worldRenderer->renderTranslucent(m_frustum, *m_basicShader, m_camera.position);

    // Final Pass: First person hand (on top of everything)
    if (cameraMode == 0) {
        glDisable(GL_CULL_FACE);
        renderFirstPersonArm(partialTicks, armProjection);
        glEnable(GL_CULL_FACE);
    }

    m_profiler.renderTime = (glfwGetTime() - renderStart) * 1000.0;



    double uiStart = glfwGetTime();
    renderUI(showDebug, showProfiler, fps, cameraMode);

    if (currentScreen) {
        double mx, my;
        glfwGetCursorPos(m_window, &mx, &my);

        int ww, wh, fw, fh;
        glfwGetWindowSize(m_window, &ww, &wh);
        glfwGetFramebufferSize(m_window, &fw, &fh);
        mx *= (double)fw / (double)ww;
        my *= (double)fh / (double)wh;

        mx /= (double)m_guiScale;
        my /= (double)m_guiScale;
        currentScreen->drawScreen((int)mx, (int)my, partialTicks);
    }

    m_profiler.uiTime = (glfwGetTime() - uiStart) * 1000.0;

    m_profiler.frameTime = (glfwGetTime() - frameStart) * 1000.0;
    m_profiler.frameTimeHistory[m_profiler.historyIndex] = m_profiler.frameTime;
    m_profiler.historyIndex = (m_profiler.historyIndex + 1) % 128;
}

void GameRenderer::renderWorld(float partialTicks, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor, float voidDarkening) {
    m_basicShader->use();
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view); // This is now rotation-only
    m_basicShader->setMat4("model", glm::mat4(1.0f));
    m_basicShader->setBool("hasTexture", true);
    m_basicShader->setVec3("fogColor", fogColor);
    m_basicShader->setVec3("cameraPos", glm::vec3(0.0f)); // Camera is at origin in relative space
    m_basicShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    m_basicShader->setVec3("sunDirection", m_world.getSunDirection());
    m_basicShader->setFloat("uTime", (float)glfwGetTime());

    // ... rest of fog logic ...
    if (m_player.isInsideOfMaterial(Material::water)) {
        m_basicShader->setInt("fogMode", 1);
        m_basicShader->setFloat("fogDensity", 0.1f);
    } else if (m_player.isInsideOfMaterial(Material::lava)) {
        m_basicShader->setInt("fogMode", 1);
        m_basicShader->setFloat("fogDensity", 2.0f);
    } else {
        m_basicShader->setInt("fogMode", 0);
        m_basicShader->setFloat("fogNear", 64.0f);
        m_basicShader->setFloat("fogFar", 256.0f);
    }


    m_renderEngine->bindTexture(m_terrainTex);
    m_frustum.update(projection * view);
    m_worldRenderer->updateDirtyMeshes(64);
    m_worldRenderer->renderOpaque(m_frustum, *m_basicShader, m_camera.position);
    renderBreakingOverlay();
}

void GameRenderer::renderBreakingOverlay() {
    if (!m_breakOverlayActive || m_breakOverlayProgress <= 0.0f) return;
    if (m_world.getBlockID(m_breakOverlayX, m_breakOverlayY, m_breakOverlayZ) == 0) return;

    const int stage = std::clamp((int)std::floor(m_breakOverlayProgress * 10.0f), 0, 9);
    const int tex = 240 + stage;
    const float u0 = (float)((tex & 15) * 16) / 256.0f;
    const float v0 = (float)((tex >> 4) * 16) / 256.0f;
    const float u1 = u0 + 16.0f / 256.0f;
    const float v1 = v0 + 16.0f / 256.0f;

    // Use relative coordinates
    const glm::vec3 relativePos = glm::vec3(glm::dvec3(m_breakOverlayX, m_breakOverlayY, m_breakOverlayZ) - m_camera.position);
    const float x0 = relativePos.x;
    const float y0 = relativePos.y;
    const float z0 = relativePos.z;
    const float x1 = x0 + 1.0f;
    const float y1 = y0 + 1.0f;
    const float z1 = z0 + 1.0f;
    const float eps = 0.001f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDisable(GL_CULL_FACE);

    m_renderEngine->bindTexture(m_terrainTex);
    Tessellator* t = Tessellator::instance;
    // Set identity model matrix for breaking overlay since we use absolute-relative coords
    m_basicShader->setMat4("model", glm::mat4(1.0f));
    t->startDrawingQuads();
    t->setColorRGBA(255, 255, 255, 180);

    // Bottom
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u0, v0);
    // Top
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u0, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v1);
    // North
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u0, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, u1, v1);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u0, v0);
    // South
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, u1, v0);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    // West
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v0);
    // East
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, u0, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, u1, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, u0, v0);
    t->draw();

    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
}

void GameRenderer::renderEntities(float partialTicks, const glm::mat4& projection, const glm::mat4& view, int cameraMode, const glm::vec3& fogColor) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_entityShader->use();
    m_entityShader->setMat4("projection", projection);
    m_entityShader->setMat4("view", view);

    if (m_player.isInsideOfMaterial(Material::water)) {
        m_entityShader->setInt("fogMode", 1);
        m_entityShader->setFloat("fogDensity", 0.1f);
    } else if (m_player.isInsideOfMaterial(Material::lava)) {
        m_entityShader->setInt("fogMode", 1);
        m_entityShader->setFloat("fogDensity", 2.0f);
    } else {
        m_entityShader->setInt("fogMode", 0);
        m_entityShader->setFloat("fogNear", 64.0f);
        m_entityShader->setFloat("fogFar", 256.0f);
    }
    m_entityShader->setVec3("fogColor", fogColor);
    m_entityShader->setVec3("cameraPos", glm::vec3(0.0f));


    auto getEntityBrightness = [&](double ex, double ey, double ez) {
        // Absolute coordinates for lighting lookup
        auto light1 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.1), (int)std::floor(ez));
        auto light2 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.9), (int)std::floor(ez));
        // ... rest ...
        int sky = std::max(light1.first, light2.first);
        int block = std::max(light1.second, light2.second);

        float skyVar2 = 1.0f - std::clamp((float)sky, 0.0f, 15.0f) / 15.0f;
        float skyBr = (1.0f - skyVar2) / (skyVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
        float blockVar2 = 1.0f - std::clamp((float)block, 0.0f, 15.0f) / 15.0f;
        float blockBr = (1.0f - blockVar2) / (blockVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
        return std::max(skyBr * m_world.getDaylightStrength(), blockBr);

    };

    auto renderOne = [&](Entity* entity, float pTicks) {
        double ex = entity->prevPosX + (entity->posX - entity->prevPosX) * (double)pTicks;
        double ey = entity->prevPosY + (entity->posY - entity->prevPosY) * (double)pTicks;
        double ez = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * (double)pTicks;

        float b = getEntityBrightness(ex, ey, ez);
        m_entityShader->setVec3("colorTint", glm::vec3(b));

        // Relative coordinates for rendering
        glm::vec3 relativePos = glm::vec3(glm::dvec3(ex, ey, ez) - m_camera.position);

        if (auto* item = dynamic_cast<EntityItem*>(entity)) {
            const bool isBlockItem = item->itemID > 0 && Block::blocksList[item->itemID] != nullptr;
            if (isBlockItem) {
                m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));
            } else {
                m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/items.png"));
            }

            const float u0 = (float)(((isBlockItem ? Block::blocksList[item->itemID]->blockIndexInTexture : item->itemID) & 15) * 16) / 256.0f;
            const float v0 = (float)(((isBlockItem ? Block::blocksList[item->itemID]->blockIndexInTexture : item->itemID) >> 4) * 16) / 256.0f;
            const float u1 = u0 + 16.0f / 256.0f;
            const float v1 = v0 + 16.0f / 256.0f;

            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(
                relativePos.x,
                relativePos.y + 0.15f + std::sin((float)glfwGetTime() * 2.0f + (float)item->entityID) * 0.05f,
                relativePos.z
            ));
            modelMat = glm::rotate(modelMat, glm::radians(-m_camera.yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMat = glm::scale(modelMat, glm::vec3(0.35f, 0.35f, 0.35f));
            m_entityShader->setMat4("model", modelMat);
            // ... rest of item rendering ...

            Tessellator* t = Tessellator::instance;
            if (isBlockItem) {
                t->startDrawingQuads();
                t->setColorOpaque(255, 255, 255);
                // top/bottom
                t->addVertexWithUV(-0.5f,  0.5f, -0.5f, u0, v1);
                t->addVertexWithUV( 0.5f,  0.5f, -0.5f, u1, v1);
                t->addVertexWithUV( 0.5f,  0.5f,  0.5f, u1, v0);
                t->addVertexWithUV(-0.5f,  0.5f,  0.5f, u0, v0);
                t->addVertexWithUV(-0.5f, -0.5f,  0.5f, u0, v1);
                t->addVertexWithUV( 0.5f, -0.5f,  0.5f, u1, v1);
                t->addVertexWithUV( 0.5f, -0.5f, -0.5f, u1, v0);
                t->addVertexWithUV(-0.5f, -0.5f, -0.5f, u0, v0);
                // north/south/east/west
                t->addVertexWithUV(-0.5f, -0.5f, -0.5f, u0, v1);
                t->addVertexWithUV( 0.5f, -0.5f, -0.5f, u1, v1);
                t->addVertexWithUV( 0.5f,  0.5f, -0.5f, u1, v0);
                t->addVertexWithUV(-0.5f,  0.5f, -0.5f, u0, v0);
                t->addVertexWithUV(-0.5f,  0.5f,  0.5f, u0, v0);
                t->addVertexWithUV( 0.5f,  0.5f,  0.5f, u1, v0);
                t->addVertexWithUV( 0.5f, -0.5f,  0.5f, u1, v1);
                t->addVertexWithUV(-0.5f, -0.5f,  0.5f, u0, v1);
                t->addVertexWithUV( 0.5f, -0.5f, -0.5f, u0, v1);
                t->addVertexWithUV( 0.5f, -0.5f,  0.5f, u1, v1);
                t->addVertexWithUV( 0.5f,  0.5f,  0.5f, u1, v0);
                t->addVertexWithUV( 0.5f,  0.5f, -0.5f, u0, v0);
                t->addVertexWithUV(-0.5f, -0.5f,  0.5f, u0, v1);
                t->addVertexWithUV(-0.5f, -0.5f, -0.5f, u1, v1);
                t->addVertexWithUV(-0.5f,  0.5f, -0.5f, u1, v0);
                t->addVertexWithUV(-0.5f,  0.5f,  0.5f, u0, v0);
                t->draw();
            } else {
                t->startDrawingQuads();
                t->setColorOpaque(255, 255, 255);
                t->addVertexWithUV(-0.5f, 0.0f, 0.0f, u0, v1);
                t->addVertexWithUV(0.5f, 0.0f, 0.0f, u1, v1);
                t->addVertexWithUV(0.5f, 1.0f, 0.0f, u1, v0);
                t->addVertexWithUV(-0.5f, 1.0f, 0.0f, u0, v0);
                t->draw();
            }
            return;
        }

        m_renderEngine->bindTexture(m_renderEngine->getTexture(dynamic_cast<EntityPlayer*>(entity) ? "/char.png" : "/mob/zombie.png"));

        float renderYaw = 0.0f, interpYaw = entity->rotationYaw, headPitch = entity->rotationPitch;
        if (auto living = dynamic_cast<EntityLiving*>(entity)) {
            renderYaw = interpAngle(living->prevRenderYawOffset, living->renderYawOffset, pTicks);
            interpYaw = interpAngle(living->prevRotationYaw, living->rotationYaw, pTicks);
            headPitch = living->prevRotationPitch + (living->rotationPitch - living->prevRotationPitch) * pTicks;
        }

        float netHeadYaw = std::clamp(interpYaw - renderYaw, -75.0f, 75.0f);
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), relativePos);
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
    m_entityShader->setVec3("cameraPos", glm::vec3(0.0f)); // View space, arm is at origin

    auto getEntityBrightness = [&](double ex, double ey, double ez) {
        auto light1 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.5), (int)std::floor(ez));
        auto light2 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 1.2), (int)std::floor(ez));
        int sky = std::max(light1.first, light2.first);
        int block = std::max(light1.second, light2.second);
        float skyVar2 = 1.0f - std::clamp((float)sky, 0.0f, 15.0f) / 15.0f;
        float skyBr = (1.0f - skyVar2) / (skyVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
        float blockVar2 = 1.0f - std::clamp((float)block, 0.0f, 15.0f) / 15.0f;
        float blockBr = (1.0f - blockVar2) / (blockVar2 * 3.0f + 1.0f) * 0.9f + 0.1f;
        return std::max(skyBr * m_world.getDaylightStrength(), blockBr);

    };

    m_entityShader->setVec3("colorTint", glm::vec3(getEntityBrightness(m_player.posX, m_player.posY, m_player.posZ)));

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
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Ensure UI isn't culled
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int sw, sh;
    glfwGetFramebufferSize(m_window, &sw, &sh);
    m_player.getMinecraft().getFont().setDisplayContext(sw, sh, (float)m_guiScale);

    glm::mat4 projection = glm::ortho(0.0f, m_scaledWidth, m_scaledHeight, 0.0f, -1.0f, 1.0f);

    glm::mat4 view = glm::mat4(1.0f);

    m_uiShader->use();
    m_uiShader->setMat4("projection", projection);
    m_uiShader->setMat4("view", view);

    m_textShader->use();
    m_textShader->setMat4("projection", projection);
    m_textShader->setMat4("view", view);

    m_uiShader->setBool("hasTexture", true);

    Tessellator::instance->setColorOpaque_I(0xFFFFFFFF);
    if (m_player.isInsideOfMaterial(Material::water)) {
        renderUnderwaterOverlay();
    }
    renderHUD();
    renderCrosshair();



    if (!showDebug) {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        return;
    }


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

    m_player.getMinecraft().getFont().drawString(getTextShader(), buf, 2.0f, 2.0f, 0xFFFFFFFF, false);

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
        m_player.getMinecraft().getFont().drawString(getTextShader(), buf, 2.0f, (float)m_scaledHeight - 80.0f, 0xFFFFFFFF, false);

        // Frame time graph
        Tessellator* t = Tessellator::instance;
        m_uiShader->use();
        m_uiShader->setBool("hasTexture", false);
        t->startDrawingQuads();
        float gx = (float)m_scaledWidth - 130.0f;
        float gy = (float)m_scaledHeight - 10.0f;

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

void GameRenderer::renderHUD() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/gui.png"));
    float centerX = m_scaledWidth / 2.0f;
    drawTexturedModalRect(centerX - 91.0f, m_scaledHeight - 22.0f, 0, 0, 182, 22); // Hotbar
    drawTexturedModalRect(centerX - 91.0f - 1.0f + (float)m_player.inventory.currentSlot * 20.0f, m_scaledHeight - 22.0f - 1.0f, 0, 22, 24, 22); // Selection

    auto drawBlockStack3D = [&](const Block* block, float x, float y) {
        if (!block) return;
        Tessellator* t = Tessellator::instance;
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));

        auto tileUV = [](int tex, float& u0, float& v0, float& u1, float& v1) {
            u0 = (float)((tex & 15) * 16) / 256.0f;
            v0 = (float)((tex >> 4) * 16) / 256.0f;
            u1 = u0 + 16.0f / 256.0f;
            v1 = v0 + 16.0f / 256.0f;
        };

        float u0, v0, u1, v1;
        const int texTop = block->getTexture(1);
        const int texSide = block->getTexture(2);

        float s = 1.15f; 
        float ox = x + 8.0f;
        float oy = y + 8.5f; // Moved up from 10.0f

        t->startDrawingQuads();
        tileUV(texTop, u0, v0, u1, v1);
        t->setColorRGBA(230, 230, 230, 255);
        t->addVertexWithUV(ox - 6.0f * s, oy - 4.0f * s, 0.0f, u0, v1);
        t->addVertexWithUV(ox, oy - 7.0f * s, 0.0f, u1, v1);
        t->addVertexWithUV(ox + 6.0f * s, oy - 4.0f * s, 0.0f, u1, v0);
        t->addVertexWithUV(ox, oy - 1.0f * s, 0.0f, u0, v0);

        tileUV(texSide, u0, v0, u1, v1);
        t->setColorRGBA(170, 170, 170, 255);
        t->addVertexWithUV(ox - 6.0f * s, oy - 4.0f * s, 0.0f, u0, v0);
        t->addVertexWithUV(ox, oy - 1.0f * s, 0.0f, u1, v0);
        t->addVertexWithUV(ox, oy + 6.0f * s, 0.0f, u1, v1);
        t->addVertexWithUV(ox - 6.0f * s, oy + 3.0f * s, 0.0f, u0, v1);

        t->setColorRGBA(200, 200, 200, 255);
        t->addVertexWithUV(ox, oy - 1.0f * s, 0.0f, u0, v0);
        t->addVertexWithUV(ox + 6.0f * s, oy - 4.0f * s, 0.0f, u1, v0);
        t->addVertexWithUV(ox + 6.0f * s, oy + 3.0f * s, 0.0f, u1, v1);
        t->addVertexWithUV(ox, oy + 6.0f * s, 0.0f, u0, v1);
        t->draw();
    };

    auto drawBlockStack2D = [&](const Block* block, float x, float y) {
        if (!block) return;
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));
        const int tex = block->getTexture(0);
        drawTexturedModalRect(x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
    };

    auto drawItemStack2D = [&](int itemID, float x, float y) {
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/items.png"));
        const int tex = itemID & 255;
        drawTexturedModalRect(x, y, (tex & 15) * 16, (tex >> 4) * 16, 16, 16);
    };

    for (int slot = 0; slot < InventoryPlayer::HOTBAR_SIZE; ++slot) {
        const ItemStack& stack = m_player.inventory.mainInventory[slot];
        if (stack.itemID <= 0 || stack.count <= 0) continue;

        m_uiShader->use();
        const float iconX = centerX - 91.0f + (float)slot * 20.0f + 3.0f;
        const float iconY = m_scaledHeight - 19.0f;
        if (Block::blocksList[stack.itemID]) {
            if (Block::blocksList[stack.itemID]->getRenderShape() == BlockRenderShape::Cross) {
                drawBlockStack2D(Block::blocksList[stack.itemID], iconX, iconY);
            } else {
                drawBlockStack3D(Block::blocksList[stack.itemID], iconX, iconY);
            }
        } else {
            drawItemStack2D(stack.itemID, iconX, iconY);
        }

        if (stack.count > 1) {
            char countBuf[8];
            std::snprintf(countBuf, sizeof(countBuf), "%d", stack.count);
            Font& font = m_player.getMinecraft().getFont();
            font.drawString(getTextShader(), countBuf, iconX + 16.0f - (float)font.getStringWidth(countBuf), iconY + 9.0f, 0xFFFFFFFF, true);
        }
    }

    if (m_player.gameMode == GameMode::SURVIVAL) {
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/icons.png"));
        for (int i = 0; i < 10; ++i) {
            float x = centerX - 91.0f + (float)i * 8.0f;
            float y = m_scaledHeight - 32.0f;
            drawTexturedModalRect(x, y, 16, 0, 9, 9); // Empty heart
            if (i * 2 + 1 < m_player.health) drawTexturedModalRect(x, y, 52, 0, 9, 9); // Full heart
            else if (i * 2 + 1 == m_player.health) drawTexturedModalRect(x, y, 61, 0, 9, 9); // Half heart
        }

        if (m_player.isInsideOfMaterial(Material::water)) {
            int air = (int)std::ceil((double)(m_player.air - 2) * 10.0 / 300.0);
            int extraAir = (int)std::ceil((double)m_player.air * 10.0 / 300.0) - air;
            for (int i = 0; i < air + extraAir; ++i) {
                if (i < air) {
                    drawTexturedModalRect(centerX - 91.0f + (float)i * 8.0f, m_scaledHeight - 32.0f - 9.0f, 16, 18, 9, 9);
                } else {
                    drawTexturedModalRect(centerX - 91.0f + (float)i * 8.0f, m_scaledHeight - 32.0f - 9.0f, 25, 18, 9, 9);
                }
            }
        }
    }

    glDisable(GL_BLEND);
}

void GameRenderer::renderCrosshair() {
    m_uiShader->use();
    m_uiShader->setBool("hasTexture", true);
    m_uiShader->setMat4("projection", glm::ortho(0.0f, m_scaledWidth, m_scaledHeight, 0.0f, -1.0f, 1.0f));
    m_uiShader->setMat4("view", glm::mat4(1.0f));

    m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/icons.png"));
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR); // Invert colors
    drawTexturedModalRect(m_scaledWidth / 2.0f - 8.0f, m_scaledHeight / 2.0f - 8.0f, 0, 0, 16, 16);
    glDisable(GL_BLEND);
}


void GameRenderer::renderUnderwaterOverlay() {
    m_renderEngine->bindTexture(m_renderEngine->getTexture("/water.png"));

    Tessellator* t = Tessellator::instance;
    float b = m_world.getDaylightStrength();
    m_uiShader->use();
    m_uiShader->setBool("hasTexture", true);
    t->startDrawingQuads();
    t->setColorRGBA((int)(b * 255), (int)(b * 255), (int)(b * 255), 128); // 0.5 opacity

    float warp = 4.0f;
    float uOff = -m_player.rotationYaw / 64.0f;
    float vOff = m_player.rotationPitch / 64.0f;

    t->addVertexWithUV(0, m_scaledHeight, -90, uOff, vOff + warp);
    t->addVertexWithUV(m_scaledWidth, m_scaledHeight, -90, uOff + warp, vOff + warp);
    t->addVertexWithUV(m_scaledWidth, 0, -90, uOff + warp, vOff);
    t->addVertexWithUV(0, 0, -90, uOff, vOff);
    t->draw();
}

void GameRenderer::drawTexturedModalRect(float x, float y, int u, int v, int width, int height) {

    float f = 0.00390625f; // 1/256
    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();
    t->setColorOpaque_I(0xFFFFFFFF);
    t->addVertexWithUV(x, y + (float)height, 0.0f, (float)u * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y + (float)height, 0.0f, (float)(u + width) * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y, 0.0f, (float)(u + width) * f, (float)v * f);
    t->addVertexWithUV(x, y, 0.0f, (float)u * f, (float)v * f);
    t->draw();
}
