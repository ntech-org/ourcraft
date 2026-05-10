#pragma once

#include <glad/glad.h>
#include <vector>
#include <cstdint>

struct Vertex {
    float x, y, z;
    float u, v;
    uint32_t color;
    float nx, ny, nz;
};

class Tessellator {
public:
    static Tessellator* instance;

    static void init();

    Tessellator(size_t bufferSize = 2097152);
    ~Tessellator();

    void startDrawingQuads();
    void startDrawing(GLenum mode);
    void draw();
    
    void addVertex(double x, double y, double z);
    void addVertexWithUV(double x, double y, double z, double u, double v);
    
    void setTextureUV(double u, double v);
    void setColorRGBA(int r, int g, int b, int a = 255);
    void setColorOpaque(int r, int g, int b);
    void setColorOpaque_I(int color);
    void setNormal(float nx, float ny, float nz);
    
    void setTranslation(double x, double y, double z);
    void disableColor();

private:
    void reset();

    std::vector<uint32_t> rawBuffer;
    size_t bufferSize;
    size_t rawBufferIndex = 0;
    int vertexCount = 0;
    int addedVertices = 0;
    
    float textureU = 0.0f;
    float textureV = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    bool hasColor = false;
    bool hasTexture = false;
    bool hasNormal = false;
    float normalX = 0.0f, normalY = 1.0f, normalZ = 0.0f;
    bool isColorDisabled = false;
    bool isDrawing = false;
    
    GLenum drawMode;
    double xOffset = 0, yOffset = 0, zOffset = 0;

    GLuint vao, vbo;
    
    static constexpr bool convertQuadsToTriangles = true;
};
