#pragma once

#include "renderer/Shader.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class RenderEngine;
class World;
class Camera;

struct CloudVertex {
    float x, y, z;
    float u, v;
    std::uint8_t r, g, b, a;
};

class CloudRenderer {
public:
    explicit CloudRenderer(RenderEngine& renderEngine);
    ~CloudRenderer();

    CloudRenderer(const CloudRenderer&) = delete;
    CloudRenderer& operator=(const CloudRenderer&) = delete;

    void render(const World& world, const Camera& camera, const glm::mat4& projection,
                const glm::mat4& view, const glm::vec3& fogColor, float partialTicks, int cloudLevel,
                float fogNear, float fogFar, bool fancyGraphics, int renderDistance);

    /** Advance cloud scroll once per game tick (20 TPS), not per rendered frame. */
    void tick() { m_cloudTickCounter++; }

private:
    void renderSimpleClouds(const World& world, const Camera& camera, const glm::mat4& projection,
                            float partialTicks, int renderDistance);
    void renderFancyClouds(const World& world, const Camera& camera, const glm::mat4& projection,
                           float partialTicks, int cloudLevel, int renderDistance);

    RenderEngine& m_renderEngine;
    Shader m_shader;
    GLuint m_cloudTexture = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_cloudTickCounter = 0;
};
