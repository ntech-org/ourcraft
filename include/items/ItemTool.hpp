#pragma once

#include "items/Item.hpp"

class Block;

class ItemTool : public Item {
public:
    ItemTool(int id, int tier, int maxDamage, float efficiency);

    virtual float getStrVsBlock(const Block& block) const;
    virtual bool canHarvestBlock(const Block& block) const;

    int tier;
    int maxDamage;
    float efficiencyOnProperMaterial;
};
