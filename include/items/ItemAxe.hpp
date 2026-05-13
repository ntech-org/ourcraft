#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemAxe : public ItemTool {
public:
    ItemAxe(int id, int tier);

    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
