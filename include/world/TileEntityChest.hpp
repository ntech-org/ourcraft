#pragma once
#include "world/TileEntity.hpp"
#include "entities/InventoryPlayer.hpp"

class TileEntityChest : public TileEntity {
public:
    TileEntityChest() = default;
    int getBlockID() const override { return 54; }
    InventoryPlayer inventory;
    static constexpr int CHEST_SIZE = 27;
};
