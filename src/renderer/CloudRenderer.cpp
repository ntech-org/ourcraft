#include "renderer/CloudRenderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <vector>

struct CloudVertex {
    float x, y, z;
    float u, v;
    uint8_t r, g, b, a;
};

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

void CloudRenderer::render(const World& world, const Camera& camera, const glm::mat4& projection,
                           const glm::mat4& view, const glm::vec3& fogColor, float partialTicks, int cloudLevel) {
    if (cloudLevel <= 0 || m_cloudTexture == 0) return;

    glm::vec3 cloudColor = world.getCloudColor();
    float py = (float)camera.position.y;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.use();
    m_shader.setMat4("projection", projection);
    glm::mat4 cloudView = glm::mat4(glm::mat3(view));
    m_shader.setMat4("view", cloudView);
    m_shader.setMat4("model", glm::mat4(1.0f));
    m_shader.setBool("hasTexture", true);
    m_shader.setBool("useGradient", false);
    m_shader.setBool("useFog", false);

    glBindTexture(GL_TEXTURE_2D, m_cloudTexture);

    float cloudY = 120.0f - py + 0.33f;
    float texel = 0.5f / 1024.0f;
    double scroll = ((double)m_cloudTickCounter + (double)partialTicks) * 0.03;
    double px = camera.position.x + scroll;
    double pz = camera.position.z;
    int gridOffX = (int)std::floor(px / 2048.0);
    int gridOffZ = (int)std::floor(pz / 2048.0);
    px -= (double)(gridOffX * 2048);
    pz -= (double)(gridOffZ * 2048);
    float uOff = (float)(px * (double)texel);
    float vOff = (float)(pz * (double)texel);

    float baseX = (float)camera.position.x;
    float baseZ = (float)camera.position.z;

    int tileSize = 32;
    int gridTiles = 256 / tileSize;

    uint8_t cr = (uint8_t)(cloudColor.r * 255);
    uint8_t cg = (uint8_t)(cloudColor.g * 255);
    uint8_t cb = (uint8_t)(cloudColor.b * 255);

    std::vector<CloudVertex> verts;
    int numTiles = gridTiles * 2;
    verts.reserve(numTiles * numTiles * 6);

    for (int cx = -gridTiles; cx < gridTiles; ++cx) {
        for (int cz = -gridTiles; cz < gridTiles; ++cz) {
            float x0 = baseX + (float)(cx * tileSize);
            float z0 = baseZ + (float)(cz * tileSize);
            float x1 = x0 + (float)tileSize;
            float z1 = z0 + (float)tileSize;
            float u0 = (float)(cx * tileSize) * texel + uOff;
            float v0 = (float)(cz * tileSize) * texel + vOff;
            float u1 = (float)(cx * tileSize + tileSize) * texel + uOff;
            float v1 = (float)(cz * tileSize + tileSize) * texel + vOff;

            verts.push_back({x0, cloudY, z1, u0, v1, cr, cg, cb, 204});
            verts.push_back({x1, cloudY, z1, u1, v1, cr, cg, cb, 204});
            verts.push_back({x1, cloudY, z0, u1, v0, cr, cg, cb, 204});
            verts.push_back({x0, cloudY, z1, u0, v1, cr, cg, cb, 204});
            verts.push_back({x1, cloudY, z0, u1, v0, cr, cg, cb, 204});
            verts.push_back({x0, cloudY, z0, u0, v0, cr, cg, cb, 204});
        }
    }

    if (verts.empty()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(CloudVertex), verts.data(), GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CloudVertex), (void*)20);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}
