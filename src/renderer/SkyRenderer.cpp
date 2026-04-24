#include "renderer/SkyRenderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/RenderEngine.hpp"
#include "world/World.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

SkyRenderer::SkyRenderer(RenderEngine& re) : m_renderEngine(re), m_shader("assets/shaders/sky.vert", "assets/shaders/sky.frag") {
    m_shader.use(); m_shader.setInt("texture1", 0);
    buildPlaneMeshes(); buildSunAndMoonMeshes(); buildStarMesh();
    m_sunTexture = (GLuint)re.getTexture("/terrain/sun.png");
    m_moonTexture = (GLuint)re.getTexture("/terrain/moon.png");
}

SkyRenderer::~SkyRenderer() { destroyMesh(m_topSky); destroyMesh(m_bottomSky); destroyMesh(m_sunMesh); destroyMesh(m_moonMesh); destroyMesh(m_starMesh); }

void SkyRenderer::render(const World& world, const Camera& camera, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& fogColor) {
    m_projection = projection; m_view = glm::mat4(glm::mat3(view));
    glm::vec3 skyColor = world.getSkyColor();
    
    // The passed fogColor already includes void darkening and liquid overrides (water/lava)
    // Using it directly ensures the sky planes blend perfectly into the background clear color.
    glm::vec4 horizonColor(fogColor, 1.0f);
    
    float py = camera.position.y;
    float starB = world.getStarBrightness();
    if (py < 32.0f) {
        float f = std::clamp(py / 32.0f, 0.0f, 1.0f);
        starB *= f * f;
    }
    float angle = world.getCelestialAngle();

    glDepthMask(GL_FALSE); glDisable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
    
    // Draw top sky plane with horizon blending
    drawMesh(m_topSky, glm::mat4(1.0f), glm::vec4(skyColor, 1.0f), false, 0, false, {}, {}, 0, 0, true, horizonColor, 0.0f, 256.0f);
    
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle * glm::two_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    if (starB > 0.0f) { 
        glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
        drawMesh(m_starMesh, rot, glm::vec4(1.0f, 1.0f, 1.0f, starB), false); 
        glDisable(GL_BLEND); glEnable(GL_CULL_FACE); 
    }

    glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    drawMesh(m_sunMesh, rot, glm::vec4(1.0f), true, m_sunTexture); 
    drawMesh(m_moonMesh, rot, glm::vec4(1.0f), true, m_moonTexture);
    glDisable(GL_BLEND); glEnable(GL_CULL_FACE);
    
    // Void color calculation matching GameRenderer
    float pyHorizon = world.getHorizon();
    float voidDarkening = std::clamp(py / pyHorizon, 0.0f, 1.0f);
    voidDarkening *= voidDarkening;
    glm::vec3 baseBottomColor = (skyColor * glm::vec3(0.2f, 0.2f, 0.6f) + glm::vec3(0.04f, 0.04f, 0.1f));
    glm::vec4 bottomPlaneColor(baseBottomColor * (py < pyHorizon ? 0.0f : 1.0f), 1.0f);

    float hOffset = std::min(pyHorizon - py, 0.0f);
    // Draw bottom sky plane (void) with horizon blending
    drawMesh(m_bottomSky, glm::translate(glm::mat4(1.0f), {0, hOffset, 0}), bottomPlaneColor, false, 0, false, {}, {}, 0, 0, true, horizonColor, 0, 256.0f);
    
    glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
}

void SkyRenderer::uploadMesh(SkyMesh& m, const std::vector<SkyVertex>& v, const std::vector<unsigned int>& idx) {
    glGenVertexArrays(1, &m.vao); glGenBuffers(1, &m.vbo); glGenBuffers(1, &m.ebo);
    glBindVertexArray(m.vao); glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(SkyVertex), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), (void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SkyVertex), (void*)20);
    glBindVertexArray(0); m.indexCount = (GLsizei)idx.size();
}

void SkyRenderer::destroyMesh(SkyMesh& m) { if (m.ebo) glDeleteBuffers(1, &m.ebo); if (m.vbo) glDeleteBuffers(1, &m.vbo); if (m.vao) glDeleteVertexArrays(1, &m.vao); m = {0,0,0,0}; }

void SkyRenderer::drawMesh(const SkyMesh& m, const glm::mat4& model, const glm::vec4& tint, bool tex, GLuint tID, bool grad, const glm::vec3& gT, const glm::vec3& gB, float gMin, float gMax, bool fog, const glm::vec4& fC, float fS, float fE) {
    if (!m.indexCount) return;
    m_shader.use(); m_shader.setMat4("projection", m_projection); m_shader.setMat4("view", m_view); m_shader.setMat4("model", model);
    m_shader.setBool("hasTexture", tex); m_shader.setBool("useGradient", grad); m_shader.setVec3("gradientTopColor", gT); m_shader.setVec3("gradientBottomColor", gB);
    m_shader.setFloat("gradientMinY", gMin); m_shader.setFloat("gradientMaxY", gMax); m_shader.setBool("useFog", fog); m_shader.setVec4("fogColor", fC);
    m_shader.setFloat("fogStart", fS); m_shader.setFloat("fogEnd", fE);
    glUniform4f(glGetUniformLocation(m_shader.ID, "tint"), tint.r, tint.g, tint.b, tint.a);
    if (tex && tID) glBindTexture(GL_TEXTURE_2D, tID);
    glBindVertexArray(m.vao); glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr); glBindVertexArray(0);
}
