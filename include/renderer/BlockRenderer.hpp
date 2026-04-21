#pragma once

#include "renderer/Tessellator.hpp"

class BlockRenderer {
public:
    static void renderSingleBlock(int textureIndex, double x, double y, double z);
    
    // Minecraft sides: 0: Bottom, 1: Top, 2: East (-Z), 3: West (+Z), 4: North (-X), 5: South (+X)
    static void renderFace(int textureIndex, double x, double y, double z, int side);

private:
    static void getUVs(int index, float& uMin, float& vMin, float& uMax, float& vMax);
};
