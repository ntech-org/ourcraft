#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemPickaxe : public ItemTool {
public:
    ItemPickaxe(int id, int tier);

    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
