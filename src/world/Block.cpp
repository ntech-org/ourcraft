#include "world/Block.hpp"
#include "world/BlockFluid.hpp"
#include "world/IBlockAccess.hpp"
#include "physics/AxisAlignedBB.hpp"

Block* Block::blocksList[256] = { nullptr };
bool Block::opaqueCubeLookup[256] = { false };
int Block::lightOpacity[256] = { 0 };
int Block::lightValue[256] = { 0 };

const Block* Block::stone = nullptr;
const Block* Block::grass = nullptr;
const Block* Block::dirt = nullptr;
const Block* Block::cobblestone = nullptr;
const Block* Block::planks = nullptr;
const Block* Block::sapling = nullptr;
const Block* Block::bedrock = nullptr;
const Block* Block::waterMoving = nullptr;
const Block* Block::waterStill = nullptr;
const Block* Block::lavaMoving = nullptr;
const Block* Block::lavaStill = nullptr;
const Block* Block::sand = nullptr;
const Block* Block::gravel = nullptr;
const Block* Block::oreGold = nullptr;
const Block* Block::oreIron = nullptr;
const Block* Block::oreCoal = nullptr;
const Block* Block::wood = nullptr;
const Block* Block::leaves = nullptr;
const Block* Block::sponge = nullptr;
const Block* Block::glass = nullptr;
const Block* Block::cloth = nullptr;
const Block* Block::flowerYellow = nullptr;
const Block* Block::flowerRed = nullptr;
const Block* Block::mushroomBrown = nullptr;
const Block* Block::mushroomRed = nullptr;
const Block* Block::blockGold = nullptr;
const Block* Block::blockSteel = nullptr;
const Block* Block::stairDouble = nullptr;
const Block* Block::stairSingle = nullptr;
const Block* Block::brick = nullptr;
const Block* Block::tnt = nullptr;
const Block* Block::bookshelf = nullptr;
const Block* Block::cobblestoneMossy = nullptr;
const Block* Block::obsidian = nullptr;
const Block* Block::torch = nullptr;
const Block* Block::fire = nullptr;
const Block* Block::mobSpawner = nullptr;
const Block* Block::stairCompactWood = nullptr;
const Block* Block::chest = nullptr;
const Block* Block::gear = nullptr;
const Block* Block::oreDiamond = nullptr;
const Block* Block::blockDiamond = nullptr;
const Block* Block::workbench = nullptr;
const Block* Block::crops = nullptr;
const Block* Block::farmland = nullptr;
const Block* Block::furnaceIdle = nullptr;
const Block* Block::furnaceActive = nullptr;
const Block* Block::signStanding = nullptr;
const Block* Block::doorWood = nullptr;
const Block* Block::ladder = nullptr;
const Block* Block::minecartTrack = nullptr;
const Block* Block::stairCompactStone = nullptr;
const Block* Block::signWall = nullptr;

class BlockGrass : public Block {
public:
    BlockGrass(int id) : Block(id, 3, Material::ground) {}
    int getTexture(int side) const override {
        if (side == 1) return 0; // Top
        if (side == 0) return 2; // Bottom
        return 3; // Sides
    }
};

class BlockLog : public Block {
public:
    BlockLog(int id) : Block(id, 20, Material::wood) {}
    int getTexture(int side) const override {
        if (side == 1 || side == 0) return 21; // Top/Bottom
        return 20; // Sides
    }
};

class BlockLeaves : public Block {
public:
    BlockLeaves(int id) : Block(id, 52, Material::leaves) {}
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
};

class BlockGlass : public Block {
public:
    BlockGlass(int id) : Block(id, 49, Material::glass) {}
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
};

void Block::init() {
    stone = new Block(1, 1, Material::rock);
    grass = new BlockGrass(2);
    dirt = new Block(3, 2, Material::ground);
    cobblestone = new Block(4, 16, Material::rock);
    planks = new Block(5, 4, Material::wood);
    sapling = new Block(6, 15, Material::plants);
    bedrock = new Block(7, 17, Material::rock);
    waterMoving = new BlockFlowing(8, Material::water); 
    waterStill = new BlockStationary(9, Material::water);
    lavaMoving = new BlockFlowing(10, Material::lava);
    lavaStill = new BlockStationary(11, Material::lava);
    sand = new Block(12, 18, Material::sand);
    gravel = new Block(13, 19, Material::sand);
    oreGold = new Block(14, 32, Material::rock);
    oreIron = new Block(15, 33, Material::rock);
    oreCoal = new Block(16, 34, Material::rock);
    wood = new BlockLog(17);
    leaves = new BlockLeaves(18);
    sponge = new Block(19, 48, Material::sponge);
    glass = new BlockGlass(20);
    cloth = new Block(35, 64, Material::cloth);
    flowerYellow = new Block(37, 13, Material::plants);
    flowerRed = new Block(38, 12, Material::plants);
    mushroomBrown = new Block(39, 29, Material::plants);
    mushroomRed = new Block(40, 28, Material::plants);
    blockGold = new Block(41, 39, Material::iron);
    blockSteel = new Block(42, 38, Material::iron);
    stairDouble = new Block(43, 5, Material::rock);
    stairSingle = new Block(44, 6, Material::rock);
    brick = new Block(45, 7, Material::rock);
    tnt = new Block(46, 8, Material::tnt);
    bookshelf = new Block(47, 35, Material::wood);
    cobblestoneMossy = new Block(48, 36, Material::rock);
    obsidian = new Block(49, 37, Material::rock);
    torch = new Block(50, 80, Material::plants);
    fire = new Block(51, 31, Material::fire);
    mobSpawner = new Block(52, 65, Material::rock);
    stairCompactWood = new Block(53, 4, Material::wood);
    chest = new Block(54, 26, Material::wood);
    gear = new Block(55, 62, Material::iron);
    oreDiamond = new Block(56, 50, Material::rock);
    blockDiamond = new Block(57, 40, Material::iron);
    workbench = new Block(58, 43, Material::wood);
    crops = new Block(59, 88, Material::plants);
    farmland = new Block(60, 87, Material::ground);
    furnaceIdle = new Block(61, 44, Material::rock);
    furnaceActive = new Block(62, 60, Material::rock);
    signStanding = new Block(63, 4, Material::wood);
    doorWood = new Block(64, 97, Material::wood);
    ladder = new Block(65, 83, Material::wood);
    minecartTrack = new Block(66, 128, Material::iron);
    stairCompactStone = new Block(67, 16, Material::rock);
    signWall = new Block(68, 4, Material::wood);

    lightOpacity[6] = 0; // sapling
    lightOpacity[8] = 3; // water moving
    lightOpacity[9] = 3; // water still
    lightOpacity[10] = 3; // lava moving
    lightOpacity[11] = 3; // lava still
    lightOpacity[18] = 1; // leaves (standard MC is 1 or 3)
    lightOpacity[20] = 0; // glass
    lightOpacity[37] = 0; // flower y
    lightOpacity[38] = 0; // flower r
    lightOpacity[39] = 0; // mushroom b
    lightOpacity[40] = 0; // mushroom r
    lightOpacity[50] = 0; // torch
    lightOpacity[51] = 0; // fire
    lightOpacity[59] = 0; // crops
    lightOpacity[63] = 0; // sign
    lightOpacity[65] = 0; // ladder
    lightOpacity[66] = 0; // rail

    lightValue[50] = 14; // Torch
    lightValue[51] = 15; // Fire
    lightValue[62] = 13; // Active Furnace
}

Block::Block(int id, int tex, const Material& mat) 
    : blockID(id), blockIndexInTexture(tex), blockMaterial(mat) 
{
    blocksList[id] = this;
    opaqueCubeLookup[id] = isOpaqueCube();
    lightOpacity[id] = isOpaqueCube() ? 255 : 0;
    lightValue[id] = 0;
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

int Block::getTexture(int side) const {
    return blockIndexInTexture;
}

BlockRenderLayer Block::getRenderLayer() const {
    return BlockRenderLayer::Opaque;
}

BlockRenderShape Block::getRenderShape() const {
    return BlockRenderShape::FullCube;
}

bool Block::isFullCube() const {
    return true;
}

bool Block::isOccluder() const {
    return isOpaqueCube() && isFullCube();
}

bool Block::isGreedyMergeable() const {
    return getRenderLayer() == BlockRenderLayer::Opaque && isFullCube();
}

bool Block::isOpaqueCube() const {
    return true;
}

bool Block::shouldSideBeRendered(const IBlockAccess& world, int x, int y, int z, int side) const {
    uint8_t bid = world.getBlockID(x, y, z);
    if (bid == 0) return true;
    if (Block::blocksList[bid]) {
        return !Block::blocksList[bid]->isFullCube();
    }
    return true;
}

void Block::getCollisionBoxes(World& world, int x, int y, int z, const AxisAlignedBB& mask, std::vector<AxisAlignedBB>& list) const {
    AxisAlignedBB bb = getCollisionBoundingBoxFromPool(world, x, y, z);
    if (bb.minX != bb.maxX || bb.minY != bb.maxY || bb.minZ != bb.maxZ) {
        if (bb.intersectsWith(mask)) {
            list.push_back(bb);
        }
    }
}

AxisAlignedBB Block::getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const {
    return AxisAlignedBB((double)x + minX, (double)y + minY, (double)z + minZ, (double)x + maxX, (double)y + maxY, (double)z + maxZ);
}

void Block::setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1) {
    minX = x0; minY = y0; minZ = z0;
    maxX = x1; maxY = y1; maxZ = z1;
}
