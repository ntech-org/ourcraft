#include "renderer/GameRenderer.hpp"
#include "Minecraft.hpp"
#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include "renderer/TextureFX.hpp"
#include "gui/GuiScreen.hpp"
#include "items/Item.hpp"

#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include "InputHandler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
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

struct FaceUV {
    float u0;
    float v0;
    float u1;
    float v1;
};

FaceUV getTextureUV(int tex) {
    float u0 = (float)((tex & 15) * 16) / 256.0f;
    float v0 = (float)((tex >> 4) * 16) / 256.0f;
    return {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};
}

bool isInventoryBlockModel(int itemID) {
    return itemID > 0
        && itemID < 256
        && Block::blocksList[itemID]
        && Block::blocksList[itemID]->getRenderShape() == BlockRenderShape::FullCube;
}

int getItemIconTexture(int itemID) {
    if (itemID > 0 && itemID < 256 && Block::blocksList[itemID]) {
        return Block::blocksList[itemID]->getTexture(2);
    }
    if (itemID >= 0 && itemID < 1024 && Item::itemsList[itemID]) {
        return Item::itemsList[itemID]->iconIndex;
    }
    return itemID & 255;
}

void addFace(Tessellator* t, int side, const FaceUV& uv, float shade) {
    int c = std::clamp((int)std::round(255.0f * shade), 0, 255);
    t->setColorOpaque(c, c, c);

    const float x0 = -0.5f, x1 = 0.5f;
    const float y0 = -0.5f, y1 = 0.5f;
    const float z0 = -0.5f, z1 = 0.5f;

    switch (side) {
        case 0: // bottom
            t->setNormal(0.0f, -1.0f, 0.0f);
            t->addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
            break;
        case 1: // top
            t->setNormal(0.0f, 1.0f, 0.0f);
            t->addVertexWithUV(x1, y1, z1, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y1, z1, uv.u0, uv.v1);
            break;
        case 2: // front (toward player)
            t->setNormal(0.0f, 0.0f, 1.0f);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v1);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v1);
            break;
        case 3: // back (away from player)
            t->setNormal(0.0f, 0.0f, -1.0f);
            t->addVertexWithUV(x0, y1, z1, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z1, uv.u1, uv.v0);
            break;
        case 4: // left
            t->setNormal(-1.0f, 0.0f, 0.0f);
            t->addVertexWithUV(x0, y1, z1, uv.u1, uv.v0);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v1);
            t->addVertexWithUV(x0, y0, z1, uv.u1, uv.v1);
            break;
        case 5: // right
            t->setNormal(1.0f, 0.0f, 0.0f);
            t->addVertexWithUV(x1, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y1, z1, uv.u0, uv.v0);
            break;
    }
}

void renderFlatHeldItem(Tessellator* t, const FaceUV& uv) {
    const float w = 1.0f;
    const float h = 1.0f;
    const float d = 1.0f / 16.0f;
    const float eps = 0.001953125f;

    // Front (facing +Z)
    t->setNormal(0.0f, 0.0f, 1.0f);
    t->addVertexWithUV(0.0f, 0.0f, 0.0f, uv.u1, uv.v1);
    t->addVertexWithUV(w, 0.0f, 0.0f, uv.u0, uv.v1);
    t->addVertexWithUV(w, h, 0.0f, uv.u0, uv.v0);
    t->addVertexWithUV(0.0f, h, 0.0f, uv.u1, uv.v0);

    // Back (facing -Z)
    t->setNormal(0.0f, 0.0f, -1.0f);
    t->addVertexWithUV(0.0f, h, -d, uv.u1, uv.v0);
    t->addVertexWithUV(w, h, -d, uv.u0, uv.v0);
    t->addVertexWithUV(w, 0.0f, -d, uv.u0, uv.v1);
    t->addVertexWithUV(0.0f, 0.0f, -d, uv.u1, uv.v1);

    // Left strips (facing -X)
    t->setNormal(-1.0f, 0.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideU = uv.u1 + (uv.u0 - uv.u1) * step - eps;
        float x = w * step;

        t->addVertexWithUV(x, 0.0f, -d, sideU, uv.v1);
        t->addVertexWithUV(x, 0.0f, 0.0f, sideU, uv.v1);
        t->addVertexWithUV(x, h, 0.0f, sideU, uv.v0);
        t->addVertexWithUV(x, h, -d, sideU, uv.v0);
    }

    // Right strips (facing +X)
    t->setNormal(1.0f, 0.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideU = uv.u1 + (uv.u0 - uv.u1) * step - eps;
        float x = w * step + d;

        t->addVertexWithUV(x, h, -d, sideU, uv.v0);
        t->addVertexWithUV(x, h, 0.0f, sideU, uv.v0);
        t->addVertexWithUV(x, 0.0f, 0.0f, sideU, uv.v1);
        t->addVertexWithUV(x, 0.0f, -d, sideU, uv.v1);
    }

    // Top strips (facing +Y)
    t->setNormal(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideV = uv.v1 + (uv.v0 - uv.v1) * step - eps;
        float y = h * step + d;

        t->addVertexWithUV(0.0f, y, 0.0f, uv.u1, sideV);
        t->addVertexWithUV(w, y, 0.0f, uv.u0, sideV);
        t->addVertexWithUV(w, y, -d, uv.u0, sideV);
        t->addVertexWithUV(0.0f, y, -d, uv.u1, sideV);
    }

    // Bottom strips (facing -Y)
    t->setNormal(0.0f, -1.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideV = uv.v1 + (uv.v0 - uv.v1) * step - eps;
        float y = h * step;

        t->addVertexWithUV(w, y, 0.0f, uv.u0, sideV);
        t->addVertexWithUV(0.0f, y, 0.0f, uv.u1, sideV);
        t->addVertexWithUV(0.0f, y, -d, uv.u1, sideV);
        t->addVertexWithUV(w, y, -d, uv.u0, sideV);
    }
}
}

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
    if (targetScale == 0) targetScale = 8; // Auto/Max

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
        float bobDist = m_player.prevDistanceWalkedModified + (m_player.distanceWalkedModified - m_player.prevDistanceWalkedModified) * partialTicks;
        float bobStr = m_player.prevCameraYaw + (m_player.cameraYaw - m_player.prevCameraYaw) * partialTicks;
        float bobPitch = m_player.prevCameraPitch + (m_player.cameraPitch - m_player.prevCameraPitch) * partialTicks;

        view = glm::translate(view, glm::vec3(std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f, -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr), 0.0f));
        view = glm::rotate(view, glm::radians(std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        view = glm::rotate(view, glm::radians(std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    view = view * m_camera.getViewMatrix();

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
    t->setColorRGBA(0, 0, 0, 102); // 0.4 alpha black

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
    const float u0 = (float)((tex & 15) * 16) / 256.0f;
    const float v0 = (float)((tex >> 4) * 16) / 256.0f;
    const float u1 = u0 + 16.0f / 256.0f;
    const float v1 = v0 + 16.0f / 256.0f;

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

    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u0, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u0, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u0, v1);
    t->addVertexWithUV(x1 + eps, y0 - eps, z0 - eps, u1, v1);
    t->addVertexWithUV(x1 + eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u0, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v0);
    t->addVertexWithUV(x1 + eps, y1 + eps, z1 + eps, u1, v0);
    t->addVertexWithUV(x1 + eps, y0 - eps, z1 + eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z1 + eps, u0, v1);
    t->addVertexWithUV(x0 - eps, y0 - eps, z0 - eps, u1, v1);
    t->addVertexWithUV(x0 - eps, y1 + eps, z0 - eps, u1, v0);
    t->addVertexWithUV(x0 - eps, y1 + eps, z1 + eps, u0, v0);
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
    m_entityShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    m_entityShader->setVec3("sunDirection", m_world.getSunDirection());


    auto getEntityBrightness = [&](double ex, double ey, double ez) {
        auto light1 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.1), (int)std::floor(ez));
        auto light2 = m_world.getLightPair((int)std::floor(ex), (int)std::floor(ey + 0.9), (int)std::floor(ez));
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

        glm::vec3 relativePos = glm::vec3(glm::dvec3(ex, ey, ez) - m_camera.position);

        if (auto* item = dynamic_cast<EntityItem*>(entity)) {
            const bool isBlockItem = isInventoryBlockModel(item->itemID);
            if (isBlockItem) {
                m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));
            } else {
                m_renderEngine->bindTexture(m_renderEngine->getTexture(item->itemID < 256 ? "/terrain.png" : "/gui/items.png"));
            }

            const int tex = getItemIconTexture(item->itemID);

            const float spin = (((float)item->age + partialTicks) / 20.0f + item->hoverStart) * 57.29578f;
            float bob = std::sin(((float)item->age + partialTicks) / 10.0f + item->hoverStart) * 0.1f + 0.38f;
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(
                relativePos.x,
                relativePos.y + bob,
                relativePos.z
            ));
            if (isBlockItem) {
                modelMat = glm::rotate(modelMat, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
            } else {
                modelMat = glm::rotate(modelMat, glm::radians(-m_camera.yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            float itemScale = isBlockItem ? 0.25f : 0.5f;
            if (item->pickupAnimationTicks > 0 && item->pickupAnimationTotalTicks > 0) {
                float pickupProgress = 1.0f - ((float)item->pickupAnimationTicks / (float)item->pickupAnimationTotalTicks);
                itemScale *= 1.0f - pickupProgress * 0.55f;
            }
            modelMat = glm::scale(modelMat, glm::vec3(itemScale, itemScale, itemScale));
            m_entityShader->setMat4("model", modelMat);

            Tessellator* t = Tessellator::instance;
            if (isBlockItem) {
                Block* block = Block::blocksList[item->itemID];
                static constexpr float kFaceShade[6] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
                t->startDrawingQuads();
                for (int side = 0; side < 6; ++side) {
                    addFace(t, side, getTextureUV(block->getTexture(side)), kFaceShade[side]);
                }
                t->draw();
            } else {
                const FaceUV uv = getTextureUV(tex);
                t->startDrawingQuads();
                t->setColorOpaque(255, 255, 255);
                t->addVertexWithUV(-0.5f, 0.0f, 0.0f, uv.u0, uv.v1);
                t->addVertexWithUV(0.5f, 0.0f, 0.0f, uv.u1, uv.v1);
                t->addVertexWithUV(0.5f, 1.0f, 0.0f, uv.u1, uv.v0);
                t->addVertexWithUV(-0.5f, 1.0f, 0.0f, uv.u0, uv.v0);
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

        if (auto player = dynamic_cast<EntityPlayer*>(entity)) {
            m_playerModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)((double)SDL_GetTicksNS() / 1e9), netHeadYaw, -headPitch, 0.0625f, swing);
            renderThirdPersonHeldItem(player, pTicks, modelMat);
        } else {
            m_zombieModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)((double)SDL_GetTicksNS() / 1e9), netHeadYaw, -headPitch, 0.0625f, swing);
        }
    };

    for (const auto& entity : m_world.getEntities()) {
        renderOne(entity.get(), partialTicks);
    }

    if (cameraMode != 0) {
        renderOne(&m_player, partialTicks);
    }
}

void GameRenderer::renderThirdPersonHeldItem(EntityPlayer* player, float partialTicks, const glm::mat4& modelMat) {
    const ItemStack& stack = player->inventory.getCurrentStack();
    if (stack.isEmpty()) return;

    glm::mat4 heldMat = modelMat;
    
    // bipedRightArm rotation point is (-5, 2, 0) in model space
    heldMat = glm::translate(heldMat, glm::vec3(-5.0f / 16.0f, 2.0f / 16.0f, 0.0f));
    
    heldMat = glm::rotate(heldMat, m_playerModel->bipedRightArm->rotateAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));
    heldMat = glm::rotate(heldMat, m_playerModel->bipedRightArm->rotateAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
    heldMat = glm::rotate(heldMat, m_playerModel->bipedRightArm->rotateAngleX, glm::vec3(1.0f, 0.0f, 0.0f));
    
    heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.45f, 0.0f)); 
    
    const bool isBlock = isInventoryBlockModel(stack.itemID);
    if (isBlock) {
        heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.1875f, -0.3125f));
        heldMat = glm::rotate(heldMat, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float s = 0.375f;
        heldMat = glm::scale(heldMat, glm::vec3(s, -s, s));
        
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));
        m_entityShader->setMat4("model", heldMat);
        m_entityShader->setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));
        
        Tessellator* t = Tessellator::instance;
        Block* block = Block::blocksList[stack.itemID];
        static constexpr float kFaceShade[6] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
        t->startDrawingQuads();
        for (int side = 0; side < 6; ++side) {
            int tex = block->getTexture(side);
            float u0 = (float)((tex & 15) << 4) / 256.0f;
            float v0 = (float)((tex & 240)) / 256.0f;
            FaceUV uv = {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};
            addFace(t, side, uv, kFaceShade[side]);
        }
        t->draw();
    } else {
        heldMat = glm::translate(heldMat, glm::vec3(0.0f, 0.1875f, 0.0f));
        float s = 0.4f;
        heldMat = glm::scale(heldMat, glm::vec3(s, s, s));
        heldMat = glm::rotate(heldMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        
        int tex = getItemIconTexture(stack.itemID);
        float u0 = (float)((tex & 15) << 4) / 256.0f;
        float v0 = (float)((tex & 240)) / 256.0f;
        FaceUV uv = {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};

        m_renderEngine->bindTexture(m_renderEngine->getTexture(stack.itemID < 256 ? "/terrain.png" : "/gui/items.png"));
        m_entityShader->setMat4("model", heldMat);
        m_entityShader->setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

        Tessellator* t = Tessellator::instance;
        t->startDrawingQuads();
        t->setColorOpaque(255, 255, 255);
        renderFlatHeldItem(t, uv);
        t->draw();
    }
}
void GameRenderer::renderFirstPersonArm(float partialTicks, const glm::mat4& projection) {
    glClear(GL_DEPTH_BUFFER_BIT);
    m_entityShader->use();
    m_entityShader->setMat4("projection", projection);
    m_entityShader->setMat4("view", glm::mat4(1.0f));
    m_entityShader->setVec3("cameraPos", glm::vec3(0.0f));
    m_entityShader->setFloat("daylightFactor", m_world.getDaylightStrength());
    glm::vec3 sunDir = m_world.getSunDirection();
    m_entityShader->setVec3("sunDirection", glm::mat3(m_camera.getViewMatrix()) * sunDir);

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

    float bobDist = m_player.prevDistanceWalkedModified + (m_player.distanceWalkedModified - m_player.prevDistanceWalkedModified) * partialTicks;
    float bobStr = m_player.prevCameraYaw + (m_player.cameraYaw - m_player.prevCameraYaw) * partialTicks;
    float bobPitch = m_player.prevCameraPitch + (m_player.cameraPitch - m_player.prevCameraPitch) * partialTicks;

    glm::mat4 baseBobMat = glm::mat4(1.0f);
    baseBobMat = glm::translate(baseBobMat, glm::vec3(std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f, -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr), 0.0f));
    baseBobMat = glm::rotate(baseBobMat, glm::radians(std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    baseBobMat = glm::rotate(baseBobMat, glm::radians(std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    baseBobMat = glm::rotate(baseBobMat, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));

    float eqProgress = m_prevEquippedProgress + (m_equippedProgress - m_prevEquippedProgress) * partialTicks;
    float swingProgress = m_player.isSwinging ? ((float)m_player.swingProgressInt + partialTicks) / 8.0f : 0.0f;

    if (m_itemToRenderID <= 0) {
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/char.png"));
        glm::mat4 armMat = baseBobMat;
        float var5 = 0.8f;
        if (swingProgress > 0.0f) {
            float f1 = std::sin(swingProgress * glm::pi<float>());
            float f2 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            armMat = glm::translate(armMat, glm::vec3(-f2 * 0.3f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.4f, -f1 * 0.4f));
        }

        armMat = glm::translate(armMat, glm::vec3(0.8f * var5, -0.75f * var5 - (1.0f - eqProgress) * 0.6f, -0.9f * var5));
        armMat = glm::rotate(armMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        if (swingProgress > 0.0f) {
            float f2 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            float f3 = std::sin(swingProgress * swingProgress * glm::pi<float>());
            armMat = glm::rotate(armMat, glm::radians(f2 * 70.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            armMat = glm::rotate(armMat, glm::radians(-f3 * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        armMat = glm::translate(armMat, glm::vec3(-1.0f, 3.6f, 3.5f));
        armMat = glm::rotate(armMat, glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        armMat = glm::rotate(armMat, glm::radians(200.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        armMat = glm::rotate(armMat, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        armMat = glm::translate(armMat, glm::vec3(5.6f, 0.0f, 0.0f));

        m_playerModel->renderFirstPersonArm(*m_entityShader, armMat, 0.0625f);
    } else {
        // 2. Render Held Item/Block
        glDisable(GL_CULL_FACE);
        glm::mat4 heldMat = baseBobMat;
        float var5 = 0.8f;
        if (swingProgress > 0.0f) {
            float var7 = std::sin(swingProgress * glm::pi<float>());
            float var8 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            heldMat = glm::translate(heldMat, glm::vec3(-var8 * 0.4f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.2f, -var7 * 0.2f));
        }

        heldMat = glm::translate(heldMat, glm::vec3(0.7f * var5, -0.65f * var5 - (1.0f - eqProgress) * 0.6f, -0.9f * var5));
        heldMat = glm::rotate(heldMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        if (swingProgress > 0.0f) {
            float var7 = std::sin(swingProgress * swingProgress * glm::pi<float>());
            float var8 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            heldMat = glm::rotate(heldMat, glm::radians(-var7 * 20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            heldMat = glm::rotate(heldMat, glm::radians(-var8 * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            heldMat = glm::rotate(heldMat, glm::radians(-var8 * 80.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }

        heldMat = glm::scale(heldMat, glm::vec3(0.4f, 0.4f, 0.4f));
        const bool isBlockItem = isInventoryBlockModel(m_itemToRenderID);
        if (isBlockItem) {
            m_renderEngine->bindTexture(m_renderEngine->getTexture("/terrain.png"));
            m_entityShader->setMat4("model", heldMat);
            m_entityShader->setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

            Tessellator* t = Tessellator::instance;
            Block* block = Block::blocksList[m_itemToRenderID];
            static constexpr float kFaceShade[6] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
            t->startDrawingQuads();
            for (int side = 0; side < 6; ++side) {
                addFace(t, side, getTextureUV(block->getTexture(side)), kFaceShade[side]);
            }
            t->draw();
        } else {
            heldMat = glm::translate(heldMat, glm::vec3(0.0f, -0.3f, 0.08f));
            heldMat = glm::scale(heldMat, glm::vec3(1.5f, 1.5f, 1.5f));
            heldMat = glm::rotate(heldMat, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            heldMat = glm::rotate(heldMat, glm::radians(335.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            heldMat = glm::translate(heldMat, glm::vec3(-(15.0f / 16.0f), -(1.0f / 16.0f), 0.0f));
            m_entityShader->setMat4("model", heldMat);
            m_entityShader->setMat3("normalMatrix", glm::mat3(glm::inverseTranspose(heldMat)));

            Tessellator* t = Tessellator::instance;
            const int tex = getItemIconTexture(m_itemToRenderID);
            const FaceUV uv = getTextureUV(tex);
            m_renderEngine->bindTexture(m_renderEngine->getTexture(m_itemToRenderID < 256 ? "/terrain.png" : "/gui/items.png"));
            t->startDrawingQuads();
            t->setColorOpaque(255, 255, 255);
            renderFlatHeldItem(t, uv);
            t->draw();
        }
        glEnable(GL_CULL_FACE);
    }
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

void GameRenderer::renderHUD() {
    glm::mat4 projection = glm::ortho(0.0f, m_scaledWidth, m_scaledHeight, 0.0f, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    m_uiShader->use();
    m_uiShader->setMat4("projection", projection);
    m_uiShader->setMat4("view", view);

    m_textShader->use();
    m_textShader->setMat4("projection", projection);
    m_textShader->setMat4("view", view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/gui.png"));
    float centerX = m_scaledWidth / 2.0f;
    drawTexturedModalRect(centerX - 91.0f, m_scaledHeight - 22.0f, 0, 0, 182, 22);
    drawTexturedModalRect(centerX - 91.0f - 1.0f + (float)m_player.inventory.currentSlot * 20.0f, m_scaledHeight - 22.0f - 1.0f, 0, 22, 24, 22);

    for (int slot = 0; slot < InventoryPlayer::HOTBAR_SIZE; ++slot) {
        const ItemStack& stack = m_player.inventory.mainInventory[slot];
        if (stack.itemID <= 0 || stack.count <= 0) continue;

        const float iconX = centerX - 91.0f + (float)slot * 20.0f + 3.0f;
        const float iconY = m_scaledHeight - 19.0f;
        Gui::drawItemStack(&m_player.getMinecraft(), stack, iconX, iconY);
    }

    if (m_player.gameMode == GameMode::SURVIVAL) {
        m_uiShader->use();
        m_uiShader->setMat4("projection", projection);
        m_uiShader->setMat4("view", view);
        m_uiShader->setBool("hasTexture", true);

        m_renderEngine->bindTexture(m_renderEngine->getTexture("/gui/icons.png"));
        for (int i = 0; i < 10; ++i) {
            float x = centerX - 91.0f + (float)i * 8.0f;
            float y = m_scaledHeight - 32.0f;
            drawTexturedModalRect(x, y, 16, 0, 9, 9);
            if (i * 2 + 1 < m_player.health) drawTexturedModalRect(x, y, 52, 0, 9, 9);
            else if (i * 2 + 1 == m_player.health) drawTexturedModalRect(x, y, 61, 0, 9, 9);
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
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
    drawTexturedModalRect(std::floor(m_scaledWidth / 2.0f) - 8.0f, std::floor(m_scaledHeight / 2.0f) - 8.0f, 0, 0, 16, 16);
    glDisable(GL_BLEND);
}


void GameRenderer::renderUnderwaterOverlay() {
    m_renderEngine->bindTexture(m_renderEngine->getTexture("/water.png"));

    Tessellator* t = Tessellator::instance;
    float b = m_world.getDaylightStrength();
    m_uiShader->use();
    m_uiShader->setBool("hasTexture", true);
    t->startDrawingQuads();
    t->setColorRGBA((int)(b * 255), (int)(b * 255), (int)(b * 255), 128);

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
