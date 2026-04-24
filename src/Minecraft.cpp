#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityZombie.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>

Minecraft::Minecraft(GLFWwindow* window, int width, int height)
    : m_window(window), m_width(width), m_height(height), m_timer(20.0f), m_lastX(width / 2.0f), m_lastY(height / 2.0f)
{
    init();
}

Minecraft::~Minecraft() {
}

void Minecraft::init() {
    NetworkManager::init();
    Block::init();
    m_renderEngine = std::make_unique<RenderEngine>();
    m_skyRenderer = std::make_unique<SkyRenderer>(*m_renderEngine);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_basicShader = std::make_unique<Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_basicShader->use();
    m_basicShader->setInt("texture1", 0);
    m_terrainTex = m_renderEngine->getTexture("/terrain.png");

    m_entityShader = std::make_unique<Shader>("assets/shaders/entity.vert", "assets/shaders/entity.frag");
    m_playerModel = std::make_unique<ModelBiped>();
    m_zombieModel = std::make_unique<ModelZombie>();
    m_fontRenderer = std::make_unique<FontRenderer>(m_renderEngine.get(), "/default.png");

    // Start Integrated Server
    m_server = std::make_unique<IntegratedServer>();
    m_server->start();

    // Connect Client
    m_client = std::make_unique<Client>();
    m_client->onPacketReceived = [this](const uint8_t* data, size_t size) {
        this->onPacketReceived(data, size);
    };

    if (!m_client->connect("127.0.0.1", 25565)) {
        throw std::runtime_error("Failed to connect to integrated server");
    }

    PacketLogin loginPacket;
    loginPacket.username = "Player";
    loginPacket.protocolVersion = 1;
    m_client->sendPacket(loginPacket);

    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(-1));

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(8.0, 1000.0, 8.0);
    m_player->preparePlayerToSpawn();

    m_worldRenderer = std::make_unique<WorldRenderer>(*m_world);

    // Initial load
    int renderDistance = 16;
    int playerCX = (int)std::floor(m_player->posX / 16.0);
    int playerCZ = (int)std::floor(m_player->posZ / 16.0);

    for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
        for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
            m_world->requestChunk(playerCX + dx, playerCZ + dz);
        }
    }

    while (!m_world->m_pendingChunks.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_world->pollGeneratedChunks();
    }
    m_worldRenderer->rebuildSectionList();
}

void Minecraft::run() {
    m_lastFrameTime = glfwGetTime();
    while (m_running && !glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        double frameDelta = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_fps = frameDelta > 0.0 ? (float)(1.0 / frameDelta) : 0.0f;

        m_timer.updateTimer();

        for (int i = 0; i < m_timer.elapsedTicks; ++i) {
            tick();
        }

        render(m_timer.renderPartialTicks);

        m_titleTimer += frameDelta;
        if (m_titleTimer >= 0.25) {
            m_titleTimer = 0.0;
            const WorldRenderer::Stats& stats = m_worldRenderer->getStats();
            char title[256];
            std::snprintf(
                title,
                sizeof(title),
                "OurCraft - FPS %.1f | Visible %zu/%zu | Rebuilds %zu (%.2f ms) | Pos: %.1f, %.1f, %.1f",
                (double)m_fps,
                stats.visibleSections,
                stats.sectionCount,
                stats.meshBuilds,
                stats.meshBuildMs,
                m_player->posX, m_player->posY, m_player->posZ
            );
            glfwSetWindowTitle(m_window, title);
        }

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Minecraft::tick() {
    // Fixed rate logic (20 TPS)
    m_world->update(0.05f); // 1/20th of a second

    m_client->poll();

    handleInput();
    m_player->onUpdate();

    PacketPlayerPosition posPacket;
    posPacket.x = m_player->posX;
    posPacket.y = m_player->posY;
    posPacket.z = m_player->posZ;
    posPacket.yaw = m_player->rotationYaw;
    posPacket.pitch = m_player->rotationPitch;
    posPacket.onGround = m_player->onGround;
    m_client->sendPacket(posPacket, false);
}

void Minecraft::render(float partialTicks) {
    // Frame rate dependent rendering
    bool anyChunksAdded = m_world->pollGeneratedChunks();

    // Interpolate camera position
    double px = m_player->prevPosX + (m_player->posX - m_player->prevPosX) * (double)partialTicks;
    double py = m_player->prevPosY + (m_player->posY - m_player->prevPosY) * (double)partialTicks + (double)m_player->yOffset;
    double pz = m_player->prevPosZ + (m_player->posZ - m_player->prevPosZ) * (double)partialTicks;

    // Local camera is NOT interpolated
    float camYaw = m_player->rotationYaw;
    float camPitch = m_player->rotationPitch;

    // View Bobbing (Camera)
    float bobX = 0.0f;
    float bobY = 0.0f;
    float bobRotZ = 0.0f;
    float bobRotX = 0.0f;
    float bobPitch = 0.0f;

    if (m_cameraMode == 0) {
        float bobDist = m_player->prevDistanceWalkedModified + (m_player->distanceWalkedModified - m_player->prevDistanceWalkedModified) * partialTicks;
        float bobStr = m_player->prevCameraYaw + (m_player->cameraYaw - m_player->prevCameraYaw) * partialTicks;
        bobPitch = m_player->prevCameraPitch + (m_player->cameraPitch - m_player->prevCameraPitch) * partialTicks;

        bobX = std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f;
        bobY = -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr);
        bobRotZ = std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f;
        bobRotX = std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f;
    }

    // Update camera based on mode
    if (m_cameraMode == 0) {
        // First person: camera at player's head
        m_camera.yaw = camYaw + 90.0f;
        m_camera.pitch = camPitch;
        m_camera.position = glm::vec3(px, py, pz);
        m_camera.updateCameraVectors();
    } else if (m_cameraMode == 1) {
        // Third person back: camera behind player
        m_camera.yaw = camYaw + 90.0f;
        m_camera.pitch = camPitch;
        m_camera.updateCameraVectors();
        m_camera.position = glm::vec3(px, py, pz) - m_camera.front * 4.0f;
    } else if (m_cameraMode == 2) {
        // Third person front: camera in front of player
        m_camera.yaw = camYaw + 90.0f + 180.0f;
        m_camera.pitch = -camPitch;
        m_camera.updateCameraVectors();
        m_camera.position = glm::vec3(px, py, pz) - m_camera.front * 4.0f;
    }

    glm::mat4 view = glm::mat4(1.0f);
    if (m_cameraMode == 0) {
        view = glm::translate(view, glm::vec3(bobX, bobY, 0.0f));
        view = glm::rotate(view, glm::radians(bobRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
        view = glm::rotate(view, glm::radians(bobRotX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    view = view * m_camera.getViewMatrix();

    int playerCX = (int)std::floor(px / 16.0);
    int playerCZ = (int)std::floor(pz / 16.0);

    int renderDistance = 8;
    int keepDistance = renderDistance + 2;
    bool chunksRequested = false;

    // Unload chunks far away
    m_worldRenderer->removeFarSections(playerCX, playerCZ, keepDistance);
    m_world->unloadFarChunks(playerCX, playerCZ, keepDistance);

    std::vector<std::pair<int, int>> chunksToLoad;
    for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
        for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
            int cx = playerCX + dx;
            int cz = playerCZ + dz;
            if (!m_world->isChunkLoaded(cx, cz) && !m_world->isChunkPending(cx, cz)) {
                chunksToLoad.push_back({cx, cz});
            }
        }
    }

    if (!chunksToLoad.empty()) {
        std::sort(chunksToLoad.begin(), chunksToLoad.end(), [playerCX, playerCZ](const auto& a, const auto& b) {
            int distA = (a.first - playerCX) * (a.first - playerCX) + (a.second - playerCZ) * (a.second - playerCZ);
            int distB = (b.first - playerCX) * (b.first - playerCX) + (b.second - playerCZ) * (b.second - playerCZ);
            return distA < distB;
        });

        for (const auto& coords : chunksToLoad) {
            m_world->requestChunk(coords.first, coords.second);
            chunksRequested = true;
        }
    }

    if (anyChunksAdded || chunksRequested) {
        m_worldRenderer->rebuildSectionList();
    }

    float currentBrightness = m_world->getBrightness(
        static_cast<int>(std::floor(px)),
        static_cast<int>(std::floor(py)),
        static_cast<int>(std::floor(pz))
    );

    // Void darkening: only depends on height, not local blocks
    float voidDarkening = 1.0f;
    float horizon = m_world->getHorizon();
    if (py < (double)horizon) {
        voidDarkening = (float)(py / (double)horizon);
        if (voidDarkening < 0.0f) voidDarkening = 0.0f;
        voidDarkening *= voidDarkening;
    }

    glm::vec3 fogColor = m_world->getFogColor();
    const glm::vec3 skyColor = m_world->getSkyColor();
    fogColor += (skyColor - fogColor) * 0.29289321881f;

    // Apply void darkening to both fog and sky
    glm::vec3 finalFogColor = fogColor * voidDarkening;

    glClearColor(finalFogColor.r, finalFogColor.g, finalFogColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspect = m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.05f, 1000.0f);

    m_skyRenderer->render(*m_world, m_camera, projection, view, voidDarkening);

    m_basicShader->use();
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("model", glm::mat4(1.0f));
    m_basicShader->setBool("hasTexture", true);
    m_basicShader->setFloat("fogNear", 64.0f);
    m_basicShader->setFloat("fogFar", 256.0f);
    m_basicShader->setVec3("fogColor", finalFogColor);

    m_basicShader->setVec3("cameraPos", m_camera.position);
    m_basicShader->setFloat("daylightFactor", m_world->getDaylightStrength());
    m_basicShader->setVec3("sunDirection", m_world->getSunDirection());

    m_renderEngine->bindTexture(m_terrainTex);
    m_frustum.update(projection * view);
    m_worldRenderer->updateDirtyMeshes(50);
    m_worldRenderer->render(m_frustum, *m_basicShader);

    // Render entities
    glDisable(GL_CULL_FACE); // Fix inside-out rendering causing upside-down illusion
    m_entityShader->use();
    m_entityShader->setMat4("projection", projection);
    m_entityShader->setMat4("view", view);
    m_entityShader->setInt("texture1", 0);

    for (const auto& entity : m_world->getEntities()) {
        float brightness = m_world->getBrightness(
            (int)std::floor(entity->posX),
            (int)std::floor(entity->posY),
            (int)std::floor(entity->posZ)
        );
        m_entityShader->setVec3("colorTint", glm::vec3(brightness));

        m_renderEngine->bindTexture(m_renderEngine->getTexture("/mob/zombie.png"));

        double ex = entity->prevPosX + (entity->posX - entity->prevPosX) * (double)partialTicks;
        double ey = entity->prevPosY + (entity->posY - entity->prevPosY) * (double)partialTicks;
        double ez = entity->prevPosZ + (entity->posZ - entity->prevPosZ) * (double)partialTicks;

        float renderYaw = 0.0f;
        float netHeadYaw = 0.0f;
        float headPitch = entity->prevRotationPitch + (entity->rotationPitch - entity->prevRotationPitch) * partialTicks;

        if (auto living = dynamic_cast<EntityLiving*>(entity.get())) {
            renderYaw = living->prevRenderYawOffset + (living->renderYawOffset - living->prevRenderYawOffset) * partialTicks;
            float interpYaw = living->prevRotationYaw + (living->rotationYaw - living->prevRotationYaw) * partialTicks;
            netHeadYaw = (interpYaw - renderYaw);

            // Clamp netHeadYaw and normalize
            while (netHeadYaw < -180.0f) netHeadYaw += 360.0f;
            while (netHeadYaw >= 180.0f) netHeadYaw -= 360.0f;

            if (netHeadYaw < -75.0f) netHeadYaw = -75.0f;
            if (netHeadYaw >= 75.0f) netHeadYaw = 75.0f;
        }

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(ex, ey, ez));
        modelMat = glm::rotate(modelMat, glm::radians(180.0f - renderYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(-1.0f, -1.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -24.0f * 0.0625f, 0.0f));

        float swing = 0.0f;
        float limbSwing = 0.0f;
        float limbSwingAmount = 0.0f;
        if (auto living = dynamic_cast<EntityLiving*>(entity.get())) {
            limbSwing = living->prevLimbSwing + (living->limbSwing - living->prevLimbSwing) * partialTicks;
            limbSwingAmount = living->prevLimbSwingAmount + (living->limbSwingAmount - living->prevLimbSwingAmount) * partialTicks;
            if (living->isSwinging) {
                swing = ((float)living->swingProgressInt + partialTicks) / 8.0f;
            }
        }

        m_zombieModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)glfwGetTime(), netHeadYaw, -headPitch, 0.0625f, swing);
    }

    // Render local player in 3rd person
    if (m_cameraMode != 0) {
        float brightness = m_world->getBrightness(
            (int)std::floor(m_player->posX),
            (int)std::floor(m_player->posY),
            (int)std::floor(m_player->posZ)
        );
        m_entityShader->setVec3("colorTint", glm::vec3(brightness));
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/char.png"));

        double ex = m_player->prevPosX + (m_player->posX - m_player->prevPosX) * (double)partialTicks;
        double ey = m_player->prevPosY + (m_player->posY - m_player->prevPosY) * (double)partialTicks;
        double ez = m_player->prevPosZ + (m_player->posZ - m_player->prevPosZ) * (double)partialTicks;

        float renderYaw = m_player->prevRenderYawOffset + (m_player->renderYawOffset - m_player->prevRenderYawOffset) * partialTicks;
        float interpYaw = m_player->prevRotationYaw + (m_player->rotationYaw - m_player->prevRotationYaw) * partialTicks;
        float headPitch = m_player->prevRotationPitch + (m_player->rotationPitch - m_player->prevRotationPitch) * partialTicks;
        float netHeadYaw = (interpYaw - renderYaw);

        while (netHeadYaw < -180.0f) netHeadYaw += 360.0f;
        while (netHeadYaw >= 180.0f) netHeadYaw -= 360.0f;

        if (netHeadYaw < -75.0f) netHeadYaw = -75.0f;
        if (netHeadYaw >= 75.0f) netHeadYaw = 75.0f;

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(ex, ey, ez));
        modelMat = glm::rotate(modelMat, glm::radians(180.0f - renderYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(-1.0f, -1.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -24.0f * 0.0625f, 0.0f));

        float limbSwing = m_player->prevLimbSwing + (m_player->limbSwing - m_player->prevLimbSwing) * partialTicks;
        float limbSwingAmount = m_player->prevLimbSwingAmount + (m_player->limbSwingAmount - m_player->prevLimbSwingAmount) * partialTicks;
        float swing = m_player->isSwinging ? ((float)m_player->swingProgressInt + partialTicks) / 8.0f : 0.0f;
        m_playerModel->render(*m_entityShader, modelMat, limbSwing, limbSwingAmount, (float)glfwGetTime(), netHeadYaw, -headPitch, 0.0625f, swing);
    }

    // Render first-person arm
    if (m_cameraMode == 0) {
        glClear(GL_DEPTH_BUFFER_BIT);
        m_entityShader->use();
        m_entityShader->setMat4("projection", projection);
        m_entityShader->setMat4("view", glm::mat4(1.0f));

        float handBrightness = m_world->getBrightness(
            (int)std::floor(m_player->posX),
            (int)std::floor(m_player->posY),
            (int)std::floor(m_player->posZ)
        );
        m_entityShader->setVec3("colorTint", glm::vec3(handBrightness));
        m_renderEngine->bindTexture(m_renderEngine->getTexture("/char.png"));

        glm::mat4 armBase = glm::mat4(1.0f);

        // Apply swing transforms
        float swingProgress = m_player->isSwinging ? ((float)m_player->swingProgressInt + partialTicks) / 8.0f : 0.0f;
        if (swingProgress > 0.0f) {
            float f = std::sin(swingProgress * glm::pi<float>());
            float f1 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            armBase = glm::translate(armBase, glm::vec3(-f1 * 0.3f, std::sin(std::sqrt(swingProgress) * glm::pi<float>() * 2.0f) * 0.4f, -f * 0.4f));
        }

        // Apply view bobbing translation
        float bobDist = m_player->prevDistanceWalkedModified + (m_player->distanceWalkedModified - m_player->prevDistanceWalkedModified) * partialTicks;
        float bobStr = m_player->prevCameraYaw + (m_player->cameraYaw - m_player->prevCameraYaw) * partialTicks;
        float bobPitch = m_player->prevCameraPitch + (m_player->cameraPitch - m_player->prevCameraPitch) * partialTicks;

        float bobX = std::sin(bobDist * glm::pi<float>()) * bobStr * 0.5f;
        float bobY = -std::abs(std::cos(bobDist * glm::pi<float>()) * bobStr);
        float bobRotZ = std::sin(bobDist * glm::pi<float>()) * bobStr * 3.0f;
        float bobRotX = std::abs(std::cos(bobDist * glm::pi<float>() + 0.2f) * bobStr) * 5.0f;

        armBase = glm::translate(armBase, glm::vec3(bobX, bobY, 0.0f));
        armBase = glm::rotate(armBase, glm::radians(bobRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
        armBase = glm::rotate(armBase, glm::radians(bobRotX), glm::vec3(1.0f, 0.0f, 0.0f));
        armBase = glm::rotate(armBase, glm::radians(bobPitch), glm::vec3(1.0f, 0.0f, 0.0f));
        armBase = glm::translate(armBase, glm::vec3(0.0f, -bobPitch * 0.015f, 0.0f));

        // Java Infdev transforms for hand
        armBase = glm::translate(armBase, glm::vec3(0.64f, -0.6f, -0.72f));
        armBase = glm::rotate(armBase, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        if (swingProgress > 0.0f) {
            float f = std::sin(swingProgress * swingProgress * glm::pi<float>());
            float f1 = std::sin(std::sqrt(swingProgress) * glm::pi<float>());
            armBase = glm::rotate(armBase, glm::radians(f1 * 70.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            armBase = glm::rotate(armBase, glm::radians(-f * 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        armBase = glm::translate(armBase, glm::vec3(-1.0f, 3.6f, 3.5f));
        armBase = glm::rotate(armBase, glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        armBase = glm::rotate(armBase, glm::radians(200.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        armBase = glm::rotate(armBase, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        armBase = glm::scale(armBase, glm::vec3(1.0f, 1.0f, 1.0f));
        armBase = glm::translate(armBase, glm::vec3(5.6f, 0.0f, 0.0f));

        m_playerModel->renderFirstPersonArm(*m_entityShader, armBase, 0.0625f);
    }

    glEnable(GL_CULL_FACE); // Re-enable culling

    // Render Debug UI
    if (m_showDebug) {
        glDisable(GL_DEPTH_TEST);
        m_entityShader->use();
        glm::mat4 ortho = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);
        m_entityShader->setMat4("projection", ortho);
        m_entityShader->setMat4("view", glm::mat4(1.0f));
        m_entityShader->setVec3("colorTint", glm::vec3(1.0f));

        char buf[512];
        const WorldRenderer::Stats& stats = m_worldRenderer->getStats();
        std::snprintf(buf, sizeof(buf),
            "OurCraft Infdev\n"
            "FPS: %.0f\n"
            "Pos: %.3f, %.3f, %.3f\n"
            "Chunk: %d, %d, %d in %d, %d\n"
            "Visible: %zu/%zu\n"
            "Camera: %s",
            (double)m_fps,
            m_player->posX, m_player->posY, m_player->posZ,
            (int)std::floor(m_player->posX) % 16, (int)std::floor(m_player->posY) % 16, (int)std::floor(m_player->posZ) % 16,
            (int)std::floor(m_player->posX / 16.0), (int)std::floor(m_player->posZ / 16.0),
            stats.visibleSections, stats.sectionCount,
            m_cameraMode == 0 ? "First Person" : (m_cameraMode == 1 ? "Third Person Back" : "Third Person Front")
        );
        m_fontRenderer->drawString(*m_entityShader, buf, 2.0f, 2.0f, 0xFFFFFFFF);
        glEnable(GL_DEPTH_TEST);
    }
}

void Minecraft::handleInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        m_running = false;

    // F3 Toggle
    bool f3 = glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS;
    if (f3 && !m_f3Pressed) {
        m_showDebug = !m_showDebug;
    }
    m_f3Pressed = f3;

    // F5 Toggle
    bool f5 = glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_PRESS;
    if (f5 && !m_f5Pressed) {
        m_cameraMode = (m_cameraMode + 1) % 3;
    }
    m_f5Pressed = f5;

    // Swing logic
    bool leftMouse = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftMouse && !m_leftMousePressed) {
        m_player->swing();
    }
    m_leftMousePressed = leftMouse;

    m_player->moveForward = 0.0f;
    m_player->moveStrafe = 0.0f;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_player->moveForward += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_player->moveForward -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_player->moveStrafe += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_player->moveStrafe -= 1.0f;
    m_player->jumping = (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS);
}

void Minecraft::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

void Minecraft::mouseCallback(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastX;
    float yoffset = m_lastY - ypos;

    m_lastX = xpos;
    m_lastY = ypos;

    float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_player->rotationYaw += xoffset;
    m_player->rotationPitch += yoffset;

    if (m_player->rotationPitch > 89.9f) m_player->rotationPitch = 89.9f;
    if (m_player->rotationPitch < -89.9f) m_player->rotationPitch = -89.9f;
}

void Minecraft::onPacketReceived(const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);

    if (type == PacketType::LoginResponse) {
        PacketLoginResponse packet;
        packet.deserialize(ptr, size - 1);
        m_playerID = packet.entityID;
        m_player->entityID = m_playerID;
    } else if (type == PacketType::SpawnEntity) {
        PacketSpawnEntity packet;
        packet.deserialize(ptr, size - 1);

        if (packet.id == m_playerID) return;

        std::unique_ptr<Entity> entity;
        if (packet.type == 1) {
            entity = std::make_unique<EntityZombie>(*m_world);
        } else {
            entity = std::make_unique<EntityPlayer>(*m_world);
        }
        entity->entityID = packet.id;
        entity->setPosition(packet.x, packet.y, packet.z);
        entity->rotationYaw = packet.yaw;
        entity->rotationPitch = packet.pitch;
        entity->handlePhysics = false;
        m_world->spawnEntity(std::move(entity));
    } else if (type == PacketType::MoveEntity) {
        PacketMoveEntity packet;
        packet.deserialize(ptr, size - 1);

        if (packet.id == m_playerID) return;

        for (auto& entity : m_world->getEntities()) {
            if (entity->entityID == packet.id) {
                // Update target positions without resetting prevPosX/Y/Z
                entity->posX = packet.x;
                entity->posY = packet.y;
                entity->posZ = packet.z;
                entity->rotationYaw = packet.yaw;
                entity->rotationPitch = packet.pitch;

                // Manually update bounding box since we're not using setPosition
                float w2 = entity->width / 2.0f;
                entity->boundingBox = AxisAlignedBB(
                    entity->posX - w2, entity->posY, entity->posZ - w2,
                    entity->posX + w2, entity->posY + entity->height, entity->posZ + w2
                );
                break;
            }
        }
    }
}
