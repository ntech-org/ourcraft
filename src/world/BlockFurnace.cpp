#include "world/BlockFurnace.hpp"
#include "world/TileEntityFurnace.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

BlockFurnace::BlockFurnace(int id, bool active, const Material& mat)
    : Block(id, 1, mat), m_active(active) {
    blockIndexInTexture = 45;
}

int BlockFurnace::getTexture(int side) const {
    if (side == 0 || side == 1) return 1;
    if (side == 2) return m_active ? 61 : 44;
    return 45;
}

void BlockFurnace::onBlockAdded(World& world, int x, int y, int z) const {
    if (!world.getTileEntity(x, y, z)) {
        auto te = std::make_unique<TileEntityFurnace>();
        te->x = x; te->y = y; te->z = z;
        te->world = &world;
        world.addTileEntity(std::move(te));
    }
}

bool BlockFurnace::onBlockActivated(World& world, int x, int y, int z, EntityPlayer* player) const {
    if (player && player->isLocalPlayer) {
        player->openFurnace(world, x, y, z);
    }
    return true;
}

void BlockFurnace::updateFurnaceBlockState(bool burning, World& world, int x, int y, int z) {
    int currentID = world.getBlockID(x, y, z);
    int targetID = burning ? Block::furnaceActive->blockID : Block::furnaceIdle->blockID;
    if (currentID != targetID) {
        TileEntity* oldTE = world.getTileEntity(x, y, z);
        world.setBlockWithNotify(x, y, z, targetID);
        if (oldTE) {
            TileEntity* newTE = world.getTileEntity(x, y, z);
            if (newTE && newTE != oldTE) {
                auto* src = dynamic_cast<TileEntityFurnace*>(oldTE);
                auto* dst = dynamic_cast<TileEntityFurnace*>(newTE);
                if (src && dst) {
                    for (int i = 0; i < 3; ++i) dst->furnaceItemStacks[i] = src->furnaceItemStacks[i];
                    dst->furnaceBurnTime = src->furnaceBurnTime;
                    dst->currentItemBurnTime = src->currentItemBurnTime;
                    dst->furnaceCookTime = src->furnaceCookTime;
                }
            }
        }
    }
}
