#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"
#include "world/BlockFurnace.hpp"
#include "world/BlockCrops.hpp"
#include "world/WorldGenTrees.hpp"
#include "world/World.hpp"
#include "world/BlockFluid.hpp"
#include "world/IBlockAccess.hpp"
#include "world/Material.hpp"
#include "world/JavaRandom.hpp"
#include "physics/AxisAlignedBB.hpp"
#include "entities/EntityPlayer.hpp"
#include "entities/Entity.hpp"
#include "entities/EntityItem.hpp"
#include "items/Item.hpp"
#include <iterator>
#include <cstdlib>

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

    stone = new BlockStone(1);
    grass = new BlockGrass(2);
    dirt = new Block(3, 2, Material::ground);
    cobblestone = new Block(4, 16, Material::rock);
    planks = new Block(5, 4, Material::wood);
    sapling = new BlockSapling(6, 15);
    bedrock = new Block(7, 17, Material::rock);
    waterMoving = new BlockFlowing(8, Material::water);
    waterStill = new BlockStationary(9, Material::water);
    lavaMoving = new BlockFlowing(10, Material::lava);
    lavaStill = new BlockStationary(11, Material::lava);
    sand = new Block(12, 18, Material::sand);
    gravel = new BlockGravel(13);
    oreGold = new BlockOre(14, 32);
    oreIron = new BlockOre(15, 33);
    oreCoal = new BlockOre(16, 34);
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
    torch = new BlockTorch(50, 80);
    fire = new Block(51, 31, Material::fire);
    mobSpawner = new Block(52, 65, Material::rock);
    stairCompactWood = new Block(53, 4, Material::wood);
    chest = new BlockChest(54);
    gear = new Block(55, 62, Material::iron);
    oreDiamond = new BlockOre(56, 50);
    blockDiamond = new Block(57, 40, Material::iron);
    workbench = new BlockWorkbench(58);
    crops = new BlockCrops(59, 88);
    farmland = new BlockFarmland(60);
    furnaceIdle = new BlockFurnace(61, false, Material::rock);
    furnaceActive = new BlockFurnace(62, true, Material::rock);
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
    for (std::size_t i = 0; i < std::size(hardnessValues); ++i) {
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
    lightOpacity[60] = 255;
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
    AxisAlignedBB bounds = getBlockBounds(world, x, y, z);
    return AxisAlignedBB((double)x + bounds.minX, (double)y + bounds.minY, (double)z + bounds.minZ,
                         (double)x + bounds.maxX, (double)y + bounds.maxY, (double)z + bounds.maxZ);
}

AxisAlignedBB Block::getSelectedBoundingBoxFromPool(World& world, int x, int y, int z) const {
    AxisAlignedBB bounds = getBlockBounds(world, x, y, z);
    return AxisAlignedBB((double)x + bounds.minX, (double)y + bounds.minY, (double)z + bounds.minZ,
                         (double)x + bounds.maxX, (double)y + bounds.maxY, (double)z + bounds.maxZ);
}

AxisAlignedBB Block::getBlockBounds(const IBlockAccess& world, int x, int y, int z) const {
    return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
}

HitResult Block::collisionRayTrace(World& world, int x, int y, int z, glm::dvec3 start, glm::dvec3 end) const {
    AxisAlignedBB bounds = getBlockBounds(world, x, y, z);
    AxisAlignedBB bb((double)x + bounds.minX, (double)y + bounds.minY, (double)z + bounds.minZ,
                     (double)x + bounds.maxX, (double)y + bounds.maxY, (double)z + bounds.maxZ);
    if (bb.minX == bb.maxX && bb.minY == bb.maxY && bb.minZ == bb.maxZ) return {HitType::NONE};
    auto hit = bb.calculateIntercept(start, end);
    if (!hit) return {HitType::NONE};
    return {HitType::BLOCK, x, y, z, hit->side, hit->hitVec};
}

static bool isNormalCube(const World& world, int x, int y, int z) {
    uint8_t id = world.getBlockID(x, y, z);
    return id > 0 && Block::blocksList[id] && Block::blocksList[id]->isFullCube();
}

bool BlockTorch::canPlaceBlockAt(World& world, int x, int y, int z) const {
    return isNormalCube(world, x - 1, y, z) || isNormalCube(world, x + 1, y, z) ||
           isNormalCube(world, x, y, z - 1) || isNormalCube(world, x, y, z + 1) ||
           isNormalCube(world, x, y - 1, z);
}

void BlockTorch::onBlockPlaced(World& world, int x, int y, int z, int side, float hitX, float hitY, float hitZ) const {
    int metadata = world.getBlockMetadata(x, y, z);
    if (side == 1 && isNormalCube(world, x, y - 1, z)) metadata = 5;
    if (side == 2 && isNormalCube(world, x, y, z + 1)) metadata = 4;
    if (side == 3 && isNormalCube(world, x, y, z - 1)) metadata = 3;
    if (side == 4 && isNormalCube(world, x + 1, y, z)) metadata = 2;
    if (side == 5 && isNormalCube(world, x - 1, y, z)) metadata = 1;
    world.setBlockMetadataWithNotify(x, y, z, (uint8_t)metadata);
}

void BlockTorch::onBlockAdded(World& world, int x, int y, int z) const {
    if (world.getBlockMetadata(x, y, z) == 0) {
        if (isNormalCube(world, x - 1, y, z)) world.setBlockMetadataWithNotify(x, y, z, 1);
        else if (isNormalCube(world, x + 1, y, z)) world.setBlockMetadataWithNotify(x, y, z, 2);
        else if (isNormalCube(world, x, y, z - 1)) world.setBlockMetadataWithNotify(x, y, z, 3);
        else if (isNormalCube(world, x, y, z + 1)) world.setBlockMetadataWithNotify(x, y, z, 4);
        else if (isNormalCube(world, x, y - 1, z)) world.setBlockMetadataWithNotify(x, y, z, 5);
    }
    if (!canPlaceBlockAt(world, x, y, z)) world.setBlockWithNotify(x, y, z, 0);
}

void BlockTorch::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    int metadata = world.getBlockMetadata(x, y, z);
    bool supported = (metadata == 1 && isNormalCube(world, x - 1, y, z)) ||
                     (metadata == 2 && isNormalCube(world, x + 1, y, z)) ||
                     (metadata == 3 && isNormalCube(world, x, y, z - 1)) ||
                     (metadata == 4 && isNormalCube(world, x, y, z + 1)) ||
                     (metadata == 5 && isNormalCube(world, x, y - 1, z));
    if (supported) return;

    if (!world.isRemote) {
        auto item = std::make_unique<EntityItem>(world, blockID, 1, 0);
        item->setPosition(x + 0.5, y + 0.5, z + 0.5);
        item->delayBeforeCanPickup = 10;
        world.spawnEntity(std::move(item));
    }
    world.setBlockWithNotify(x, y, z, 0);
}

bool BlockChest::canPlaceBlockAt(World& world, int x, int y, int z) const {
    int adjacent = 0;
    const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& offset : offsets) {
        int nx = x + offset[0], nz = z + offset[1];
        if (world.getBlockID(nx, y, nz) != blockID) continue;
        if (++adjacent > 1) return false;
        for (const auto& neighborOffset : offsets) {
            int nnx = nx + neighborOffset[0], nnz = nz + neighborOffset[1];
            if (nnx == x && nnz == z) continue;
            if (world.getBlockID(nnx, y, nnz) == blockID) return false;
        }
    }
    return true;
}

void BlockChest::onBlockAdded(World& world, int x, int y, int z) const {
    if (world.getTileEntity(x, y, z)) return;
    auto chest = std::make_unique<TileEntityChest>();
    chest->x = x; chest->y = y; chest->z = z; chest->world = &world;
    if (!world.isRemote && world.getSaveHandler()) {
        world.getSaveHandler()->loadChest(x, y, z, chest->chestContents, TileEntityChest::CHEST_SIZE);
    }
    world.addTileEntity(std::move(chest));
}

void BlockChest::onBlockRemoval(World& world, int x, int y, int z, int metadata) const {
    auto* chest = dynamic_cast<TileEntityChest*>(world.getTileEntity(x, y, z));
    if (chest && !world.isRemote) {
        for (const ItemStack& stack : chest->chestContents) {
            if (stack.isEmpty()) continue;
            auto item = std::make_unique<EntityItem>(world, stack.itemID, stack.count, stack.metadata);
            item->setPosition(x + 0.5, y + 0.5, z + 0.5);
            item->delayBeforeCanPickup = 10;
            world.spawnEntity(std::move(item));
        }
        if (world.getSaveHandler()) world.getSaveHandler()->removeChest(x, y, z);
    }
    world.removeTileEntity(x, y, z);
}

void Block::setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1) const {
    minX = x0; minY = y0; minZ = z0;
    maxX = x1; maxY = y1; maxZ = z1;
}

int Block::idDropped(int metadata) const {
    return blockID;
}

bool BlockFarmland::isWaterNearby(World& world, int x, int y, int z) const {
    for (int dx = -4; dx <= 4; ++dx) {
        for (int dy = 0; dy <= 1; ++dy) {
            for (int dz = -4; dz <= 4; ++dz) {
                if (world.getBlockMaterial(x + dx, y + dy, z + dz) == Material::water) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool BlockFarmland::isCropsNearby(World& world, int x, int y, int z) const {
    for (int dx = x; dx <= x; ++dx) {
        for (int dz = z; dz <= z; ++dz) {
            if (world.getBlockID(dx, y + 1, dz) == Block::crops->blockID) {
                return true;
            }
        }
    }
    return false;
}

void BlockFarmland::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    if (random.nextInt(5) != 0) return;
    if (isWaterNearby(world, x, y, z)) {
        world.setBlockMetadataWithNotify(x, y, z, 7);
        return;
    }
    int meta = world.getBlockMetadata(x, y, z);
    if (meta > 0) {
        world.setBlockMetadataWithNotify(x, y, z, meta - 1);
    } else if (!isCropsNearby(world, x, y, z)) {
        world.setBlockWithNotify(x, y, z, Block::dirt->blockID);
    }
}

void BlockFarmland::onEntityWalking(World& world, int x, int y, int z, Entity* entity) const {
    (void)entity;
    if (std::rand() % 4 == 0) {
        world.setBlockWithNotify(x, y, z, Block::dirt->blockID);
    }
}

void BlockFarmland::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    (void)neighborID;
    if (world.getBlockMaterial(x, y + 1, z).isSolid()) {
        world.setBlockWithNotify(x, y, z, Block::dirt->blockID);
    }
}

void BlockFarmland::onBlockAdded(World& world, int x, int y, int z) const {
    world.setBlockMetadataWithNotify(x, y, z, 0);
}

bool BlockSapling::canPlaceBlockAt(World& world, int x, int y, int z) const {
    int belowID = world.getBlockID(x, y - 1, z);
    return belowID == Block::grass->blockID || belowID == Block::dirt->blockID;
}

bool BlockSapling::canBlockStay(World& world, int x, int y, int z) const {
    auto lightPair = world.getLightPair(x, y, z);
    if (std::max(lightPair.first, lightPair.second) < 8) return false;
    int belowID = world.getBlockID(x, y - 1, z);
    return belowID == Block::grass->blockID || belowID == Block::dirt->blockID;
}

void BlockSapling::updateTick(World& world, int x, int y, int z, JavaRandom& random) const {
    if (world.getBlockID(x, y, z) != blockID) return;
    auto lightPair = world.getLightPair(x, y, z);
    int light = std::max(lightPair.first, lightPair.second);
    if (light < 9 || random.nextInt(5) != 0) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
        return;
    }
    int meta = world.getBlockMetadata(x, y, z);
    if (meta < 15) {
        world.setBlockMetadataWithNotify(x, y, z, (uint8_t)(meta + 1));
    } else {
        world.setBlockWithNotify(x, y, z, 0);
        WorldGenTrees gen;
        if (!gen.generate(world, random, x, y, z)) {
            world.setBlockWithNotify(x, y, z, blockID);
        }
    }
}

void BlockSapling::onBlockAdded(World& world, int x, int y, int z) const {
    if (world.getBlockID(x, y, z) == blockID) {
        world.scheduleBlockUpdate(x, y, z, blockID, tickRate());
    }
}

void BlockSapling::onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {
    (void)neighborID;
    if (world.getBlockID(x, y, z) != blockID) return;
    if (!canBlockStay(world, x, y, z)) {
        world.setBlockWithNotify(x, y, z, 0);
    }
}

float Block::getHardness(uint8_t blockID) {
    return blockHardness[blockID];
}
