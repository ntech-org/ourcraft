#include "world/TileEntityFurnace.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "world/BlockFurnace.hpp"
#include "world/Material.hpp"
#include "items/Item.hpp"
#include <algorithm>

TileEntityFurnace::TileEntityFurnace() {
    for (auto& s : furnaceItemStacks) s = {0, 0, 0};
}

void TileEntityFurnace::updateEntity() {
    if (!world) return;
    bool wasBurning = furnaceBurnTime > 0;
    bool dirty = false;

    if (furnaceBurnTime > 0) {
        --furnaceBurnTime;
    }

    if (furnaceBurnTime == 0 && canSmelt()) {
        currentItemBurnTime = furnaceBurnTime = getItemBurnTime(furnaceItemStacks[1]);
        if (furnaceBurnTime > 0) {
            dirty = true;
            if (furnaceItemStacks[1].count > 0) {
                --furnaceItemStacks[1].count;
                if (furnaceItemStacks[1].count <= 0) {
                    furnaceItemStacks[1] = {0, 0, 0};
                }
            }
        }
    }

    if (isBurning() && canSmelt()) {
        ++furnaceCookTime;
        if (furnaceCookTime >= 200) {
            furnaceCookTime = 0;
            smeltItem();
            dirty = true;
        }
    } else {
        furnaceCookTime = 0;
    }

    if (wasBurning != (furnaceBurnTime > 0)) {
        dirty = true;
        if (world && !world->isRemote) {
            BlockFurnace::updateFurnaceBlockState(furnaceBurnTime > 0, *world, x, y, z);
        }
    }
}

int TileEntityFurnace::getCookProgressScaled(int scale) const {
    return furnaceCookTime * scale / 200;
}

int TileEntityFurnace::getBurnTimeRemainingScaled(int scale) const {
    int burnTime = currentItemBurnTime;
    if (burnTime == 0) burnTime = 200;
    return furnaceBurnTime * scale / burnTime;
}

bool TileEntityFurnace::canSmelt() const {
    if (furnaceItemStacks[0].isEmpty()) return false;
    int resultID = getSmeltingResult(furnaceItemStacks[0].itemID);
    if (resultID < 0) return false;
    if (furnaceItemStacks[2].isEmpty()) return true;
    if (furnaceItemStacks[2].itemID != resultID) return false;
    if (furnaceItemStacks[2].count >= 64) return false;
    return true;
}

void TileEntityFurnace::smeltItem() {
    if (!canSmelt()) return;
    int resultID = getSmeltingResult(furnaceItemStacks[0].itemID);
    if (resultID < 0) return;

    if (furnaceItemStacks[2].isEmpty()) {
        furnaceItemStacks[2] = {resultID, 1, 0};
    } else if (furnaceItemStacks[2].itemID == resultID) {
        ++furnaceItemStacks[2].count;
    }

    --furnaceItemStacks[0].count;
    if (furnaceItemStacks[0].count <= 0) {
        furnaceItemStacks[0] = {0, 0, 0};
    }
}

int TileEntityFurnace::getSmeltingResult(int itemID) {
    if (itemID == Block::oreIron->blockID) return Item::ingotIron->itemID;
    if (itemID == Block::oreGold->blockID) return Item::ingotGold->itemID;
    if (itemID == Block::oreDiamond->blockID) return Item::diamond->itemID;
    if (itemID == Block::sand->blockID) return Block::glass->blockID;
    if (itemID == Block::cobblestone->blockID) return Block::stone->blockID;
    if (itemID == Item::porkRaw->itemID) return Item::porkCooked->itemID;
    return -1;
}

int TileEntityFurnace::getItemBurnTime(const ItemStack& stack) {
    if (stack.isEmpty()) return 0;
    int id = stack.itemID;
    if (id < 256) {
        const Block* block = Block::blocksList[id];
        if (block && block->blockMaterial == Material::wood) return 300;
    }
    if (id == Item::stick->itemID) return 100;
    if (id == Item::coal->itemID) return 1600;
    return 0;
}
