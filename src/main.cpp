#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/Frustum.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/WorldRenderer.hpp"
#include "renderer/Camera.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"

Camera camera;
float lastX = 427, lastY = 240;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int windowWidth = 854;
int windowHeight = 480;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 10.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += cameraSpeed * camera.front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= cameraSpeed * camera.front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.position += cameraSpeed * camera.up;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.position -= cameraSpeed * camera.up;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(854, 480, "OurCraft - Infdev Port", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    Block::init();
    RenderEngine renderEngine;
    SkyRenderer skyRenderer(renderEngine);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Shader basicShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    basicShader.use();
    basicShader.setInt("texture1", 0);
    int terrainTex = renderEngine.getTexture("/terrain.png");

    World world;
    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz) {
            auto chunk = std::make_unique<Chunk>(cx, cz);
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    chunk->setBlockID(x, 0, z, 7); // Bedrock
                    for (int y = 1; y < 60; ++y) chunk->setBlockID(x, y, z, 1); // Stone
                    for (int y = 60; y < 64; ++y) chunk->setBlockID(x, y, z, 3); // Dirt
                    chunk->setBlockID(x, 64, z, 2); // Grass
                }
            }

            if (cx == 0 && cz == 0) {
                // Carve a chamber plus an access shaft so interior faces are visible in the test world.
                for (int x = 4; x <= 11; ++x) {
                    for (int z = 4; z <= 11; ++z) {
                        for (int y = 24; y <= 36; ++y) {
                            chunk->setBlockID(x, y, z, 0);
                        }
                    }
                }

                for (int x = 7; x <= 8; ++x) {
                    for (int z = 7; z <= 8; ++z) {
                        for (int y = 37; y <= 64; ++y) {
                            chunk->setBlockID(x, y, z, 0);
                        }
                    }
                }

                for (int x = 0; x <= 8; ++x) {
                    for (int z = 7; z <= 8; ++z) {
                        for (int y = 28; y <= 31; ++y) {
                            chunk->setBlockID(x, y, z, 0);
                        }
                    }
                }
            }

            world.addChunk(std::move(chunk));
        }
    }

    camera.position = glm::vec3(8.0f, 66.0f, -6.0f);
    camera.yaw = 90.0f;
    camera.pitch = 0.0f;
    camera.updateCameraVectors();

    WorldRenderer worldRenderer(world);
    Frustum frustum;
    double titleTimer = 0.0;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        world.update(deltaTime);

        processInput(window);

        // Always use current framebuffer size
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);

        // Update fog brightness multiplier based on world brightness at camera position
        static float fogMultiplier = 1.0f;
        float currentBrightness = world.getBrightness(
            static_cast<int>(std::floor(camera.position.x)),
            static_cast<int>(std::floor(camera.position.y)),
            static_cast<int>(std::floor(camera.position.z))
        );
        fogMultiplier += (currentBrightness - fogMultiplier) * 0.1f;

        // Void darkening: if below Y=32, darken the lower sky/fog even more
        float voidDarkening = 1.0f;
        if (camera.position.y < 32.0f) {
            voidDarkening = std::clamp(camera.position.y / 32.0f, 0.0f, 1.0f);
            // Squaring it makes the darkening more dramatic as you go down
            voidDarkening *= voidDarkening;
        }

        glm::vec3 fogColor = world.getFogColor();
        const glm::vec3 skyColor = world.getSkyColor();
        // Blend fog color with sky color based on render distance (assuming Far = 0)
        // factor = 1.0 - pow(0.25, 0.25) = 0.29289321881
        const float blendFactor = 0.29289321881f;
        fogColor += (skyColor - fogColor) * blendFactor;
        
        // Apply brightness multiplier and void darkening to fog
        fogColor *= (fogMultiplier * voidDarkening);

        glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = windowHeight > 0 ? static_cast<float>(windowWidth) / static_cast<float>(windowHeight) : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.05f, 1000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        skyRenderer.render(world, camera, projection, view, fogMultiplier);

        basicShader.use();

        basicShader.setMat4("projection", projection);
        basicShader.setMat4("view", view);
        basicShader.setMat4("model", model);
        basicShader.setBool("hasTexture", true);

        // In Infdev, world fog start is farPlaneDistance * 0.25, end is farPlaneDistance
        // For Far (256.0), that is 64.0 and 256.0.
        // main.cpp used 96.0 and 224.0, which is also fine but let's stick to Infdev or keep consistent.
        // Let's use 64.0 and 256.0 to be "exact" if possible.
        basicShader.setFloat("fogNear", 64.0f);
        basicShader.setFloat("fogFar", 256.0f);
        basicShader.setVec3("fogColor", fogColor);
        basicShader.setVec3("cameraPos", camera.position);
        basicShader.setFloat("daylightFactor", world.getDaylightStrength());
        basicShader.setVec3("sunDirection", world.getSunDirection());

        renderEngine.bindTexture(terrainTex);
        frustum.update(projection * view);
        worldRenderer.updateDirtyMeshes();
        worldRenderer.render(frustum, basicShader);

        titleTimer += deltaTime;
        if (titleTimer >= 0.25) {
            titleTimer = 0.0;
            const WorldRenderer::Stats& stats = worldRenderer.getStats();
            char title[256];
            std::snprintf(
                title,
                sizeof(title),
                "OurCraft - FPS %.1f | Visible %zu/%zu | Draws %zu | Tris %zu | Rebuilds %zu (%.2f ms)",
                deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f,
                stats.visibleSections,
                stats.sectionCount,
                stats.drawCalls,
                stats.triangles,
                stats.meshBuilds,
                stats.meshBuildMs
            );
            glfwSetWindowTitle(window, title);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
