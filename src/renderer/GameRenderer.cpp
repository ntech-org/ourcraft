#include "renderer/GameRenderer.hpp"
#include "renderer/ItemRenderer.hpp"
#include "renderer/EntityRenderHelper.hpp"
#include "renderer/UIRenderHelper.hpp"
#include "renderer/TextureFX.hpp"
#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include "items/Item.hpp"
#include "gui/GuiScreen.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include "entities/EntityPlayer.hpp"
#include "InputHandler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

GameRenderer::GameRenderer(SDL_Window* window, World& world, EntityPlayer& player)
    : m_window(window), m_world(world), m_player(player)
{
    SDL_GetWindowSizeInPixels(window, &m_width, &m_height);
    resize(m_width, m_height);

    m_renderEngine = std::make_unique<RenderEngine>();

    m_equippedProgress = 1.0f;
    m_prevEquippedProgress = 1.0f;
    m_itemToRenderID = m_player.inventory.getCurrentItemID();

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
    if (targetScale == 0) targetScale = 8;

    while (m_guiScale < targetScale && m_width / (m_guiScale + 1) >= 320 && m_height / (m_guiScale + 1) >= 240) {
        m_guiScale++;
    }

    m_scaledWidth = (float)m_width / (float)m_guiScale;
    m_scaledHeight = (float)m_height / (float)m_guiScale;
}

void GameRenderer::render(float partialTicks, int cameraMode, bool showDebug, bool showBoundaries, bool showProfiler, float fps, std::shared_ptr<GuiScreen> currentScreen) {
    double frameStart = (double)SDL_GetTicksNS() / 1e9;
    m_world.pollGeneratedChunks();
    for (auto& newChunk : m_world.popNewChunks()) {
        m_worldRenderer->addSectionsForChunk(newChunk);
    }

    double px = m_player.prevPosX + (m_player.posX - m_player.prevPosX) * (double)partialTicks;
    double py = m_player.prevPosY + (m_player.posY - m_player.prevPosY) * (double)partialTicks + (double)m_player.yOffset;
    double pz = m_player.prevPosZ + (m_player.posZ - m_player.prevPosZ) * (double)partialTicks;

    float camYaw = m_player.rotationYaw;
    float camPitch = m_player.rotationPitch;

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
        view = computeViewBobMatrix(m_player, partialTicks) * m_camera.getViewMatrix();
    } else {
        view = view * m_camera.getViewMatrix();
    }

    int playerCX = (int)std::floor(px / 16.0);
    int playerCZ = (int)std::floor(pz / 16.0);

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

    double renderStart = (double)SDL_GetTicksNS() / 1e9;
    m_skyRenderer->render(m_world, m_camera, projection, view, fogColor);

    double worldStart = (double)SDL_GetTicksNS() / 1e9;
    renderWorld(partialTicks, projection, view, fogColor, voidDarkening);
    m_profiler.worldTime = ((double)SDL_GetTicksNS() / 1e9 - worldStart) * 1000.0;

    m_debugShader->use();
    m_debugShader->setMat4("projection", projection);
    m_debugShader->setMat4("view", view);
    m_worldRenderer->renderDebug(m_frustum, *m_debugShader, showBoundaries, m_camera.position);

    double entityStart = (double)SDL_GetTicksNS() / 1e9;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    renderEntities(partialTicks, projection, view, cameraMode, fogColor);
    glEnable(GL_CULL_FACE);
    m_profiler.entityTime = ((double)SDL_GetTicksNS() / 1e9 - entityStart) * 1000.0;

    m_renderEngine->bindTexture(m_terrainTex);
    m_worldRenderer->renderTranslucent(m_frustum, *m_basicShader, m_camera.position);

    if (cameraMode == 0) {
        glDisable(GL_CULL_FACE);
        renderFirstPersonArm(partialTicks, armProjection);
        glEnable(GL_CULL_FACE);
    }

    m_profiler.renderTime = ((double)SDL_GetTicksNS() / 1e9 - renderStart) * 1000.0;

    double uiStart = (double)SDL_GetTicksNS() / 1e9;
    renderUI(showDebug, showProfiler, fps, cameraMode);

    if (currentScreen) {
        float mx, my;
        SDL_GetMouseState(&mx, &my);

        int ww, wh, fw, fh;
        SDL_GetWindowSize(m_window, &ww, &wh);
        SDL_GetWindowSizeInPixels(m_window, &fw, &fh);
        mx *= (float)fw / (float)ww;
        my *= (float)fh / (float)wh;

        mx /= (float)m_guiScale;
        my /= (float)m_guiScale;
        currentScreen->drawScreen((int)mx, (int)my, partialTicks);
    }

    m_profiler.uiTime = ((double)SDL_GetTicksNS() / 1e9 - uiStart) * 1000.0;

    m_profiler.frameTime = ((double)SDL_GetTicksNS() / 1e9 - frameStart) * 1000.0;
    m_profiler.frameTimeHistory[m_profiler.historyIndex] = m_profiler.frameTime;
    m_profiler.historyIndex = (m_profiler.historyIndex + 1) % 128;
}

void GameRenderer::updateItemEquippedProgress() {
    m_prevEquippedProgress = m_equippedProgress;

    int currentID = m_player.inventory.getCurrentItemID();
    float speed = 0.4f;
    float target = (currentID == m_itemToRenderID) ? 1.0f : 0.0f;
    float delta = target - m_equippedProgress;

    if (delta < -speed) delta = -speed;
    if (delta > speed) delta = speed;

    m_equippedProgress += delta;

    if (m_equippedProgress < 0.1f) {
        m_itemToRenderID = currentID;
    }
}

void GameRenderer::renderWorld(float partialTicks, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor, float voidDarkening) {
    m_basicShader->use();
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("model", glm::mat4(1.0f));
    m_basicShader->setBool("hasTexture", true);
    m_basicShader->setVec3("fogColor", fogColor);
    m_basicShader->setVec3("cameraPos", glm::vec3(0.0f));
    m_basicShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    m_basicShader->setVec3("sunDirection", m_world.getSunDirection());
    m_basicShader->setFloat("uTime", (float)((double)SDL_GetTicksNS() / 1e9));

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
    renderSelectionBox(projection, view);
    renderBreakingOverlay(projection, view);
}

void GameRenderer::renderSelectionBox(const glm::mat4& projection, const glm::mat4& view) {
    const HitResult& hit = m_player.getMinecraft().getObjectMouseOver();
    if (hit.type != HitType::BLOCK) return;

    int x = hit.x;
    int y = hit.y;
    int z = hit.z;

    const glm::vec3 relativePos = glm::vec3(glm::dvec3(x, y, z) - m_camera.position);
    float x0 = relativePos.x - 0.002f;
    float y0 = relativePos.y - 0.002f;
    float z0 = relativePos.z - 0.002f;
    float x1 = x0 + 1.004f;
    float y1 = y0 + 1.004f;
    float z1 = z0 + 1.004f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glLineWidth(2.0f);

    m_uiShader->use();
    m_uiShader->setMat4("projection", projection);
    m_uiShader->setMat4("view", view);
    m_uiShader->setBool("hasTexture", false);

    Tessellator* t = Tessellator::instance;
    t->startDrawing(GL_LINES);
    t->setColorRGBA(0, 0, 0, 102);

    t->addVertex(x0, y0, z0); t->addVertex(x1, y0, z0);
    t->addVertex(x1, y0, z0); t->addVertex(x1, y0, z1);
    t->addVertex(x1, y0, z1); t->addVertex(x0, y0, z1);
    t->addVertex(x0, y0, z1); t->addVertex(x0, y0, z0);

    t->addVertex(x0, y1, z0); t->addVertex(x1, y1, z0);
    t->addVertex(x1, y1, z0); t->addVertex(x1, y1, z1);
    t->addVertex(x1, y1, z1); t->addVertex(x0, y1, z1);
    t->addVertex(x0, y1, z1); t->addVertex(x0, y1, z0);

    t->addVertex(x0, y0, z0); t->addVertex(x0, y1, z0);
    t->addVertex(x1, y0, z0); t->addVertex(x1, y1, z0);
    t->addVertex(x1, y0, z1); t->addVertex(x1, y1, z1);
    t->addVertex(x0, y0, z1); t->addVertex(x0, y1, z1);

    t->draw();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void GameRenderer::renderBreakingOverlay(const glm::mat4& projection, const glm::mat4& view) {
    if (!m_breakOverlayActive || m_breakOverlayProgress <= 0.0f) return;
    if (m_world.getBlockID(m_breakOverlayX, m_breakOverlayY, m_breakOverlayZ) == 0) return;

    const int stage = std::clamp((int)std::floor(m_breakOverlayProgress * 10.0f), 0, 9);
    const int tex = 240 + stage;
    const FaceUV uv = getTextureUV(tex);

    const glm::vec3 relativePos = glm::vec3(glm::dvec3(m_breakOverlayX, m_breakOverlayY, m_breakOverlayZ) - m_camera.position);
    const float x0 = relativePos.x;
    const float y0 = relativePos.y;
    const float z0 = relativePos.z;
    const float x1 = x0 + 1.0f;
    const float y1 = y0 + 1.0f;
    const float z1 = z0 + 1.0f;
    const float eps = 0.001f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDisable(GL_CULL_FACE);

    m_renderEngine->bindTexture(m_terrainTex);
    Tessellator* t = Tessellator::instance;

    m_uiShader->use();
    m_uiShader->setMat4("projection", projection);
    m_uiShader->setMat4("view", view);
    m_uiShader->setBool("hasTexture", true);

    t->startDrawingQuads();
    t->setColorRGBA(255, 255, 255, 255);

    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, uv.u0, uv.v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, uv.u1, uv.v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, uv.u1, uv.v0);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, uv.u0, uv.v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, uv.u0, uv.v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, uv.u1, uv.v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, uv.u1, uv.v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, uv.u0, uv.v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, uv.u0, uv.v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, uv.u1, uv.v1);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, uv.u1, uv.v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, uv.u0, uv.v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, uv.u0, uv.v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, uv.u1, uv.v0);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, uv.u1, uv.v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, uv.u0, uv.v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, uv.u0, uv.v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, uv.u1, uv.v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, uv.u1, uv.v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, uv.u0, uv.v0);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, uv.u0, uv.v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, uv.u1, uv.v1);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, uv.u1, uv.v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, uv.u0, uv.v0);
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
    m_entityShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    m_entityShader->setVec3("sunDirection", m_world.getSunDirection());

    for (const auto& entity : m_world.getEntities()) {
        renderEntity(*entity.get(), partialTicks, m_world, m_camera, *m_entityShader, *m_renderEngine, *m_playerModel, *m_zombieModel);
        if (auto* player = dynamic_cast<EntityPlayer*>(entity.get())) {
            ::renderThirdPersonHeldItem(player, partialTicks, m_camera.position, *m_entityShader, *m_renderEngine, *m_playerModel);
        }
    }

    if (cameraMode != 0) {
        renderEntity(m_player, partialTicks, m_world, m_camera, *m_entityShader, *m_renderEngine, *m_playerModel, *m_zombieModel);
        ::renderThirdPersonHeldItem(&m_player, partialTicks, m_camera.position, *m_entityShader, *m_renderEngine, *m_playerModel);
    }
}

void GameRenderer::renderFirstPersonArm(float partialTicks, const glm::mat4& projection) {
    ::renderFirstPersonArm(*this, m_player, m_world, m_camera, partialTicks, projection,
                           m_equippedProgress, m_prevEquippedProgress, m_itemToRenderID);
}

void GameRenderer::renderThirdPersonHeldItem(EntityPlayer* player, float partialTicks, const glm::dvec3& cameraPos) {
    ::renderThirdPersonHeldItem(player, partialTicks, cameraPos, *m_entityShader, *m_renderEngine, *m_playerModel);
}

void GameRenderer::renderUI(bool showDebug, bool showProfiler, float fps, int cameraMode) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int sw, sh;
    SDL_GetWindowSizeInPixels(m_window, &sw, &sh);
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
        ::renderUnderwaterOverlay(*m_uiShader, *m_renderEngine, m_player, m_world, m_scaledWidth, m_scaledHeight);
    }
    ::renderHUD(*this, m_player, *m_uiShader, *m_textShader, *m_renderEngine, m_scaledWidth, m_scaledHeight);

    m_uiShader->use();
    m_uiShader->setBool("hasTexture", true);
    ::renderCrosshair(*this, *m_renderEngine, m_scaledWidth, m_scaledHeight);

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

        Tessellator* t = Tessellator::instance;
        m_uiShader->use();
        m_uiShader->setBool("hasTexture", false);
        t->startDrawingQuads();
        float gx = (float)m_scaledWidth - 130.0f;
        float gy = (float)m_scaledHeight - 10.0f;

        for (int i = 0; i < 128; ++i) {
            float val = (float)m_profiler.frameTimeHistory[(m_profiler.historyIndex + i) % 128];
            float h = std::clamp(val, 0.0f, 60.0f);
            uint32_t c = val > 16.66f ? 0xFFFF0000 : 0xFF00FF00;
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

void GameRenderer::drawTexturedModalRect(float x, float y, int u, int v, int width, int height) {
    float f = 0.00390625f;
    Tessellator* t = Tessellator::instance;
    t->startDrawingQuads();
    t->setColorOpaque_I(0xFFFFFFFF);
    t->addVertexWithUV(x, y + (float)height, 0.0f, (float)u * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y + (float)height, 0.0f, (float)(u + width) * f, (float)(v + height) * f);
    t->addVertexWithUV(x + (float)width, y, 0.0f, (float)(u + width) * f, (float)v * f);
    t->addVertexWithUV(x, y, 0.0f, (float)u * f, (float)v * f);
    t->draw();
}
