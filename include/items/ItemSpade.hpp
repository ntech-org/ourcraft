#pragma once

#include "items/ItemTool.hpp"

class Block;

class ItemSpade : public ItemTool {
public:
    ItemSpade(int id, int tier);

    float getStrVsBlock(const Block& block) const override;
    bool canHarvestBlock(const Block& block) const override;
};
