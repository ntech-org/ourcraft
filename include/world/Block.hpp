#pragma once

#include "world/Material.hpp"
#include <glm/vec3.hpp>
#include <cstdint>
#include <string>
#include <vector>

enum class BlockRenderLayer {
    Opaque,
    Cutout,
    Translucent,
    Special
};

enum class BlockRenderShape {
    FullCube,
    Cross,
    Special
};

class World;
class AxisAlignedBB;
class JavaRandom;
class Entity;

class Block {
public:
    static Block* blocksList[256];
    static bool opaqueCubeLookup[256];
    static int lightOpacity[256];
    static int lightValue[256];
    static float blockHardness[256];

    static const Block* stone;
    static const Block* grass;
    static const Block* dirt;
    static const Block* cobblestone;
    static const Block* planks;
    static const Block* sapling;
    static const Block* bedrock;
    static const Block* waterMoving;
    static const Block* waterStill;
    static const Block* lavaMoving;
    static const Block* lavaStill;
    static const Block* sand;
    static const Block* gravel;
    static const Block* oreGold;
    static const Block* oreIron;
    static const Block* oreCoal;
    static const Block* wood;
    static const Block* leaves;
    static const Block* sponge;
    static const Block* glass;
    static const Block* cloth;
    static const Block* flowerYellow;
    static const Block* flowerRed;
    static const Block* mushroomBrown;
    static const Block* mushroomRed;
    static const Block* blockGold;
    static const Block* blockSteel;
    static const Block* stairDouble;
    static const Block* stairSingle;
    static const Block* brick;
    static const Block* tnt;
    static const Block* bookshelf;
    static const Block* cobblestoneMossy;
    static const Block* obsidian;
    static const Block* torch;
    static const Block* fire;
    static const Block* mobSpawner;
    static const Block* stairCompactWood;
    static const Block* chest;
    static const Block* gear;
    static const Block* oreDiamond;
    static const Block* blockDiamond;
    static const Block* workbench;
    static const Block* crops;
    static const Block* farmland;
    static const Block* furnaceIdle;
    static const Block* furnaceActive;
    static const Block* signStanding;
    static const Block* doorWood;
    static const Block* ladder;
    static const Block* minecartTrack;
    static const Block* stairCompactStone;
    static const Block* signWall;

    static void init();
    static float getHardness(uint8_t blockID);

    Block(int id, int tex, const Material& mat);
    virtual ~Block() = default;

    virtual int getTexture(int side) const;
    virtual BlockRenderLayer getRenderLayer() const;
    virtual BlockRenderShape getRenderShape() const;
    virtual bool isFullCube() const;
    virtual bool isOccluder() const;
    virtual bool isGreedyMergeable() const;
    virtual bool isOpaqueCube() const;

    virtual void getCollisionBoxes(World& world, int x, int y, int z, const AxisAlignedBB& mask, std::vector<AxisAlignedBB>& list) const;
    virtual AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const;

    virtual void updateTick(World& world, int x, int y, int z, JavaRandom& random) const {}
    virtual void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {}
    virtual void onBlockAdded(World& world, int x, int y, int z) const {}
    virtual int tickRate() const { return 10; }
    virtual bool shouldSideBeRendered(const class IBlockAccess& world, int x, int y, int z, int side) const;
    virtual void velocityToAddToEntity(World& world, int x, int y, int z, Entity* entity, glm::vec3& velocity) const {}

    const int blockID;
    int blockIndexInTexture;
    const Material& blockMaterial;

protected:
    void setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1);

    double minX, minY, minZ;
    double maxX, maxY, maxZ;
};
