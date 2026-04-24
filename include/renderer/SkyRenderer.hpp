#pragma once

#include "renderer/Shader.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Camera;
class RenderEngine;
class World;

class SkyRenderer {
public:
    explicit SkyRenderer(RenderEngine& renderEngine);
    ~SkyRenderer();

    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    void render(const World& world, const Camera& camera, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor);

private:
    struct SkyVertex {
        float x, y, z;
        float u, v;
        unsigned int color;
    };

    struct SkyMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
    };

    void uploadMesh(SkyMesh& mesh, const std::vector<SkyVertex>& vertices, const std::vector<unsigned int>& indices);
    void destroyMesh(SkyMesh& mesh);
    void buildPlaneMeshes();
    void buildSunAndMoonMeshes();
    void buildStarMesh();
    void drawMesh(
        const SkyMesh& mesh,
        const glm::mat4& model,
        const glm::vec4& tint,
        bool textured,
        GLuint texture = 0,
        bool useGradient = false,
        const glm::vec3& gradientTop = glm::vec3(1.0f),
        const glm::vec3& gradientBottom = glm::vec3(1.0f),
        float gradientMinY = 0.0f,
        float gradientMaxY = 1.0f,
        bool useFog = false,
        const glm::vec4& fogColor = glm::vec4(1.0f),
        float fogStart = 0.0f,
        float fogEnd = 1.0f
    );

    RenderEngine& m_renderEngine;
    Shader m_shader;
    SkyMesh m_topSky;
    SkyMesh m_bottomSky;
    SkyMesh m_sunMesh;
    SkyMesh m_moonMesh;
    SkyMesh m_starMesh;
    GLuint m_sunTexture = 0;
    GLuint m_moonTexture = 0;
    glm::mat4 m_projection {1.0f};
    glm::mat4 m_view {1.0f};
};
