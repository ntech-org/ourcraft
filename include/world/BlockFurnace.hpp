#pragma once

#include "world/Block.hpp"

class TileEntityFurnace;

class BlockFurnace : public Block {
public:
    BlockFurnace(int id, bool active, const Material& mat);

    bool onBlockActivated(World& world, int x, int y, int z, EntityPlayer* player) const override;
    void onBlockAdded(World& world, int x, int y, int z) const override;
    int getTexture(int side) const override;

    static void updateFurnaceBlockState(bool burning, World& world, int x, int y, int z);

private:
    bool m_active;
};
