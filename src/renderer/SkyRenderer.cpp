#include "renderer/SkyRenderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/World.hpp"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

namespace {
constexpr unsigned int kWhite = 0xFFFFFFFFu;

class JavaRandom {
public:
    explicit JavaRandom(int64_t seed)
        : m_seed((seed ^ 0x5DEECE66DL) & ((1LL << 48) - 1)) {}

    int next(int bits) {
        m_seed = (m_seed * 0x5DEECE66DL + 0xBL) & ((1LL << 48) - 1);
        return static_cast<int>(m_seed >> (48 - bits));
    }

    float nextFloat() {
        return static_cast<float>(next(24)) / static_cast<float>(1 << 24);
    }

    double nextDouble() {
        return ((static_cast<int64_t>(next(26)) << 27) + next(27)) / static_cast<double>(1LL << 53);
    }

private:
    int64_t m_seed;
};
}

SkyRenderer::SkyRenderer(RenderEngine& renderEngine)
    : m_renderEngine(renderEngine), m_shader("assets/shaders/sky.vert", "assets/shaders/sky.frag") {
    m_shader.use();
    m_shader.setInt("texture1", 0);

    buildPlaneMeshes();
    buildSunAndMoonMeshes();
    buildStarMesh();

    const int sunTexture = m_renderEngine.getTexture("/terrain/sun.png");
    const int moonTexture = m_renderEngine.getTexture("/terrain/moon.png");
    m_sunTexture = sunTexture >= 0 ? static_cast<GLuint>(sunTexture) : 0;
    m_moonTexture = moonTexture >= 0 ? static_cast<GLuint>(moonTexture) : 0;
}

SkyRenderer::~SkyRenderer() {
    destroyMesh(m_topSky);
    destroyMesh(m_bottomSky);
    destroyMesh(m_sunMesh);
    destroyMesh(m_moonMesh);
    destroyMesh(m_starMesh);
}

void SkyRenderer::render(const World& world, const Camera& camera, const glm::mat4& projection, const glm::mat4& view, float fogMultiplier) {
    m_projection = projection;
    m_view = glm::mat4(glm::mat3(view));

    const glm::vec3 skyColor = world.getSkyColor(); // Don't apply multiplier to top sky color
    glm::vec3 fogColor = world.getFogColor();

    // Blend fog color with sky color based on render distance (assuming Far = 0)
    // float factor = 1.0f - pow(1.0f / (4.0f - renderDistance), 0.25f);
    const float blendFactor = 0.29289321881f; // for renderDistance = 0
    fogColor += (world.getSkyColor() - fogColor) * blendFactor;

    // Void darkening for fog: starts at Y=32
    float fogVoidDarkening = 1.0f;
    if (camera.position.y < 32.0f) {
        fogVoidDarkening = glm::clamp(camera.position.y / 32.0f, 0.0f, 1.0f);
        fogVoidDarkening *= fogVoidDarkening;
    }
    fogColor *= (fogMultiplier * fogVoidDarkening);

    const glm::vec3 bottomColor = {
        skyColor.r * 0.2f + 0.04f,
        skyColor.g * 0.2f + 0.04f,
        skyColor.b * 0.6f + 0.1f
    };

    // Void darkening for bottom sky: completely black below Y=64
    float skyVoidDarkening = 1.0f;
    if (camera.position.y < world.getHorizon()) {
        skyVoidDarkening = 0.0f;
    }

    // Always darken bottom sky based on fogMultiplier and skyVoidDarkening
    const glm::vec3 darkenedBottomColor = bottomColor * (fogMultiplier * skyVoidDarkening);

    const float starBrightness = world.getStarBrightness() * (fogMultiplier * fogVoidDarkening);
    const float celestialAngle = world.getCelestialAngle();

    // In Java Infdev, fogEnd for sky is farPlaneDistance * 0.8
    const float farPlaneDistance = 256.0f; // Matching Far distance
    const float fogEnd = farPlaneDistance * 0.8f;
    const float fogStart = 0.0f;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Render top sky with fog (using darkened fog color, but raw sky color)
    drawMesh(m_topSky, glm::mat4(1.0f), glm::vec4(skyColor, 1.0f), false, 0, false, glm::vec3(1.0f), glm::vec3(1.0f), 0.0f, 1.0f, true, glm::vec4(fogColor, 1.0f), fogStart, fogEnd);

    const glm::mat4 celestialRotation = glm::rotate(glm::mat4(1.0f), celestialAngle * glm::two_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    if (starBrightness > 0.0f) {
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        drawMesh(m_starMesh, celestialRotation, glm::vec4(1.0f, 1.0f, 1.0f, starBrightness), false);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    drawMesh(m_sunMesh, celestialRotation, glm::vec4(1.0f), true, m_sunTexture);
    drawMesh(m_moonMesh, celestialRotation, glm::vec4(1.0f), true, m_moonTexture);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    // Bottom sky - positioned at world horizon (void layer)
    // If below horizon, don't move the plane up, stay at the bottom of the skybox
    float horizonOffset = world.getHorizon() - camera.position.y;
    if (horizonOffset > 0.0f) {
        horizonOffset = 0.0f;
    }
    drawMesh(
        m_bottomSky,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, horizonOffset, 0.0f)),
        glm::vec4(darkenedBottomColor, 1.0f),
        false, 0, false, glm::vec3(1.0f), glm::vec3(1.0f), 0.0f, 1.0f,
        true, glm::vec4(fogColor, 1.0f), fogStart, fogEnd
    );

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void SkyRenderer::uploadMesh(SkyMesh& mesh, const std::vector<SkyVertex>& vertices, const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(SkyVertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), reinterpret_cast<void*>(offsetof(SkyVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), reinterpret_cast<void*>(offsetof(SkyVertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SkyVertex), reinterpret_cast<void*>(offsetof(SkyVertex, color)));

    glBindVertexArray(0);
    mesh.indexCount = static_cast<GLsizei>(indices.size());
}

void SkyRenderer::destroyMesh(SkyMesh& mesh) {
    if (mesh.ebo != 0) {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }
    if (mesh.vbo != 0) {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }
    if (mesh.vao != 0) {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }
    mesh.indexCount = 0;
}

void SkyRenderer::buildPlaneMeshes() {
    // Match old RenderGlobal sky generation: tiled quads from -384 to +384 in 64-block steps.
    const float planeMin = -384.0f;
    const float planeMax = 384.0f;
    const float step = 64.0f;
    const float topY = 16.0f;
    const float bottomY = -16.0f;
    auto buildPlaneMesh = [&](SkyMesh& mesh, float y, bool flipX) {
        std::vector<SkyVertex> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(13 * 13 * 4);
        indices.reserve(13 * 13 * 6);

        for (float x = planeMin; x <= planeMax; x += step) {
            for (float z = planeMin; z <= planeMax; z += step) {
                const float x0 = flipX ? x + step : x;
                const float x1 = flipX ? x : x + step;
                const unsigned int base = static_cast<unsigned int>(vertices.size());

                vertices.push_back({x0, y, z, 0.0f, 0.0f, kWhite});
                vertices.push_back({x1, y, z, 1.0f, 0.0f, kWhite});
                vertices.push_back({x1, y, z + step, 1.0f, 1.0f, kWhite});
                vertices.push_back({x0, y, z + step, 0.0f, 1.0f, kWhite});

                indices.insert(indices.end(), {
                    base + 0, base + 1, base + 2,
                    base + 0, base + 2, base + 3
                });
            }
        }

        uploadMesh(mesh, vertices, indices);
    };

    buildPlaneMesh(m_topSky, topY, false);
    buildPlaneMesh(m_bottomSky, bottomY, true);
}

void SkyRenderer::buildSunAndMoonMeshes() {
    const std::vector<unsigned int> quadIndices = {0, 1, 2, 0, 2, 3};

    {
        const float size = 30.0f;
        std::vector<SkyVertex> sunVertices = {
            {-size, 100.0f, -size, 0.0f, 0.0f, kWhite},
            { size, 100.0f, -size, 1.0f, 0.0f, kWhite},
            { size, 100.0f,  size, 1.0f, 1.0f, kWhite},
            {-size, 100.0f,  size, 0.0f, 1.0f, kWhite}
        };
        uploadMesh(m_sunMesh, sunVertices, quadIndices);
    }

    {
        const float size = 20.0f;
        std::vector<SkyVertex> moonVertices = {
            {-size, -100.0f,  size, 1.0f, 1.0f, kWhite},
            { size, -100.0f,  size, 0.0f, 1.0f, kWhite},
            { size, -100.0f, -size, 0.0f, 0.0f, kWhite},
            {-size, -100.0f, -size, 1.0f, 0.0f, kWhite}
        };
        uploadMesh(m_moonMesh, moonVertices, quadIndices);
    }
}

void SkyRenderer::buildStarMesh() {
    std::vector<SkyVertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(1500 * 4);
    indices.reserve(1500 * 6);

    JavaRandom random(10842LL);

    for (int i = 0; i < 1500; ++i) {
        double x = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double y = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double z = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double size = static_cast<double>(0.25f + random.nextFloat() * 0.25f);
        double lenSq = x * x + y * y + z * z;

        if (lenSq < 1.0 && lenSq > 0.01) {
            double invLen = 1.0 / std::sqrt(lenSq);
            x *= invLen;
            y *= invLen;
            z *= invLen;

            double vx = x * 100.0;
            double vy = y * 100.0;
            double vz = z * 100.0;

            double yaw = std::atan2(x, z);
            double sinYaw = std::sin(yaw);
            double cosYaw = std::cos(yaw);
            double pitch = std::atan2(std::sqrt(x * x + z * z), y);
            double sinPitch = std::sin(pitch);
            double cosPitch = std::cos(pitch);
            double rotation = random.nextDouble() * glm::pi<double>() * 2.0;
            double sinRot = std::sin(rotation);
            double cosRot = std::cos(rotation);

            for (int j = 0; j < 4; ++j) {
                double localX = 0.0;
                double localY = static_cast<double>((j & 2) - 1) * size;
                double localZ = static_cast<double>((j + 1 & 2) - 1) * size;

                double r1 = localY * cosRot - localZ * sinRot;
                double r2 = localZ * cosRot + localY * sinRot;
                double r3 = r1 * sinPitch + localX * cosPitch;
                double r4 = localX * sinPitch - r1 * cosPitch;
                double r5 = r4 * sinYaw - r2 * cosYaw;
                double r6 = r2 * sinYaw + r4 * cosYaw;

                vertices.push_back({
                    static_cast<float>(vx + r5),
                    static_cast<float>(vy + r3),
                    static_cast<float>(vz + r6),
                    0.0f, 0.0f, kWhite
                });
            }

            const unsigned int base = static_cast<unsigned int>(vertices.size()) - 4;
            indices.insert(indices.end(), {
                base + 0, base + 1, base + 2,
                base + 0, base + 2, base + 3
            });
        }
    }

    uploadMesh(m_starMesh, vertices, indices);
}

void SkyRenderer::drawMesh(
    const SkyMesh& mesh,
    const glm::mat4& model,
    const glm::vec4& tint,
    bool textured,
    GLuint texture,
    bool useGradient,
    const glm::vec3& gradientTop,
    const glm::vec3& gradientBottom,
    float gradientMinY,
    float gradientMaxY,
    bool useFog,
    const glm::vec4& fogColor,
    float fogStart,
    float fogEnd
) {
    if (mesh.indexCount == 0) {
        return;
    }

    m_shader.use();
    m_shader.setMat4("projection", m_projection);
    m_shader.setMat4("view", m_view);
    m_shader.setMat4("model", model);
    m_shader.setBool("hasTexture", textured);
    m_shader.setBool("useGradient", useGradient);
    m_shader.setVec3("gradientTopColor", gradientTop);
    m_shader.setVec3("gradientBottomColor", gradientBottom);
    m_shader.setFloat("gradientMinY", gradientMinY);
    m_shader.setFloat("gradientMaxY", gradientMaxY);

    m_shader.setBool("useFog", useFog);
    m_shader.setVec4("fogColor", fogColor);
    m_shader.setFloat("fogStart", fogStart);
    m_shader.setFloat("fogEnd", fogEnd);

    glUniform4f(glGetUniformLocation(m_shader.ID, "tint"), tint.r, tint.g, tint.b, tint.a);

    if (textured && texture != 0) {
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
