#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
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
    Block::init();
    m_renderEngine = std::make_unique<RenderEngine>();
    m_skyRenderer = std::make_unique<SkyRenderer>(*m_renderEngine);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_basicShader = std::make_unique<Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_basicShader->use();
    m_basicShader->setInt("texture1", 0);
    m_terrainTex = m_renderEngine->getTexture("/terrain.png");

    m_world = std::make_unique<World>();
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(-1));

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(8.0, 100.0, 8.0);
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
                frameDelta > 0.0 ? 1.0 / frameDelta : 0.0,
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
    
    handleInput();
    m_player->onUpdate();
}

void Minecraft::render(float partialTicks) {
    // Frame rate dependent rendering
    bool anyChunksAdded = m_world->pollGeneratedChunks();

    // Interpolate camera position
    double px = m_player->prevPosX + (m_player->posX - m_player->prevPosX) * (double)partialTicks;
    double py = m_player->prevPosY + (m_player->posY - m_player->prevPosY) * (double)partialTicks + (double)m_player->yOffset;
    double pz = m_player->prevPosZ + (m_player->posZ - m_player->prevPosZ) * (double)partialTicks;

    m_camera.position = glm::vec3(px, py, pz);
    
    // Use raw rotation for zero-latency mouse look
    m_camera.yaw = m_player->rotationYaw;
    m_camera.pitch = m_player->rotationPitch;
    m_camera.updateCameraVectors();

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
    glm::mat4 view = m_camera.getViewMatrix();

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
}

void Minecraft::handleInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        m_running = false;

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
