#pragma once
#include "world/Block.hpp"
#include "world/JavaRandom.hpp"

class BlockCrops : public Block {
public:
    BlockCrops(int id, int tex);
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    bool isSameTypeCulled() const override { return false; }
    BlockRenderLayer getRenderLayer() const override { return BlockRenderLayer::Cutout; }
    BlockRenderShape getRenderShape() const override { return BlockRenderShape::Cross; }
    AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const override { return AxisAlignedBB(0, 0, 0, 0, 0, 0); }
    int getTexture(int side) const override;
    int getTexture(int side, int metadata) const override;
    bool canThisPlantGrowOnThisBlockID(int id) const;
    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    void onBlockDestroyedByPlayer(World& world, int x, int y, int z, int metadata) const override;
    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;
    int idDropped(int metadata) const override;
private:
    void dropSeeds(World& world, int x, int y, int z, int meta) const;
};
