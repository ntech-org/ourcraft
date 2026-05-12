#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"
#include "world/World.hpp"
#include "world/BlockFluid.hpp"
#include "world/IBlockAccess.hpp"
#include "physics/AxisAlignedBB.hpp"
#include "entities/EntityPlayer.hpp"

Block* Block::blocksList[256] = { nullptr };
bool Block::opaqueCubeLookup[256] = { false };
int Block::lightOpacity[256] = { 0 };
int Block::lightValue[256] = { 0 };
float Block::blockHardness[256] = { 0.0f };

Block* Block::stone = nullptr;
Block* Block::grass = nullptr;
Block* Block::dirt = nullptr;
Block* Block::cobblestone = nullptr;
Block* Block::planks = nullptr;
Block* Block::sapling = nullptr;
Block* Block::bedrock = nullptr;
Block* Block::waterMoving = nullptr;
Block* Block::waterStill = nullptr;
Block* Block::lavaMoving = nullptr;
Block* Block::lavaStill = nullptr;
Block* Block::sand = nullptr;
Block* Block::gravel = nullptr;
Block* Block::oreGold = nullptr;
Block* Block::oreIron = nullptr;
Block* Block::oreCoal = nullptr;
Block* Block::wood = nullptr;
Block* Block::leaves = nullptr;
Block* Block::sponge = nullptr;
Block* Block::glass = nullptr;
Block* Block::cloth = nullptr;
Block* Block::flowerYellow = nullptr;
Block* Block::flowerRed = nullptr;
Block* Block::mushroomBrown = nullptr;
Block* Block::mushroomRed = nullptr;
Block* Block::blockGold = nullptr;
Block* Block::blockSteel = nullptr;
Block* Block::stairDouble = nullptr;
Block* Block::stairSingle = nullptr;
Block* Block::brick = nullptr;
Block* Block::tnt = nullptr;
Block* Block::bookshelf = nullptr;
Block* Block::cobblestoneMossy = nullptr;
Block* Block::obsidian = nullptr;
Block* Block::torch = nullptr;
Block* Block::fire = nullptr;
Block* Block::mobSpawner = nullptr;
Block* Block::stairCompactWood = nullptr;
Block* Block::chest = nullptr;
Block* Block::gear = nullptr;
Block* Block::oreDiamond = nullptr;
Block* Block::blockDiamond = nullptr;
Block* Block::workbench = nullptr;
Block* Block::crops = nullptr;
Block* Block::farmland = nullptr;
Block* Block::furnaceIdle = nullptr;
Block* Block::furnaceActive = nullptr;
Block* Block::signStanding = nullptr;
Block* Block::doorWood = nullptr;
Block* Block::ladder = nullptr;
Block* Block::minecartTrack = nullptr;
Block* Block::stairCompactStone = nullptr;
Block* Block::signWall = nullptr;

void Block::init() {
    for (int i = 0; i < 256; ++i) {
        blockHardness[i] = 1.0f;
    }
    blockHardness[0] = 0.0f;

    stone = new Block(1, 1, Material::rock);
    grass = new BlockGrass(2);
    dirt = new Block(3, 2, Material::ground);
    cobblestone = new Block(4, 16, Material::rock);
    planks = new Block(5, 4, Material::wood);
    sapling = new BlockCross(6, 15);
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
    flowerYellow = new BlockCross(37, 13);
    flowerRed = new BlockCross(38, 12);
    mushroomBrown = new BlockCross(39, 29);
    mushroomRed = new BlockCross(40, 28);
    blockGold = new Block(41, 39, Material::iron);
    blockSteel = new Block(42, 38, Material::iron);
    stairDouble = new Block(43, 5, Material::rock);
    stairSingle = new Block(44, 6, Material::rock);
    brick = new Block(45, 7, Material::rock);
    tnt = new Block(46, 8, Material::tnt);
    bookshelf = new Block(47, 35, Material::wood);
    cobblestoneMossy = new Block(48, 36, Material::rock);
    obsidian = new Block(49, 37, Material::rock);
    torch = new BlockCross(50, 80);
    fire = new Block(51, 31, Material::fire);
    mobSpawner = new Block(52, 65, Material::rock);
    stairCompactWood = new Block(53, 4, Material::wood);
    chest = new Block(54, 26, Material::wood);
    gear = new Block(55, 62, Material::iron);
    oreDiamond = new Block(56, 50, Material::rock);
    blockDiamond = new Block(57, 40, Material::iron);
    workbench = new BlockWorkbench(58);
    crops = new BlockCross(59, 88);
    farmland = new Block(60, 87, Material::ground);
    furnaceIdle = new Block(61, 44, Material::rock);
    furnaceActive = new Block(62, 60, Material::rock);
    signStanding = new Block(63, 4, Material::wood);
    doorWood = new Block(64, 97, Material::wood);
    ladder = new Block(65, 83, Material::wood);
    minecartTrack = new Block(66, 128, Material::iron);
    stairCompactStone = new Block(67, 16, Material::rock);
    signWall = new Block(68, 4, Material::wood);

    // Set step sounds for each block type
    stone->stepSound = &SOUND_STONE;
    grass->stepSound = &SOUND_GRASS;
    dirt->stepSound = &SOUND_GRAVEL;
    sand->stepSound = &SOUND_SAND;
    gravel->stepSound = &SOUND_GRAVEL;
    wood->stepSound = &SOUND_WOOD;
    leaves->stepSound = &SOUND_GRASS;
    glass->stepSound = &SOUND_GLASS;
    cloth->stepSound = &SOUND_CLOTH;
    cobblestoneMossy->stepSound = &SOUND_STONE;
    brick->stepSound = &SOUND_STONE;
    bookshelf->stepSound = &SOUND_WOOD;
    stairCompactWood->stepSound = &SOUND_WOOD;
    chest->stepSound = &SOUND_WOOD;
    signStanding->stepSound = &SOUND_WOOD;
    signWall->stepSound = &SOUND_WOOD;
    doorWood->stepSound = &SOUND_WOOD;
    ladder->stepSound = &SOUND_WOOD;
    minecartTrack->stepSound = &SOUND_STONE;
    stairCompactStone->stepSound = &SOUND_STONE;
    planks->stepSound = &SOUND_WOOD;
    bedrock->stepSound = &SOUND_STONE;
    cobblestone->stepSound = &SOUND_STONE;

    static const float hardnessValues[] = {
        1, 1.5f, 0.6f, 0.5f, 2, 2, 0, -1, -1, -1, -1, -1, 0.5f, 0.6f, 3, 3, 3,
        2, 0.2f, 0.6f, 0.3f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.8f,
        0, 0, 0, 0, 0, 5, 5, 2, 2, 2, 0, 1.5f, 2, 10, 0, -1, 5, 2.5f, 0,
        3, 5, 2.5f, 0, 0.6f, 3.5f, 3.5f, 1, 3, 0.4f, 0.7f, 2, 1
    };
    for (int i = 0; i < 69; ++i) {
        if (hardnessValues[i] != 0.0f || i == 0) {
            blockHardness[i] = hardnessValues[i];
        }
    }

    lightOpacity[6] = 0;
    lightOpacity[8] = 3;
    lightOpacity[9] = 3;
    lightOpacity[10] = 3;
    lightOpacity[11] = 3;
    lightOpacity[18] = 1;
    lightOpacity[20] = 0;
    lightOpacity[37] = 0;
    lightOpacity[38] = 0;
    lightOpacity[39] = 0;
    lightOpacity[40] = 0;
    lightOpacity[50] = 0;
    lightOpacity[51] = 0;
    lightOpacity[59] = 0;
    lightOpacity[63] = 0;
    lightOpacity[65] = 0;
    lightOpacity[66] = 0;

    lightValue[10] = 15;
    lightValue[11] = 15;
    lightValue[50] = 14;
    lightValue[51] = 15;
    lightValue[62] = 13;
}

Block::Block(int id, int tex, const Material& mat)
    : blockID(id), blockIndexInTexture(tex), blockMaterial(mat)
{
    blocksList[id] = this;
    opaqueCubeLookup[id] = isOpaqueCube();
    lightOpacity[id] = isOpaqueCube() ? 15 : 0;
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

bool Block::isSameTypeCulled() const {
    return false;
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

float Block::getHardness(uint8_t blockID) {
    return blockHardness[blockID];
}
