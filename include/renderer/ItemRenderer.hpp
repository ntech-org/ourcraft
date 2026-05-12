#pragma once

#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace {

inline float interpAngle(float prev, float current, float pTicks) {
    float diff = current - prev;
    while (diff < -180.0f) diff += 360.0f;
    while (diff >= 180.0f) diff -= 360.0f;
    return prev + diff * pTicks;
}

struct FaceUV {
    float u0, v0, u1, v1;
};

inline FaceUV getTextureUV(int tex) {
    float u0 = (float)((tex & 15) * 16) / 256.0f;
    float v0 = (float)((tex >> 4) * 16) / 256.0f;
    return {u0, v0, u0 + 16.0f / 256.0f, v0 + 16.0f / 256.0f};
}

inline bool isInventoryBlockModel(int itemID) {
    return itemID > 0
        && itemID < 256
        && Block::blocksList[itemID]
        && Block::blocksList[itemID]->getRenderShape() == BlockRenderShape::FullCube;
}

inline int getItemIconTexture(int itemID) {
    if (itemID > 0 && itemID < 256 && Block::blocksList[itemID]) {
        return Block::blocksList[itemID]->getTexture(2);
    }
    if (itemID >= 0 && itemID < 1024 && Item::itemsList[itemID]) {
        return Item::itemsList[itemID]->iconIndex;
    }
    return itemID & 255;
}

inline void addFace(Tessellator* t, int side, const FaceUV& uv, float shade) {
    int c = std::clamp((int)std::round(255.0f * shade), 0, 255);
    t->setColorOpaque(c, c, c);

    const float x0 = -0.5f, x1 = 0.5f;
    const float y0 = -0.5f, y1 = 0.5f;
    const float z0 = -0.5f, z1 = 0.5f;

    switch (side) {
        case 0:
            t->setNormal(0.0f, -1.0f, 0.0f);
            t->addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
            break;
        case 1:
            t->setNormal(0.0f, 1.0f, 0.0f);
            t->addVertexWithUV(x1, y1, z1, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y1, z1, uv.u0, uv.v1);
            break;
        case 2:
            t->setNormal(0.0f, 0.0f, 1.0f);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v1);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v1);
            break;
        case 3:
            t->setNormal(0.0f, 0.0f, -1.0f);
            t->addVertexWithUV(x0, y1, z1, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z1, uv.u1, uv.v0);
            break;
        case 4:
            t->setNormal(-1.0f, 0.0f, 0.0f);
            t->addVertexWithUV(x0, y1, z1, uv.u1, uv.v0);
            t->addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
            t->addVertexWithUV(x0, y0, z0, uv.u0, uv.v1);
            t->addVertexWithUV(x0, y0, z1, uv.u1, uv.v1);
            break;
        case 5:
            t->setNormal(1.0f, 0.0f, 0.0f);
            t->addVertexWithUV(x1, y0, z1, uv.u0, uv.v1);
            t->addVertexWithUV(x1, y0, z0, uv.u1, uv.v1);
            t->addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
            t->addVertexWithUV(x1, y1, z1, uv.u0, uv.v0);
            break;
    }
}

inline void renderFlatHeldItem(Tessellator* t, const FaceUV& uv) {
    const float w = 1.0f;
    const float h = 1.0f;
    const float d = 1.0f / 16.0f;
    const float eps = 0.001953125f;

    t->setNormal(0.0f, 0.0f, 1.0f);
    t->addVertexWithUV(0.0f, 0.0f, 0.0f, uv.u1, uv.v1);
    t->addVertexWithUV(w, 0.0f, 0.0f, uv.u0, uv.v1);
    t->addVertexWithUV(w, h, 0.0f, uv.u0, uv.v0);
    t->addVertexWithUV(0.0f, h, 0.0f, uv.u1, uv.v0);

    t->setNormal(0.0f, 0.0f, -1.0f);
    t->addVertexWithUV(0.0f, h, -d, uv.u1, uv.v0);
    t->addVertexWithUV(w, h, -d, uv.u0, uv.v0);
    t->addVertexWithUV(w, 0.0f, -d, uv.u0, uv.v1);
    t->addVertexWithUV(0.0f, 0.0f, -d, uv.u1, uv.v1);

    t->setNormal(-1.0f, 0.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideU = uv.u1 + (uv.u0 - uv.u1) * step - eps;
        float x = w * step;
        t->addVertexWithUV(x, 0.0f, -d, sideU, uv.v1);
        t->addVertexWithUV(x, 0.0f, 0.0f, sideU, uv.v1);
        t->addVertexWithUV(x, h, 0.0f, sideU, uv.v0);
        t->addVertexWithUV(x, h, -d, sideU, uv.v0);
    }

    t->setNormal(1.0f, 0.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideU = uv.u1 + (uv.u0 - uv.u1) * step - eps;
        float x = w * step + d;
        t->addVertexWithUV(x, h, -d, sideU, uv.v0);
        t->addVertexWithUV(x, h, 0.0f, sideU, uv.v0);
        t->addVertexWithUV(x, 0.0f, 0.0f, sideU, uv.v1);
        t->addVertexWithUV(x, 0.0f, -d, sideU, uv.v1);
    }

    t->setNormal(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideV = uv.v1 + (uv.v0 - uv.v1) * step - eps;
        float y = h * step + d;
        t->addVertexWithUV(0.0f, y, 0.0f, uv.u1, sideV);
        t->addVertexWithUV(w, y, 0.0f, uv.u0, sideV);
        t->addVertexWithUV(w, y, -d, uv.u0, sideV);
        t->addVertexWithUV(0.0f, y, -d, uv.u1, sideV);
    }

    t->setNormal(0.0f, -1.0f, 0.0f);
    for (int i = 0; i < 16; ++i) {
        float step = (float)i / 16.0f;
        float sideV = uv.v1 + (uv.v0 - uv.v1) * step - eps;
        float y = h * step;
        t->addVertexWithUV(w, y, 0.0f, uv.u0, sideV);
        t->addVertexWithUV(0.0f, y, 0.0f, uv.u1, sideV);
        t->addVertexWithUV(0.0f, y, -d, uv.u1, sideV);
        t->addVertexWithUV(w, y, -d, uv.u0, sideV);
    }
}

} // namespace
