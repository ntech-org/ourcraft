#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemHoe : public ItemTool {
public:
    ItemHoe(int id, int tier);

    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
