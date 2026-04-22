#include "Application.hpp"
#include "world/Block.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Application::Application() {
    init();
}

Application::~Application() {
    cleanup();
}

void Application::init() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "OurCraft - Infdev Port", NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

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

    m_camera.position = glm::vec3(8.0f, 80.0f, 8.0f);
    m_camera.yaw = 90.0f;
    m_camera.pitch = 0.0f;
    m_camera.updateCameraVectors();

    m_worldRenderer = std::make_unique<WorldRenderer>(*m_world);

    // Initial load
    int renderDistance = 8;
    int playerCX = (int)std::floor(m_camera.position.x / 16.0f);
    int playerCZ = (int)std::floor(m_camera.position.z / 16.0f);

    std::vector<std::pair<int, int>> initialChunks;
    for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
        for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
            initialChunks.push_back({playerCX + dx, playerCZ + dz});
        }
    }

    std::sort(initialChunks.begin(), initialChunks.end(), [playerCX, playerCZ](const auto& a, const auto& b) {
        int distA = (a.first - playerCX) * (a.first - playerCX) + (a.second - playerCZ) * (a.second - playerCZ);
        int distB = (b.first - playerCX) * (b.first - playerCX) + (b.second - playerCZ) * (b.second - playerCZ);
        return distA < distB;
    });

    for (const auto& coords : initialChunks) {
        m_world->requestChunk(coords.first, coords.second);
    }

    // Spin until loaded to ensure initial visibility

    while (!m_world->m_pendingChunks.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_world->pollGeneratedChunks();
    }
    m_worldRenderer->rebuildSectionList();

}

void Application::cleanup() {
    m_worldRenderer.reset();
    m_skyRenderer.reset();
    m_basicShader.reset();
    m_world.reset();
    m_renderEngine.reset();
    glfwTerminate();
}

void Application::run() {
    mainLoop();
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        update(m_deltaTime);
        handleInput();

        glfwGetFramebufferSize(m_window, &m_width, &m_height);
        glViewport(0, 0, m_width, m_height);

        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Application::update(float deltaTime) {
    if (deltaTime > 0.0f) {
        m_world->update(deltaTime);
    }
    bool anyChunksAdded = m_world->pollGeneratedChunks();

    int playerCX = (int)std::floor(m_camera.position.x / 16.0f);
    int playerCZ = (int)std::floor(m_camera.position.z / 16.0f);

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
}

void Application::handleInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    float cameraSpeed = 100.0f * m_deltaTime;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.position += cameraSpeed * m_camera.front;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.position -= cameraSpeed * m_camera.front;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.position -= glm::normalize(glm::cross(m_camera.front, m_camera.up)) * cameraSpeed;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.position += glm::normalize(glm::cross(m_camera.front, m_camera.up)) * cameraSpeed;
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_camera.position += cameraSpeed * m_camera.up;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        m_camera.position -= cameraSpeed * m_camera.up;
}

void Application::render() {
    static float fogMultiplier = 1.0f;
    float currentBrightness = m_world->getBrightness(
        static_cast<int>(std::floor(m_camera.position.x)),
        static_cast<int>(std::floor(m_camera.position.y)),
        static_cast<int>(std::floor(m_camera.position.z))
    );
    fogMultiplier += (currentBrightness - fogMultiplier) * 0.1f;

    float voidDarkening = 1.0f;
    if (m_camera.position.y < 32.0f) {
        voidDarkening = std::clamp(m_camera.position.y / 32.0f, 0.0f, 1.0f);
        voidDarkening *= voidDarkening;
    }

    glm::vec3 fogColor = m_world->getFogColor();
    const glm::vec3 skyColor = m_world->getSkyColor();
    const float blendFactor = 0.29289321881f;
    fogColor += (skyColor - fogColor) * blendFactor;
    fogColor *= (fogMultiplier * voidDarkening);

    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspect = m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.05f, 1000.0f);
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    m_skyRenderer->render(*m_world, m_camera, projection, view, fogMultiplier);

    m_basicShader->use();
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("model", model);
    m_basicShader->setBool("hasTexture", true);
    m_basicShader->setFloat("fogNear", 64.0f);
    m_basicShader->setFloat("fogFar", 256.0f);
    m_basicShader->setVec3("fogColor", fogColor);
    m_basicShader->setVec3("cameraPos", m_camera.position);
    m_basicShader->setFloat("daylightFactor", m_world->getDaylightStrength());
    m_basicShader->setVec3("sunDirection", m_world->getSunDirection());

    m_renderEngine->bindTexture(m_terrainTex);
    m_frustum.update(projection * view);
    m_worldRenderer->updateDirtyMeshes(50);
    m_worldRenderer->render(m_frustum, *m_basicShader);

    m_titleTimer += m_deltaTime;
    if (m_titleTimer >= 0.25) {
        m_titleTimer = 0.0;
        const WorldRenderer::Stats& stats = m_worldRenderer->getStats();
        char title[256];
        std::snprintf(
            title,
            sizeof(title),
            "OurCraft - FPS %.1f | Visible %zu/%zu | Draws %zu | Tris %zu | Rebuilds %zu (%.2f ms)",
            m_deltaTime > 0.0f ? 1.0f / m_deltaTime : 0.0f,
            stats.visibleSections,
            stats.sectionCount,
            stats.drawCalls,
            stats.triangles,
            stats.meshBuilds,
            stats.meshBuildMs
        );
        glfwSetWindowTitle(m_window, title);
    }
}

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_width = width;
    app->m_height = height;
    glViewport(0, 0, width, height);
}

void Application::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->m_firstMouse) {
        app->m_lastX = xpos;
        app->m_lastY = ypos;
        app->m_firstMouse = false;
    }

    float xoffset = xpos - app->m_lastX;
    float yoffset = app->m_lastY - ypos;

    app->m_lastX = xpos;
    app->m_lastY = ypos;

    app->m_camera.processMouseMovement(xoffset, yoffset);
}
