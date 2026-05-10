#include "inventory/CraftingManager.hpp"
#include <algorithm>

CraftingManager& CraftingManager::getInstance() {
    static CraftingManager instance;
    return instance;
}

CraftingManager::CraftingManager() {
    // Basic Infdev recipes
    // Wood -> Planks
    addRecipe({5, 4}, 1, 1, {17}); 
    // Planks -> Sticks
    addRecipe({280, 4}, 1, 2, {5, 5});
    // Sticks + Coal -> Torches
    addRecipe({50, 4}, 1, 2, {263, 280});
    // Planks -> Crafting Table
    addRecipe({58, 1}, 2, 2, {5, 5, 5, 5});
    // Cobblestone -> Furnace
    addRecipe({61, 1}, 3, 3, {4, 4, 4, 4, 0, 4, 4, 4, 4});
    // Wooden Pickaxe
    addRecipe({270, 1}, 3, 3, {5, 5, 5, 0, 280, 0, 0, 280, 0});
    // Stone Pickaxe
    addRecipe({274, 1}, 3, 3, {4, 4, 4, 0, 280, 0, 0, 280, 0});
}

void CraftingManager::addRecipe(ItemStack result, int w, int h, const std::vector<int>& ingredients) {
    Recipe r;
    r.result = result;
    r.width = w;
    r.height = h;
    r.ingredients = ingredients;
    m_recipes.push_back(r);
}

ItemStack CraftingManager::findMatchingRecipe(const ItemStack* grid, int gridW, int gridH) {
    for (const auto& recipe : m_recipes) {
        if (matches(recipe, grid, gridW, gridH)) {
            return recipe.result;
        }
    }
    return {0, 0, 0};
}

bool CraftingManager::matches(const Recipe& recipe, const ItemStack* grid, int gridW, int gridH) {
    for (int ox = 0; ox <= gridW - recipe.width; ++ox) {
        for (int oy = 0; oy <= gridH - recipe.height; ++oy) {
            bool match = true;
            
            // Check if recipe fits at (ox, oy)
            for (int rx = 0; rx < recipe.width; ++rx) {
                for (int ry = 0; ry < recipe.height; ++ry) {
                    int gridIdx = (oy + ry) * gridW + (ox + rx);
                    int recipeIdx = ry * recipe.width + rx;
                    if (grid[gridIdx].itemID != recipe.ingredients[recipeIdx]) {
                        match = false;
                        break;
                    }
                }
                if (!match) break;
            }

            if (match) {
                // Check if the rest of the grid is empty
                for (int gx = 0; gx < gridW; ++gx) {
                    for (int gy = 0; gy < gridH; ++gy) {
                        int rx = gx - ox;
                        int ry = gy - oy;
                        if (rx < 0 || ry < 0 || rx >= recipe.width || ry >= recipe.height) {
                            if (!grid[gy * gridW + gx].isEmpty()) {
                                match = false;
                                break;
                            }
                        }
                    }
                    if (!match) break;
                }
            }

            if (match) return true;
        }
    }
    return false;
}

void CraftingManager::consumeRecipe(ItemStack* grid, int gridW, int gridH) {
    for (int i = 0; i < gridW * gridH; ++i) {
        if (!grid[i].isEmpty()) {
            grid[i].count--;
            if (grid[i].count <= 0) grid[i] = {0, 0, 0};
        }
    }
}
