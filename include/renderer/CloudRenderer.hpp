#pragma once

#include "renderer/Shader.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>

class RenderEngine;
class World;
class Camera;

class CloudRenderer {
public:
    explicit CloudRenderer(RenderEngine& renderEngine);
    ~CloudRenderer();

    CloudRenderer(const CloudRenderer&) = delete;
    CloudRenderer& operator=(const CloudRenderer&) = delete;

    void render(const World& world, const Camera& camera, const glm::mat4& projection,
                const glm::mat4& view, const glm::vec3& fogColor, float partialTicks, int cloudLevel);

    void tick() { m_cloudTickCounter++; }

private:
    RenderEngine& m_renderEngine;
    Shader m_shader;
    GLuint m_cloudTexture = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_cloudTickCounter = 0;
};
