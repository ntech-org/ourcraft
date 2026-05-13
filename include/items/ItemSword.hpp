#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemSword : public ItemTool {
public:
    ItemSword(int id, int tier);

    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
