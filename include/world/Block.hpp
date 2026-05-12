#pragma once

#include "world/Material.hpp"
#include "sound/StepSound.hpp"
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
class EntityPlayer;

class Block {
public:
    static Block* blocksList[256];
    static bool opaqueCubeLookup[256];
    static int lightOpacity[256];
    static int lightValue[256];
    static float blockHardness[256];

    static Block* stone;
    static Block* grass;
    static Block* dirt;
    static Block* cobblestone;
    static Block* planks;
    static Block* sapling;
    static Block* bedrock;
    static Block* waterMoving;
    static Block* waterStill;
    static Block* lavaMoving;
    static Block* lavaStill;
    static Block* sand;
    static Block* gravel;
    static Block* oreGold;
    static Block* oreIron;
    static Block* oreCoal;
    static Block* wood;
    static Block* leaves;
    static Block* sponge;
    static Block* glass;
    static Block* cloth;
    static Block* flowerYellow;
    static Block* flowerRed;
    static Block* mushroomBrown;
    static Block* mushroomRed;
    static Block* blockGold;
    static Block* blockSteel;
    static Block* stairDouble;
    static Block* stairSingle;
    static Block* brick;
    static Block* tnt;
    static Block* bookshelf;
    static Block* cobblestoneMossy;
    static Block* obsidian;
    static Block* torch;
    static Block* fire;
    static Block* mobSpawner;
    static Block* stairCompactWood;
    static Block* chest;
    static Block* gear;
    static Block* oreDiamond;
    static Block* blockDiamond;
    static Block* workbench;
    static Block* crops;
    static Block* farmland;
    static Block* furnaceIdle;
    static Block* furnaceActive;
    static Block* signStanding;
    static Block* doorWood;
    static Block* ladder;
    static Block* minecartTrack;
    static Block* stairCompactStone;
    static Block* signWall;

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
    virtual bool isSameTypeCulled() const;

    virtual void getCollisionBoxes(World& world, int x, int y, int z, const AxisAlignedBB& mask, std::vector<AxisAlignedBB>& list) const;
    virtual AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const;

    virtual void updateTick(World& world, int x, int y, int z, JavaRandom& random) const {}
    virtual void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const {}
    virtual void onBlockAdded(World& world, int x, int y, int z) const {}
    virtual bool onBlockActivated(World& world, int x, int y, int z, EntityPlayer* player) const { return false; }
    virtual int tickRate() const { return 10; }
    virtual bool shouldSideBeRendered(const class IBlockAccess& world, int x, int y, int z, int side) const;
    virtual void velocityToAddToEntity(World& world, int x, int y, int z, Entity* entity, glm::vec3& velocity) const {}

    const int blockID;
    int blockIndexInTexture;
    const Material& blockMaterial;
    const StepSound* stepSound = &SOUND_STONE;

protected:
    void setBlockBounds(float x0, float y0, float z0, float x1, float y1, float z1);

    double minX, minY, minZ;
    double maxX, maxY, maxZ;
};
