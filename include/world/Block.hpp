#pragma once

#include "world/Material.hpp"
#include <string>
#include <vector>

class Block {
public:
    static Block* blocksList[256];
    static bool opaqueCubeLookup[256];
    static int lightOpacity[256];

    static const Block* stone;
    static const Block* grass;
    static const Block* dirt;
    static const Block* cobblestone;
    static const Block* planks;
    static const Block* bedrock;

    static void init();

    Block(int id, int tex, const Material& mat);
    virtual ~Block() = default;

    virtual int getTexture(int side) const;
    bool isOpaqueCube() const;

    const int blockID;
    int blockIndexInTexture;
    const Material& blockMaterial;

protected:
    void setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1);

    double minX, minY, minZ;
    double maxX, maxY, maxZ;
};
