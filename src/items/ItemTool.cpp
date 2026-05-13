#include "items/ItemTool.hpp"
#include "world/Block.hpp"

ItemTool::ItemTool(int id, int tier, int maxDamage, float efficiency)
    : Item(id), tier(tier), maxDamage(maxDamage), efficiencyOnProperMaterial(efficiency) {}

float ItemTool::getStrVsBlock(const Block& block) const {
    return 1.0f;
}

bool ItemTool::canHarvestBlock(const Block& block) const {
    return false;
}
