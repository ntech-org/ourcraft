#pragma once

#include "world/TileEntity.hpp"
#include "entities/InventoryPlayer.hpp"

class TileEntityFurnace : public TileEntity {
public:
    TileEntityFurnace();

    void updateEntity() override;
    int getBlockID() const override { return 62; }

    ItemStack furnaceItemStacks[3];

    int furnaceBurnTime = 0;
    int currentItemBurnTime = 0;
    int furnaceCookTime = 0;

    bool isBurning() const { return furnaceBurnTime > 0; }
    int getCookProgressScaled(int scale) const;
    int getBurnTimeRemainingScaled(int scale) const;

    bool canSmelt() const;
    void smeltItem();

    static int getSmeltingResult(int itemID);
    static int getItemBurnTime(const ItemStack& stack);
};
