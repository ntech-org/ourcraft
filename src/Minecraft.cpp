#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>

Minecraft::Minecraft(GLFWwindow* window, int width, int height)
    : m_window(window), m_width(width), m_height(height), m_timer(20.0f)
{
    init();
}

Minecraft::~Minecraft() {}

void Minecraft::init() {
    Block::init();
    
    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(-1));

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(8.0, 80.0, 8.0);
    m_player->preparePlayerToSpawn();

    m_gameRenderer = std::make_unique<GameRenderer>(m_window, *m_world, *m_player);
    m_inputHandler = std::make_unique<InputHandler>(m_window, *m_player);
    m_networkHandler = std::make_unique<NetworkHandler>(*m_world, *m_player);

    if (!m_networkHandler->connect("127.0.0.1", 25565)) {
        throw std::runtime_error("Failed to connect to integrated server");
    }

    // Initial chunk load
    for (int dx = -16; dx <= 16; ++dx) {
        for (int dz = -16; dz <= 16; ++dz) {
            m_world->requestChunk(dx, dz);
        }
    }
    while (!m_world->m_pendingChunks.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_world->pollGeneratedChunks();
    }
    m_gameRenderer->getWorldRenderer().rebuildSectionList();
}

void Minecraft::run() {
    m_lastFrameTime = glfwGetTime();
    while (m_running && !glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        double frameDelta = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_fps = frameDelta > 0.0 ? (float)(1.0 / frameDelta) : 0.0f;

        m_timer.updateTimer();
        for (int i = 0; i < m_timer.elapsedTicks; ++i) tick();

        m_gameRenderer->render(m_timer.renderPartialTicks, m_inputHandler->getCameraMode(), m_inputHandler->isDebugVisible(), m_fps);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Minecraft::tick() {
    m_world->update(0.05f);
    m_networkHandler->update();
    m_inputHandler->update();
    m_player->onUpdate();
    m_gameRenderer->getRenderEngine().updateTextureFX();
    m_networkHandler->sendPlayerPosition(*m_player);
}

void Minecraft::resize(int width, int height) {
    m_width = width; m_height = height;
    m_gameRenderer->resize(width, height);
}

void Minecraft::mouseCallback(double xpos, double ypos) {
    m_inputHandler->handleMouse(xpos, ypos);
}
