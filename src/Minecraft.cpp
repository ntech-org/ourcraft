#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "renderer/Tessellator.hpp"
#include <chrono>
#include <thread>
#include <stdexcept>

Minecraft::Minecraft(GLFWwindow* window, int width, int height)
    : m_window(window), m_width(width), m_height(height), m_timer(20.0f)
{
    Tessellator::init();
    init();
}

Minecraft::~Minecraft() {}

void Minecraft::init() {
    Block::init();

    m_world = std::make_unique<World>();
    m_world->isRemote = true;
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(1772835215));

    m_player = std::make_unique<EntityPlayer>(*m_world);
    m_player->setPosition(0.0, 128.0, 0.0);
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
        double updateStart = glfwGetTime();
        for (int i = 0; i < m_timer.elapsedTicks; ++i) tick();
        m_gameRenderer->getProfiler().updateTime = (glfwGetTime() - updateStart) * 1000.0;

        m_gameRenderer->render(m_timer.renderPartialTicks,
                               m_inputHandler->getCameraMode(),
                               m_inputHandler->isDebugVisible(),
                               m_inputHandler->isChunkBoundariesVisible(),
                               m_inputHandler->isProfilerVisible(),
                               m_fps);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Minecraft::tick() {
    m_world->update(0.05f);
    m_networkHandler->update();
    m_inputHandler->update();

    // Raycast for block picking
    float reach = 5.0f;
    glm::vec3 eyePos = glm::vec3(m_player->posX, m_player->posY + 1.62f, m_player->posZ);
    float yaw = glm::radians(m_player->rotationYaw);
    float pitch = glm::radians(m_player->rotationPitch);
    glm::vec3 lookDir = glm::vec3(
        -std::sin(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::cos(yaw) * std::cos(pitch)
    );

    glm::vec3 endPos = eyePos + lookDir * reach;
    HitResult hit = m_world->rayTraceBlocks(eyePos, endPos);

    if (m_inputHandler->isLeftClick()) {
        m_player->swing();
        if (hit.type == HitType::BLOCK) {
            m_world->setBlockWithNotify(hit.x, hit.y, hit.z, 0);
        }
    }


    if (m_inputHandler->isRightClick()) {
        if (hit.type == HitType::BLOCK) {
            int x = hit.x, y = hit.y, z = hit.z;
            if (hit.sideHit == 0) y--; else if (hit.sideHit == 1) y++;
            else if (hit.sideHit == 2) z--; else if (hit.sideHit == 3) z++;
            else if (hit.sideHit == 4) x--; else if (hit.sideHit == 5) x++;

            AxisAlignedBB blockBB((double)x, (double)y, (double)z, (double)x + 1.0, (double)y + 1.0, (double)z + 1.0);
            if (!m_player->boundingBox.intersectsWith(blockBB)) {
                int itemID = m_player->inventory.getCurrentItemID();
                if (itemID > 0) {
                    m_world->setBlockWithNotify(x, y, z, (uint8_t)itemID);
                    m_player->swing();
                }
            }
        }
    }



    if (m_inputHandler->shouldReloadChunks()) {
        m_gameRenderer->getWorldRenderer().rebuildSectionList();
    }

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

void Minecraft::scrollCallback(double xoffset, double yoffset) {
    m_inputHandler->handleScroll(xoffset, yoffset);
}
