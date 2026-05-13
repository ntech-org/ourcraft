#pragma once

#include <cstdlib>
#include "world/Block.hpp"
#include "world/BlockFluid.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

class BlockStone : public Block {
public:
    BlockStone(int id) : Block(id, 1, Material::rock) {}
    int idDropped(int metadata) const override { return 4; }
};

class BlockGrass : public Block {
public:
    BlockGrass(int id) : Block(id, 3, Material::ground) {}
    int getTexture(int side) const override {
        if (side == 1) return 0;
        if (side == 0) return 2;
        return 3;
    }
    int idDropped(int metadata) const override { return 3; }
};

class BlockGravel : public Block {
public:
    BlockGravel(int id) : Block(id, 19, Material::sand) {}
    int idDropped(int metadata) const override {
        if (std::rand() % 10 == 0) return 318;
        return 13;
    }
};

class BlockFarmland : public Block {
public:
    BlockFarmland(int id) : Block(id, 87, Material::ground) {}
    int idDropped(int metadata) const override { return 3; }
};

class BlockOre : public Block {
public:
    BlockOre(int id, int tex) : Block(id, tex, Material::rock) {}
    int idDropped(int metadata) const override {
        if (blockID == 16) return 263;
        if (blockID == 56) return 264;
        return blockID;
    }
};

class BlockLog : public Block {
public:
    BlockLog(int id) : Block(id, 20, Material::wood) {}
    int getTexture(int side) const override {
        if (side == 1 || side == 0) return 21;
        return 20;
    }
};

class BlockLeaves : public Block {
public:
    BlockLeaves(int id) : Block(id, 52, Material::leaves) {}
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    int idDropped(int metadata) const override { return 0; }
};

class BlockGlass : public Block {
public:
    BlockGlass(int id) : Block(id, 49, Material::glass) {}
    bool isOpaqueCube() const override { return false; }
    bool isSameTypeCulled() const override { return true; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
};

class BlockWorkbench : public Block {
public:
    BlockWorkbench(int id) : Block(id, 43, Material::wood) {}
    int getTexture(int side) const override {
        if (side == 1) return 43;
        if (side == 0) return 4;
        if (side == 2 || side == 3) return 59;
        return 60;
    }
    bool onBlockActivated(World& world, int x, int y, int z, EntityPlayer* player) const override {
        if (world.isRemote) {
            player->openCraftingTable();
        }
        return true;
    }
};

class BlockCross : public Block {
public:
    BlockCross(int id, int tex) : Block(id, tex, Material::plants) {
        setBlockBounds(0.1f, 0.0f, 0.1f, 0.9f, 0.8f, 0.9f);
    }
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    BlockRenderShape getRenderShape() const override { return BlockRenderShape::Cross; }
    AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const override { return AxisAlignedBB(0,0,0,0,0,0); }
};
