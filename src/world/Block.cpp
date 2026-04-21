#include "world/Block.hpp"

Block* Block::blocksList[256] = { nullptr };
bool Block::opaqueCubeLookup[256] = { false };
int Block::lightOpacity[256] = { 0 };

const Block* Block::stone = nullptr;
const Block* Block::grass = nullptr;
const Block* Block::dirt = nullptr;
const Block* Block::cobblestone = nullptr;
const Block* Block::planks = nullptr;
const Block* Block::bedrock = nullptr;

class BlockGrass : public Block {
public:
    BlockGrass(int id) : Block(id, 3, Material::ground) {}
    int getTexture(int side) const override {
        if (side == 1) return 0; // Top
        if (side == 0) return 2; // Bottom
        return 3; // Sides
    }
};

void Block::init() {
    stone = new Block(1, 1, Material::rock);
    grass = new BlockGrass(2);
    dirt = new Block(3, 2, Material::ground);
    cobblestone = new Block(4, 16, Material::rock);
    planks = new Block(5, 4, Material::wood);
    bedrock = new Block(7, 17, Material::rock);
}

Block::Block(int id, int tex, const Material& mat) 
    : blockID(id), blockIndexInTexture(tex), blockMaterial(mat) 
{
    blocksList[id] = this;
    opaqueCubeLookup[id] = isOpaqueCube();
    lightOpacity[id] = isOpaqueCube() ? 255 : 0;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

int Block::getTexture(int side) const {
    return blockIndexInTexture;
}

bool Block::isOpaqueCube() const {
    return true;
}

void Block::setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1) {
    minX = x0; minY = y0; minZ = z0;
    maxX = x1; maxY = y1; maxZ = z1;
}
