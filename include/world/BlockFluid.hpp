#pragma once

#include "world/Block.hpp"
#include "physics/AxisAlignedBB.hpp"

class BlockFluid : public Block {
public:
    BlockFluid(int id, const Material& mat);

    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;
    
    int getTexture(int side) const override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    AxisAlignedBB getCollisionBoundingBoxFromPool(World& world, int x, int y, int z) const override { return AxisAlignedBB(0,0,0,0,0,0); }
    int tickRate() const override;
    BlockRenderLayer getRenderLayer() const override;
    bool shouldSideBeRendered(const IBlockAccess& world, int x, int y, int z, int side) const override;
    void velocityToAddToEntity(World& world, int x, int y, int z, Entity* entity, glm::vec3& velocity) const override;

    static float getPercentAir(int meta);
    static double getFlowDirection(const IBlockAccess& world, int x, int y, int z, const Material& mat);

protected:
    void checkForHarden(World& world, int x, int y, int z) const;
    void triggerLavaMixEffects(World& world, int x, int y, int z) const;
    int getFlowDecay(World& world, int x, int y, int z) const;
    int getEffectiveFlowDecay(const IBlockAccess& world, int x, int y, int z) const;
    glm::vec3 getFlowVector(const IBlockAccess& world, int x, int y, int z) const;
};

class BlockFlowing : public BlockFluid {
public:
    BlockFlowing(int id, const Material& mat);

    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;

private:
    void flowIntoBlock(World& world, int x, int y, int z, int meta) const;
    int calculateFlowCost(World& world, int x, int y, int z, int distance, int side) const;
    std::vector<bool> getOptimalFlowDirections(World& world, int x, int y, int z) const;
    bool blockBlocksFlow(World& world, int x, int y, int z) const;
    int getSmallestFlowDecay(World& world, int x, int y, int z, int currentSmallest, int& numAdjacentSources) const;
    bool liquidCanDisplaceBlock(World& world, int x, int y, int z) const;
};

class BlockStationary : public BlockFluid {
public:
    BlockStationary(int id, const Material& mat);

    void onBlockAdded(World& world, int x, int y, int z) const override;
    void onNeighborBlockChange(World& world, int x, int y, int z, int neighborID) const override;
    void updateTick(World& world, int x, int y, int z, JavaRandom& random) const override;
};
