#include "world/TileEntityChest.hpp"
#include "world/World.hpp"
#include "world/storage/SaveHandler.hpp"

TileEntityChest::TileEntityChest() {
    for (auto& stack : chestContents) stack = {};
}

void TileEntityChest::setInventorySlotContents(int slot, const ItemStack& stack) {
    if (slot < 0 || slot >= CHEST_SIZE) return;
    chestContents[slot] = stack;
    if (chestContents[slot].count > 64) chestContents[slot].count = 64;
    markDirty();
}

void TileEntityChest::markDirty() {
    if (world && !world->isRemote && world->getSaveHandler()) {
        world->getSaveHandler()->saveChest(x, y, z, chestContents, CHEST_SIZE);
    }
}
