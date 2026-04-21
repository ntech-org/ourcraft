#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/Shader.hpp"
#include "renderer/Tessellator.hpp"
#include "renderer/RenderEngine.hpp"
#include "renderer/BlockRenderer.hpp"
#include "renderer/ChunkRenderer.hpp"
#include "renderer/Camera.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"

Camera camera;
float lastX = 427, lastY = 240;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
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

    Block::init();
    Tessellator::init();
    RenderEngine renderEngine;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Shader basicShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    int terrainTex = renderEngine.getTexture("/terrain.png");

    // Create a few chunks to test
    std::vector<Chunk*> chunks;
    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz) {
            Chunk* chunk = new Chunk(cx, cz);
            // Fill chunk with some blocks
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    chunk->setBlockID(x, 0, z, 7); // Bedrock
                    for (int y = 1; y < 60; ++y) chunk->setBlockID(x, y, z, 1); // Stone
                    for (int y = 60; y < 64; ++y) chunk->setBlockID(x, y, z, 3); // Dirt
                    chunk->setBlockID(x, 64, z, 2); // Grass
                }
            }
            chunks.push_back(chunk);
        }
    }

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        basicShader.use();
        
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), 854.0f / 480.0f, 0.05f, 1000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        basicShader.setMat4("projection", projection);
        basicShader.setMat4("view", view);
        basicShader.setMat4("model", model);
        basicShader.setBool("hasTexture", true);

        renderEngine.bindTexture(terrainTex);

        Tessellator* t = Tessellator::instance;
        t->startDrawingQuads();
        t->setColorOpaque(255, 255, 255);
        
        for (auto chunk : chunks) {
            ChunkRenderer::generateMesh(*chunk);
        }

        t->draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
