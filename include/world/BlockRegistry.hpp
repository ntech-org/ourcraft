#pragma once

#include "world/Block.hpp"
#include "world/BlockCrops.hpp"
#include "world/BlockFluid.hpp"
#include "world/TileEntityChest.hpp"
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
    BlockFarmland(int id) : Block(id, 87, Material::ground) {
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 15.0f / 16.0f, 1.0f);
    }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool isOccluder() const override { return false; }
    bool isSameTypeCulled() const override { return true; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }

    int getTexture(int side) const override {
        if (side == 1) return blockIndexInTexture;
        return 2;
    }
    int getTexture(int side, int meta) const override {
        if (side == 1 && meta > 0) return blockIndexInTexture - 1;
        if (side == 1) return blockIndexInTexture;
        return 2;
    }
    int idDropped(int metadata) const override { return Block::dirt ? Block::dirt->blockID : blockID; }

    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;
    void onEntityWalking(World& world, int x, int y, int z, Entity* entity) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    void onBlockAdded(World& world, int x, int y, int z) const override;

    bool isWaterNearby(World& world, int x, int y, int z) const;
    bool isCropsNearby(World& world, int x, int y, int z) const;
};

class BlockSapling : public Block {
public:
    BlockSapling(int id, int tex) : Block(id, tex, Material::plants) {
        setBlockBounds(0.1f, 0.0f, 0.1f, 0.9f, 0.8f, 0.9f);
    }
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    BlockRenderShape getRenderShape() const override { return BlockRenderShape::Cross; }
    AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const override {
        return AxisAlignedBB(0, 0, 0, 0, 0, 0);
    }

    int getTexture(int side) const override { return blockIndexInTexture; }

    bool canPlaceBlockAt(World& world, int x, int y, int z) const override;
    bool canBlockStay(World& world, int x, int y, int z) const;
    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;
    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
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

class BlockTorch : public Block {
public:
    BlockTorch(int id, int tex) : Block(id, tex, Material::circuits) {
        setBlockBounds(0.5f - 0.15f, 0.2f, 0.5f - 0.15f, 0.5f + 0.15f, 0.8f, 0.5f + 0.15f);
    }
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    BlockRenderShape getRenderShape() const override { return BlockRenderShape::Special; }

    AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const override {
        return AxisAlignedBB(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    AxisAlignedBB getBlockBounds(const IBlockAccess& world, int x, int y, int z) const override {
        int meta = world.getBlockMetadata(x, y, z);
        if (meta == 1) {
            return AxisAlignedBB(0.0, 0.2, 0.35, 0.3, 0.8, 0.65);
        }
        if (meta == 2) return AxisAlignedBB(0.7, 0.2, 0.35, 1.0, 0.8, 0.65);
        if (meta == 3) return AxisAlignedBB(0.35, 0.2, 0.0, 0.65, 0.8, 0.3);
        if (meta == 4) return AxisAlignedBB(0.35, 0.2, 0.7, 0.65, 0.8, 1.0);
        return AxisAlignedBB(0.4, 0.0, 0.4, 0.6, 0.6, 0.6);
    }
    bool canPlaceBlockAt(World& world, int x, int y, int z) const override;
    void onBlockPlaced(World& world, int x, int y, int z, int side, float hitX, float hitY, float hitZ) const override;
    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    int idDropped(int metadata) const override { return blockID; }
};

class BlockChest : public Block {
public:
    BlockChest(int id) : Block(id, 26, Material::wood) {
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
    }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    int getTexture(int side) const override { return side == 0 || side == 1 ? 25 : 26; }
    bool canPlaceBlockAt(World& world, int x, int y, int z) const override;
    bool onBlockActivated(World& world, int x, int y, int z, EntityPlayer* player) const override { return true; }
    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onBlockRemoval(World& world, int x, int y, int z, int metadata) const override;
};
