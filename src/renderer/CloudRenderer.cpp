#include "renderer/CloudRenderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>
#include <algorithm>

CloudRenderer::CloudRenderer(RenderEngine& renderEngine)
: m_renderEngine(renderEngine), m_shader("assets/shaders/sky.vert", "assets/shaders/sky.frag") {
    m_shader.use();
    m_shader.setInt("texture1", 0);
    int texID = renderEngine.getTexture("/clouds.png");
    if (texID >= 0) {
        m_cloudTexture = (GLuint)texID;
    }
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

CloudRenderer::~CloudRenderer() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

static void pushQuad(std::vector<CloudVertex>& v,
                     float x0, float y0, float z0, float u0, float v0,
                     float x1, float y1, float z1, float u1, float v1,
                     float x2, float y2, float z2, float u2, float v2,
                     float x3, float y3, float z3, float u3, float v3,
                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    v.push_back({x0, y0, z0, u0, v0, r, g, b, a});
    v.push_back({x1, y1, z1, u1, v1, r, g, b, a});
    v.push_back({x2, y2, z2, u2, v2, r, g, b, a});
    v.push_back({x0, y0, z0, u0, v0, r, g, b, a});
    v.push_back({x2, y2, z2, u2, v2, r, g, b, a});
    v.push_back({x3, y3, z3, u3, v3, r, g, b, a});
}

static uint8_t scaleColor(uint8_t c, float s) {
    return (uint8_t)std::min(255, (int)(c * s));
}

void CloudRenderer::render(const World& world, const Camera& camera, const glm::mat4& projection,
                           const glm::mat4& view, const glm::vec3& fogColor, float partialTicks, int cloudLevel,
                           float fogNear, float fogFar, bool fancyGraphics, int renderDistance) {
    if (cloudLevel <= 0 || m_cloudTexture == 0) return;

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Camera-relative rendering (rotation only), matching sky/infdev cloud path.
    m_shader.use();
    m_shader.setMat4("projection", projection);
    m_shader.setMat4("view", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.1f)) * glm::mat4(glm::mat3(view)));
    m_shader.setMat4("model", glm::mat4(1.0f));
    m_shader.setBool("hasTexture", true);
    m_shader.setBool("useGradient", false);
    m_shader.setBool("useFog", false);
    m_shader.setVec4("tint", glm::vec4(1.0f));
    m_shader.setVec3("uCameraPos", glm::vec3(0.0f));
    m_shader.setFloat("uTime", 0.0f);

    glBindTexture(GL_TEXTURE_2D, m_cloudTexture);
    // Soft alpha edges; clouds.png uses alpha for shape.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // cloudLevel: 0=Off, 1=Fast, 2=Fancy. Also honor legacy fancyGraphics when level is Fast.
    (void)fancyGraphics;
    if (cloudLevel >= 2) {
        renderFancyClouds(world, camera, projection, partialTicks, cloudLevel, renderDistance);
    } else {
        renderSimpleClouds(world, camera, projection, partialTicks, renderDistance);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

static void uploadAndDraw(GLuint vao, GLuint vbo, const std::vector<CloudVertex>& verts, bool twoPassDepth) {
    if (verts.empty()) return;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(CloudVertex)), verts.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CloudVertex), (void*)20);

    if (twoPassDepth) {
        // Infdev colorMask trick: first pass fills the depth buffer, second pass
        // draws color only where depth matches so translucent layers don't stack badly.
        for (int pass = 0; pass < 2; ++pass) {
            if (pass == 0) {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_TRUE);
                glDepthFunc(GL_LESS);
            } else {
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_FALSE);
                glDepthFunc(GL_LEQUAL);
            }
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LESS);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CloudRenderer::renderSimpleClouds(const World& world, const Camera& camera, const glm::mat4& projection,
                                       float partialTicks, int renderDistance) {
    (void)projection;

    float py = (float)camera.position.y;
    float cloudY = 120.0f - py + 0.33f;

    const float uvScale = 0.5f / 1024.0f;

    double scrollX = camera.position.x
                   + ((double)m_cloudTickCounter + (double)partialTicks) * 0.03;
    double scrollZ = camera.position.z;

    int wrapX = (int)std::floor(scrollX / 2048.0);
    int wrapZ = (int)std::floor(scrollZ / 2048.0);
    scrollX -= wrapX * 2048.0;
    scrollZ -= wrapZ * 2048.0;

    float uOff = (float)(scrollX * (double)uvScale);
    float vOff = (float)(scrollZ * (double)uvScale);

    glm::vec3 cloudColor = world.getCloudColor(partialTicks);
    uint8_t cr = (uint8_t)(cloudColor.r * 255);
    uint8_t cg = (uint8_t)(cloudColor.g * 255);
    uint8_t cb = (uint8_t)(cloudColor.b * 255);
    const uint8_t ca = 204;

    const int tileSize = 32;
    const int renderRadius = std::max(32, renderDistance * 16);
    const int halfTiles = (int)std::ceil((float)renderRadius / (float)tileSize) + 1;

    std::vector<CloudVertex> verts;
    verts.reserve((size_t)(halfTiles * 2) * (size_t)(halfTiles * 2) * 6);

    for (int ix = -halfTiles; ix < halfTiles; ++ix) {
        for (int iz = -halfTiles; iz < halfTiles; ++iz) {
            float x0 = (float)(ix * tileSize);
            float z0 = (float)(iz * tileSize);
            float x1 = x0 + (float)tileSize;
            float z1 = z0 + (float)tileSize;

            float u0 = x0 * uvScale + uOff;
            float u1 = x1 * uvScale + uOff;
            float v0 = z0 * uvScale + vOff;
            float v1 = z1 * uvScale + vOff;

            pushQuad(verts,
                     x0, cloudY, z1, u0, v1,
                     x1, cloudY, z1, u1, v1,
                     x1, cloudY, z0, u1, v0,
                     x0, cloudY, z0, u0, v0,
                     cr, cg, cb, ca);
        }
    }

    uploadAndDraw(m_vao, m_vbo, verts, false);
}

void CloudRenderer::renderFancyClouds(const World& world, const Camera& camera, const glm::mat4& projection,
                                       float partialTicks, int cloudLevel, int renderDistance) {
    (void)projection;
    (void)cloudLevel;

    float py = (float)camera.position.y;
    const float cloudScale = 12.0f;
    const float cloudHeight = 4.0f;
    float cloudY = 108.0f - py + 0.33f;

    double cx = (camera.position.x
                 + ((double)m_cloudTickCounter + (double)partialTicks) * 0.03) / (double)cloudScale;
    double cz = camera.position.z / (double)cloudScale + 0.33;

    int wrapX = (int)std::floor(cx / 2048.0);
    int wrapZ = (int)std::floor(cz / 2048.0);
    cx -= wrapX * 2048.0;
    cz -= wrapZ * 2048.0;

    const float uvScale = 1.0f / 256.0f;
    float uOff = (float)std::floor(cx) * uvScale;
    float vOff = (float)std::floor(cz) * uvScale;
    float fracX = (float)(cx - std::floor(cx));
    float fracZ = (float)(cz - std::floor(cz));

    glm::vec3 cloudColor = world.getCloudColor(partialTicks);
    uint8_t cr = (uint8_t)(cloudColor.r * 255);
    uint8_t cg = (uint8_t)(cloudColor.g * 255);
    uint8_t cb = (uint8_t)(cloudColor.b * 255);
    const uint8_t ca = 204;

    const int columnSize = 8;
    const float sectionSize = (float)columnSize * cloudScale;
    const int renderRadius = std::max(32, renderDistance * 16);
    const int sectionRadius = (int)std::ceil((float)renderRadius / sectionSize) + 1;
    const float faceEps = 1.0f / 1024.0f;

    std::vector<CloudVertex> verts;
    verts.reserve(64 * 1024);

    for (int secX = -sectionRadius + 1; secX <= sectionRadius; ++secX) {
        for (int secZ = -sectionRadius + 1; secZ <= sectionRadius; ++secZ) {
            float baseU = (float)(secX * columnSize);
            float baseV = (float)(secZ * columnSize);
            float x0 = (baseU - fracX) * cloudScale;
            float z0 = (baseV - fracZ) * cloudScale;
            float cell = (float)columnSize * cloudScale;
            float x1 = x0 + cell;
            float z1 = z0 + cell;

            if (cloudY > -cloudHeight - 1.0f) {
                float y = cloudY;
                uint8_t r = scaleColor(cr, 0.7f);
                uint8_t g = scaleColor(cg, 0.7f);
                uint8_t b = scaleColor(cb, 0.7f);
                float u00 = (baseU + 0.0f) * uvScale + uOff;
                float u11 = (baseU + (float)columnSize) * uvScale + uOff;
                float v00 = (baseV + 0.0f) * uvScale + vOff;
                float v11 = (baseV + (float)columnSize) * uvScale + vOff;
                pushQuad(verts,
                         x0, y, z1, u00, v11,
                         x1, y, z1, u11, v11,
                         x1, y, z0, u11, v00,
                         x0, y, z0, u00, v00,
                         r, g, b, ca);
            }

            if (cloudY <= cloudHeight + 1.0f) {
                float y = cloudY + cloudHeight - faceEps;
                float u00 = (baseU + 0.0f) * uvScale + uOff;
                float u11 = (baseU + (float)columnSize) * uvScale + uOff;
                float v00 = (baseV + 0.0f) * uvScale + vOff;
                float v11 = (baseV + (float)columnSize) * uvScale + vOff;
                pushQuad(verts,
                         x0, y, z1, u00, v11,
                         x1, y, z1, u11, v11,
                         x1, y, z0, u11, v00,
                         x0, y, z0, u00, v00,
                         cr, cg, cb, ca);
            }

            uint8_t sr = scaleColor(cr, 0.9f);
            uint8_t sg = scaleColor(cg, 0.9f);
            uint8_t sb = scaleColor(cb, 0.9f);
            uint8_t er = scaleColor(cr, 0.8f);
            uint8_t eg = scaleColor(cg, 0.8f);
            uint8_t eb = scaleColor(cb, 0.8f);

            if (secX > -1) {
                for (int s = 0; s < columnSize; ++s) {
                    float xs = x0 + (float)s * cloudScale;
                    float u = (baseU + (float)s + 0.5f) * uvScale + uOff;
                    float v1 = (baseV + (float)columnSize) * uvScale + vOff;
                    float v0 = (baseV + 0.0f) * uvScale + vOff;
                    pushQuad(verts,
                             xs, cloudY, z1, u, v1,
                             xs, cloudY + cloudHeight, z1, u, v1,
                             xs, cloudY + cloudHeight, z0, u, v0,
                             xs, cloudY, z0, u, v0,
                             sr, sg, sb, ca);
                }
            }

            if (secX <= 1) {
                for (int s = 0; s < columnSize; ++s) {
                    float xs = x0 + ((float)s + 1.0f - faceEps) * cloudScale;
                    float u = (baseU + (float)s + 0.5f) * uvScale + uOff;
                    float v1 = (baseV + (float)columnSize) * uvScale + vOff;
                    float v0 = (baseV + 0.0f) * uvScale + vOff;
                    pushQuad(verts,
                             xs, cloudY, z1, u, v1,
                             xs, cloudY + cloudHeight, z1, u, v1,
                             xs, cloudY + cloudHeight, z0, u, v0,
                             xs, cloudY, z0, u, v0,
                             sr, sg, sb, ca);
                }
            }

            if (secZ > -1) {
                for (int s = 0; s < columnSize; ++s) {
                    float zs = z0 + (float)s * cloudScale;
                    float u0 = (baseU + 0.0f) * uvScale + uOff;
                    float u1 = (baseU + (float)columnSize) * uvScale + uOff;
                    float v = (baseV + (float)s + 0.5f) * uvScale + vOff;
                    pushQuad(verts,
                             x0, cloudY + cloudHeight, zs, u0, v,
                             x1, cloudY + cloudHeight, zs, u1, v,
                             x1, cloudY, zs, u1, v,
                             x0, cloudY, zs, u0, v,
                             er, eg, eb, ca);
                }
            }

            if (secZ <= 1) {
                for (int s = 0; s < columnSize; ++s) {
                    float zs = z0 + ((float)s + 1.0f - faceEps) * cloudScale;
                    float u0 = (baseU + 0.0f) * uvScale + uOff;
                    float u1 = (baseU + (float)columnSize) * uvScale + uOff;
                    float v = (baseV + (float)s + 0.5f) * uvScale + vOff;
                    pushQuad(verts,
                             x0, cloudY + cloudHeight, zs, u0, v,
                             x1, cloudY + cloudHeight, zs, u1, v,
                             x1, cloudY, zs, u1, v,
                             x0, cloudY, zs, u0, v,
                             er, eg, eb, ca);
                }
            }
        }
    }

    uploadAndDraw(m_vao, m_vbo, verts, true);
}
