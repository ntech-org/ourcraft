#pragma once

#include "renderer/Tessellator.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

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

void addFace(Tessellator* t, int side, const FaceUV& uv, float shade);
void renderFlatHeldItem(Tessellator* t, const FaceUV& uv);
