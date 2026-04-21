#include "renderer/BlockRenderer.hpp"

void BlockRenderer::getUVs(int index, float& uMin, float& vMin, float& uMax, float& vMax) {
    float u = (float)(index % 16) / 16.0f;
    float v = (float)(index / 16) / 16.0f;
    uMin = u;
    vMin = v;
    uMax = u + 0.0624375f; // 15.984375 / 256
    vMax = v + 0.0624375f;
}

void BlockRenderer::renderFace(int textureIndex, double x, double y, double z, int side) {
    Tessellator* t = Tessellator::instance;
    float u0, v0, u1, v1;
    getUVs(textureIndex, u0, v0, u1, v1);

    double x0 = x;
    double x1 = x + 1.0;
    double y0 = y;
    double y1 = y + 1.0;
    double z0 = z;
    double z1 = z + 1.0;

    switch(side) {
        case 0: // Bottom (-Y)
            t->addVertexWithUV(x0, y0, z1, u0, v1);
            t->addVertexWithUV(x0, y0, z0, u0, v0);
            t->addVertexWithUV(x1, y0, z0, u1, v0);
            t->addVertexWithUV(x1, y0, z1, u1, v1);
            break;
        case 1: // Top (+Y)
            t->addVertexWithUV(x1, y1, z1, u1, v1);
            t->addVertexWithUV(x1, y1, z0, u1, v0);
            t->addVertexWithUV(x0, y1, z0, u0, v0);
            t->addVertexWithUV(x0, y1, z1, u0, v1);
            break;
        case 2: // East (-Z)
            t->addVertexWithUV(x0, y1, z0, u1, v0);
            t->addVertexWithUV(x1, y1, z0, u0, v0);
            t->addVertexWithUV(x1, y0, z0, u0, v1);
            t->addVertexWithUV(x0, y0, z0, u1, v1);
            break;
        case 3: // West (+Z)
            t->addVertexWithUV(x0, y1, z1, u0, v0);
            t->addVertexWithUV(x0, y0, z1, u0, v1);
            t->addVertexWithUV(x1, y0, z1, u1, v1);
            t->addVertexWithUV(x1, y1, z1, u1, v0);
            break;
        case 4: // North (-X)
            t->addVertexWithUV(x0, y1, z1, u1, v0);
            t->addVertexWithUV(x0, y1, z0, u0, v0);
            t->addVertexWithUV(x0, y0, z0, u0, v1);
            t->addVertexWithUV(x0, y0, z1, u1, v1);
            break;
        case 5: // South (+X)
            t->addVertexWithUV(x1, y0, z1, u0, v1);
            t->addVertexWithUV(x1, y0, z0, u1, v1);
            t->addVertexWithUV(x1, y1, z0, u1, v0);
            t->addVertexWithUV(x1, y1, z1, u0, v0);
            break;
    }
}

void BlockRenderer::renderSingleBlock(int textureIndex, double x, double y, double z) {
    for (int i = 0; i < 6; ++i) {
        renderFace(textureIndex, x, y, z, i);
    }
}
