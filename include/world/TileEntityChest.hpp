#pragma once
#include "world/TileEntity.hpp"
#include "entities/InventoryPlayer.hpp"

class TileEntityChest : public TileEntity {
public:
    TileEntityChest();
    int getBlockID() const override { return 54; }
    static constexpr int CHEST_SIZE = 27;

    ItemStack& getStackInSlot(int slot) { return chestContents[slot]; }
    const ItemStack& getStackInSlot(int slot) const { return chestContents[slot]; }
    void setInventorySlotContents(int slot, const ItemStack& stack);
    void markDirty();

    ItemStack chestContents[CHEST_SIZE];
};
