#include "renderer/Tessellator.hpp"
#include <cstring>
#include <stdexcept>

Tessellator* Tessellator::instance = nullptr;

void Tessellator::init() {
    if (!instance) {
        instance = new Tessellator(2097152);
    }
}

Tessellator::Tessellator(size_t bufferSize) : bufferSize(bufferSize) {
    rawBuffer.resize(bufferSize);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Position: 3 floats, offset 0, stride 36 (9 uint32s)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 36, (void*)0);

    // UV: 2 floats, offset 12
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 36, (void*)12);

    // Color: 4 bytes (RGBA), offset 20
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 36, (void*)20);

    // Normal: 3 floats, offset 24
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 36, (void*)24);

    glBindVertexArray(0);
}

Tessellator::~Tessellator() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Tessellator::reset() {
    vertexCount = 0;
    rawBufferIndex = 0;
    addedVertices = 0;
    hasNormal = false;
    normalX = 0.0f; normalY = 1.0f; normalZ = 0.0f;
}

void Tessellator::startDrawingQuads() {
    startDrawing(GL_QUADS);
}

void Tessellator::startDrawing(GLenum mode) {
    if (isDrawing) {
        throw std::runtime_error("Already tesselating!");
    }
    isDrawing = true;
    reset();
    drawMode = mode;
    hasColor = false;
    hasTexture = false;
    isColorDisabled = false;
}

void Tessellator::draw() {
    if (!isDrawing) {
        throw std::runtime_error("Not tesselating!");
    }
    isDrawing = false;

    if (vertexCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, rawBufferIndex * sizeof(uint32_t), rawBuffer.data(), GL_STREAM_DRAW);

        glBindVertexArray(vao);

        GLenum mode = drawMode;
        if (mode == GL_QUADS && convertQuadsToTriangles) {
            // In modern OpenGL, GL_QUADS is deprecated or unavailable in core profile.
            // But the Java code already converts them to triangles if convertQuadsToTriangles is true.
            // Wait, let's look at the Java code's addVertex.
            mode = GL_TRIANGLES;
        }

        glDrawArrays(mode, 0, vertexCount);

        glBindVertexArray(0);
    }

    reset();
}

void Tessellator::setTextureUV(double u, double v) {
    hasTexture = true;
    textureU = (float)u;
    textureV = (float)v;
}

void Tessellator::setColorRGBA(int r, int g, int b, int a) {
    if (isColorDisabled) return;
    if (r > 255) r = 255; else if (r < 0) r = 0;
    if (g > 255) g = 255; else if (g < 0) g = 0;
    if (b > 255) b = 255; else if (b < 0) b = 0;
    if (a > 255) a = 255; else if (a < 0) a = 0;

    hasColor = true;
    // ABGR or RGBA? Java says: color = var4 << 24 | var3 << 16 | var2 << 8 | var1;
    // That is A << 24 | B << 16 | G << 8 | R.
    // In little endian, this is R, G, B, A in memory.
    color = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
}

void Tessellator::setColorOpaque(int r, int g, int b) {
    setColorRGBA(r, g, b, 255);
}

void Tessellator::setColorOpaque_I(int c) {
    int r = (c >> 16) & 255;
    int g = (c >> 8) & 255;
    int b = c & 255;
    setColorOpaque(r, g, b);
}

void Tessellator::setNormal(float nx, float ny, float nz) {
    hasNormal = true;
    normalX = nx;
    normalY = ny;
    normalZ = nz;
}

void Tessellator::setTranslation(double x, double y, double z) {
    xOffset = x;
    yOffset = y;
    zOffset = z;
}

void Tessellator::disableColor() {
    isColorDisabled = true;
}

union FloatInt {
    float f;
    uint32_t i;
};

void Tessellator::addVertexWithUV(double x, double y, double z, double u, double v) {
    setTextureUV(u, v);
    addVertex(x, y, z);
}

void Tessellator::addVertex(double x, double y, double z) {
    if (drawMode == GL_QUADS && convertQuadsToTriangles && addedVertices % 4 == 3) {
        // When we are about to add the 4th vertex of a quad,
        // we first duplicate vertex 0 and vertex 2 to form two triangles.
        // Current buffer has: [V0, V1, V2]
        // We want: [V0, V1, V2, V0, V2, V3]
        
        // Copy V0 (at rawBufferIndex - 27)
        for (int j = 0; j < 9; ++j) {
            rawBuffer[rawBufferIndex + j] = rawBuffer[rawBufferIndex - 27 + j];
        }
        rawBufferIndex += 9;
        vertexCount++;

        // Copy V2 (at rawBufferIndex - 18)
        for (int j = 0; j < 9; ++j) {
            rawBuffer[rawBufferIndex + j] = rawBuffer[rawBufferIndex - 18 + j];
        }
        rawBufferIndex += 9;
        vertexCount++;
    }

    FloatInt fi;
    if (hasTexture) {
        fi.f = textureU; rawBuffer[rawBufferIndex + 3] = fi.i;
        fi.f = textureV; rawBuffer[rawBufferIndex + 4] = fi.i;
    }
    if (hasColor) rawBuffer[rawBufferIndex + 5] = color;

    fi.f = (float)(x + xOffset); rawBuffer[rawBufferIndex + 0] = fi.i;
    fi.f = (float)(y + yOffset); rawBuffer[rawBufferIndex + 1] = fi.i;
    fi.f = (float)(z + zOffset); rawBuffer[rawBufferIndex + 2] = fi.i;

    if (hasNormal) {
        fi.f = normalX; rawBuffer[rawBufferIndex + 6] = fi.i;
        fi.f = normalY; rawBuffer[rawBufferIndex + 7] = fi.i;
        fi.f = normalZ; rawBuffer[rawBufferIndex + 8] = fi.i;
    } else {
        fi.f = 0.0f; rawBuffer[rawBufferIndex + 6] = fi.i;
        fi.f = 1.0f; rawBuffer[rawBufferIndex + 7] = fi.i;
        fi.f = 0.0f; rawBuffer[rawBufferIndex + 8] = fi.i;
    }

    rawBufferIndex += 9;
    vertexCount++;
    addedVertices++;

    if (rawBufferIndex >= bufferSize - 72) {
        draw();
        isDrawing = true;
    }
}
