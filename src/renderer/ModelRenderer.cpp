#include "renderer/ModelRenderer.hpp"
#include "renderer/Shader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

ModelRenderer::ModelRenderer(int textureOffsetX, int textureOffsetY)
    : m_textureOffsetX(textureOffsetX), m_textureOffsetY(textureOffsetY) {
}

ModelRenderer::~ModelRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void ModelRenderer::addBox(float x, float y, float z, int w, int h, int d, float scale) {
    float x1 = x - scale;
    float y1 = y - scale;
    float z1 = z - scale;
    float x2 = x + (float)w + scale;
    float y2 = y + (float)h + scale;
    float z2 = z + (float)d + scale;

    if (mirror) {
        float tmp = x2;
        x2 = x1;
        x1 = tmp;
    }

    // Typical Minecraft box UV mapping logic
    float texU = (float)m_textureOffsetX;
    float texV = (float)m_textureOffsetY;
    float tw = 64.0f; // Standard texture size
    float th = 32.0f;

    auto addQuad = [&](const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4, float u1, float v1, float u2, float v2) {
        m_vertices.push_back({p1, {u2 / tw, v1 / th}});
        m_vertices.push_back({p2, {u1 / tw, v1 / th}});
        m_vertices.push_back({p3, {u1 / tw, v2 / th}});
        m_vertices.push_back({p1, {u2 / tw, v1 / th}});
        m_vertices.push_back({p3, {u1 / tw, v2 / th}});
        m_vertices.push_back({p4, {u2 / tw, v2 / th}});
    };

    float f_w = (float)w;
    float f_h = (float)h;
    float f_d = (float)d;

    // Front (facing -Z, front of player)
    addQuad({x1, y1, z1}, {x2, y1, z1}, {x2, y2, z1}, {x1, y2, z1}, texU + f_d, texV + f_d, texU + f_d + f_w, texV + f_d + f_h);
    // Back (facing +Z, back of player)
    addQuad({x2, y1, z2}, {x1, y1, z2}, {x1, y2, z2}, {x2, y2, z2}, texU + f_d + f_w + f_d, texV + f_d, texU + f_d + f_w + f_d + f_w, texV + f_d + f_h);
    // Top
    addQuad({x2, y1, z1}, {x2, y1, z2}, {x1, y1, z2}, {x1, y1, z1}, texU + f_d, texV, texU + f_d + f_w, texV + f_d);
    // Bottom
    addQuad({x1, y2, z1}, {x1, y2, z2}, {x2, y2, z2}, {x2, y2, z1}, texU + f_d + f_w, texV, texU + f_d + f_w + f_w, texV + f_d);
    // Right
    addQuad({x1, y1, z1}, {x1, y1, z2}, {x1, y2, z2}, {x1, y2, z1}, texU, texV + f_d, texU + f_d, texV + f_d + f_h);
    // Left
    addQuad({x2, y1, z2}, {x2, y1, z1}, {x2, y2, z1}, {x2, y2, z2}, texU + f_d + f_w + f_d, texV + f_d, texU + f_d + f_w + f_d + f_w, texV + f_d + f_h);
}

void ModelRenderer::setRotationPoint(float x, float y, float z) {
    rotationPointX = x;
    rotationPointY = y;
    rotationPointZ = z;
}

void ModelRenderer::compile() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(ModelVertex), m_vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, texCoord));

    glBindVertexArray(0);
    m_compiled = true;
}

void ModelRenderer::render(Shader& shader, const glm::mat4& baseModel, float scale) {
    if (!showModel) return;
    if (!m_compiled) compile();

    glm::mat4 model = baseModel;
    model = glm::translate(model, glm::vec3(rotationPointX * scale, rotationPointY * scale, rotationPointZ * scale));

    if (rotateAngleZ != 0.0f) model = glm::rotate(model, rotateAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));
    if (rotateAngleY != 0.0f) model = glm::rotate(model, rotateAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
    if (rotateAngleX != 0.0f) model = glm::rotate(model, rotateAngleX, glm::vec3(1.0f, 0.0f, 0.0f));

    model = glm::scale(model, glm::vec3(scale, scale, scale));

    shader.setMat4("model", model);
    glVertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());
    glBindVertexArray(0);
}
