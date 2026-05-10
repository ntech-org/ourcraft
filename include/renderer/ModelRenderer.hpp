#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Shader;

struct ModelVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class ModelRenderer {
public:
    ModelRenderer(int textureOffsetX, int textureOffsetY);
    ~ModelRenderer();

    void addBox(float x, float y, float z, int width, int height, int depth, float scale = 0.0f);
    void setRotationPoint(float x, float y, float z);
    void render(Shader& shader, const glm::mat4& baseModel, float scale);

    float rotationPointX = 0.0f;
    float rotationPointY = 0.0f;
    float rotationPointZ = 0.0f;
    float rotateAngleX = 0.0f;
    float rotateAngleY = 0.0f;
    float rotateAngleZ = 0.0f;
    bool mirror = false;
    bool showModel = true;

private:
    void compile();

    int m_textureOffsetX;
    int m_textureOffsetY;
    std::vector<ModelVertex> m_vertices;
    GLuint m_vao = 0, m_vbo = 0;
    bool m_compiled = false;
};
