#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemHoe : public ItemTool {
public:
    ItemHoe(int id, int tier);

    bool onItemUse(ItemStack& stack, EntityPlayer& player, World& world, int x, int y, int z, int side) override;
    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
