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

    // Position: 3 floats, offset 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);

    // UV: 2 floats, offset 12
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (void*)12);

    // Color: 4 bytes (RGBA), offset 20
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 32, (void*)20);

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
    ++addedVertices;

    if (drawMode == GL_QUADS && convertQuadsToTriangles && addedVertices % 4 == 0) {
        // Quad to Triangle conversion (0, 1, 2, 3) -> (0, 1, 2), (0, 2, 3)
        // The Java code does something like this:
        for (int i = 0; i < 2; ++i) {
            int offset = 8 * (3 - i);
            if (hasTexture) {
                rawBuffer[rawBufferIndex + 3] = rawBuffer[rawBufferIndex - offset + 3];
                rawBuffer[rawBufferIndex + 4] = rawBuffer[rawBufferIndex - offset + 4];
            }
            if (hasColor) {
                rawBuffer[rawBufferIndex + 5] = rawBuffer[rawBufferIndex - offset + 5];
            }
            rawBuffer[rawBufferIndex + 0] = rawBuffer[rawBufferIndex - offset + 0];
            rawBuffer[rawBufferIndex + 1] = rawBuffer[rawBufferIndex - offset + 1];
            rawBuffer[rawBufferIndex + 2] = rawBuffer[rawBufferIndex - offset + 2];

            rawBufferIndex += 8;
            ++vertexCount;
        }
    }

    FloatInt fi;

    if (hasTexture) {
        fi.f = textureU;
        rawBuffer[rawBufferIndex + 3] = fi.i;
        fi.f = textureV;
        rawBuffer[rawBufferIndex + 4] = fi.i;
    }

    if (hasColor) {
        rawBuffer[rawBufferIndex + 5] = color;
    }

    // Position
    fi.f = (float)(x + xOffset);
    rawBuffer[rawBufferIndex + 0] = fi.i;
    fi.f = (float)(y + yOffset);
    rawBuffer[rawBufferIndex + 1] = fi.i;
    fi.f = (float)(z + zOffset);
    rawBuffer[rawBufferIndex + 2] = fi.i;

    rawBufferIndex += 8;
    ++vertexCount;

    if (vertexCount % 4 == 0 && rawBufferIndex >= bufferSize - 32) {
        draw();
        isDrawing = true;
    }
}
