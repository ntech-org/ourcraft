#pragma once

#include "entities/InventoryPlayer.hpp"
#include <vector>
#include <memory>

struct Recipe {
    int width, height;
    std::vector<int> ingredients; // itemID, 0 for empty
    ItemStack result;
    bool shapeless = false;
};

class CraftingManager {
public:
    static CraftingManager& getInstance();

    void addRecipe(ItemStack result, int w, int h, const std::vector<int>& ingredients);
    void addShapelessRecipe(ItemStack result, const std::vector<int>& ingredients);

    ItemStack findMatchingRecipe(const ItemStack* grid, int gridW, int gridH);
    void consumeRecipe(ItemStack* grid, int gridW, int gridH);

private:
    CraftingManager();
    std::vector<Recipe> m_recipes;

    bool matches(const Recipe& recipe, const ItemStack* grid, int gridW, int gridH);
};
