#include "inventory/CraftingManager.hpp"
#include <algorithm>
#include <numeric>

CraftingManager& CraftingManager::getInstance() {
    static CraftingManager instance;
    return instance;
}

CraftingManager::CraftingManager() {
    // Basic materials
    addRecipe({5, 4}, 1, 1, {17});                    // Log -> 4 Planks
    addRecipe({280, 4}, 1, 2, {5, 5});                // 2 Planks -> 4 Sticks
    addRecipe({50, 4}, 1, 2, {263, 280});             // Coal + Stick -> 4 Torches
    addRecipe({281, 4}, 3, 1, {5, 5, 5});             // 3 Planks -> 4 Bowls

    // Building blocks
    addRecipe({58, 1}, 2, 2, {5, 5, 5, 5});           // 4 Planks -> Crafting Table
    addRecipe({61, 1}, 3, 3, {4, 4, 4, 4, 0, 4, 4, 4, 4}); // 8 Cobblestone -> Furnace
    addRecipe({54, 1}, 3, 3, {5, 5, 5, 5, 0, 5, 5, 5, 5}); // 8 Planks -> Chest
    addRecipe({64, 1}, 3, 3, {5, 5, 5, 5, 5, 5, 0, 0, 0}); // 6 Planks -> Door
    addRecipe({323, 1}, 2, 3, {5, 5, 5, 5, 5, 280}); // 6 Planks + Stick -> Sign
    addRecipe({65, 3}, 3, 3, {0, 280, 0, 280, 280, 280, 280, 0, 280}); // 7 Sticks -> 3 Ladder
    addRecipe({44, 3}, 1, 3, {4, 4, 4});              // 3 Cobblestone -> 3 Stone Slab
    addRecipe({321, 1}, 3, 3, {280, 280, 280, 280, 287, 280, 280, 280, 280}); // 8 Sticks + Cloth -> Painting
    addRecipe({53, 4}, 3, 3, {5, 0, 0, 5, 5, 0, 5, 5, 5}); // 6 Planks -> 4 Wood Stairs
    addRecipe({67, 4}, 3, 3, {4, 0, 0, 4, 4, 0, 4, 4, 4}); // 6 Cobblestone -> 4 Stone Stairs

    // Tools - Pickaxes
    addRecipe({270, 1}, 3, 3, {5, 5, 5, 0, 280, 0, 0, 280, 0}); // Wooden Pickaxe
    addRecipe({274, 1}, 3, 3, {4, 4, 4, 0, 280, 0, 0, 280, 0}); // Stone Pickaxe
    addRecipe({257, 1}, 3, 3, {265, 265, 265, 0, 280, 0, 0, 280, 0}); // Iron Pickaxe
    addRecipe({278, 1}, 3, 3, {264, 264, 264, 0, 280, 0, 0, 280, 0}); // Diamond Pickaxe
    addRecipe({285, 1}, 3, 3, {266, 266, 266, 0, 280, 0, 0, 280, 0}); // Gold Pickaxe

    // Tools - Axes
    addRecipe({271, 1}, 3, 3, {5, 5, 0, 5, 280, 0, 0, 280, 0}); // Wooden Axe
    addRecipe({275, 1}, 3, 3, {4, 4, 0, 4, 280, 0, 0, 280, 0}); // Stone Axe
    addRecipe({258, 1}, 3, 3, {265, 265, 0, 265, 280, 0, 0, 280, 0}); // Iron Axe
    addRecipe({279, 1}, 3, 3, {264, 264, 0, 264, 280, 0, 0, 280, 0}); // Diamond Axe
    addRecipe({286, 1}, 3, 3, {266, 266, 0, 266, 280, 0, 0, 280, 0}); // Gold Axe

    // Tools - Shovels
    addRecipe({269, 1}, 3, 3, {0, 5, 0, 0, 280, 0, 0, 280, 0}); // Wooden Shovel
    addRecipe({273, 1}, 3, 3, {0, 4, 0, 0, 280, 0, 0, 280, 0}); // Stone Shovel
    addRecipe({256, 1}, 3, 3, {0, 265, 0, 0, 280, 0, 0, 280, 0}); // Iron Shovel
    addRecipe({277, 1}, 3, 3, {0, 264, 0, 0, 280, 0, 0, 280, 0}); // Diamond Shovel
    addRecipe({284, 1}, 3, 3, {0, 266, 0, 0, 280, 0, 0, 280, 0}); // Gold Shovel

    // Tools - Hoes
    addRecipe({290, 1}, 3, 3, {5, 5, 0, 0, 280, 0, 0, 280, 0}); // Wooden Hoe
    addRecipe({291, 1}, 3, 3, {4, 4, 0, 0, 280, 0, 0, 280, 0}); // Stone Hoe
    addRecipe({292, 1}, 3, 3, {265, 265, 0, 0, 280, 0, 0, 280, 0}); // Iron Hoe
    addRecipe({293, 1}, 3, 3, {264, 264, 0, 0, 280, 0, 0, 280, 0}); // Diamond Hoe
    addRecipe({294, 1}, 3, 3, {266, 266, 0, 0, 280, 0, 0, 280, 0}); // Gold Hoe

    // Weapons - Swords
    addRecipe({268, 1}, 3, 3, {0, 5, 0, 0, 5, 0, 0, 280, 0}); // Wooden Sword
    addRecipe({272, 1}, 3, 3, {0, 4, 0, 0, 4, 0, 0, 280, 0}); // Stone Sword
    addRecipe({267, 1}, 3, 3, {0, 265, 0, 0, 265, 0, 0, 280, 0}); // Iron Sword
    addRecipe({276, 1}, 3, 3, {0, 264, 0, 0, 264, 0, 0, 280, 0}); // Diamond Sword
    addRecipe({283, 1}, 3, 3, {0, 266, 0, 0, 266, 0, 0, 280, 0}); // Gold Sword

    // Weapons - Bow & Arrow
    addRecipe({261, 1}, 3, 3, {0, 287, 280, 287, 0, 280, 0, 287, 280}); // Bow
    addRecipe({262, 4}, 3, 3, {0, 318, 0, 0, 280, 0, 0, 288, 0}); // 4 Arrows

    // Armor - Leather
    addRecipe({330, 1}, 3, 3, {287, 287, 287, 287, 0, 287, 0, 0, 0});   // Cap
    addRecipe({331, 1}, 3, 3, {287, 0, 287, 287, 287, 287, 287, 287, 287}); // Tunic
    addRecipe({332, 1}, 3, 3, {287, 287, 287, 287, 0, 287, 287, 0, 287}); // Pants
    addRecipe({333, 1}, 3, 3, {287, 0, 287, 287, 0, 287, 0, 0, 0});   // Boots

    // Armor - Iron
    addRecipe({338, 1}, 3, 3, {265, 265, 265, 265, 0, 265, 0, 0, 0}); // Helmet
    addRecipe({339, 1}, 3, 3, {265, 0, 265, 265, 265, 265, 265, 265, 265}); // Chestplate
    addRecipe({340, 1}, 3, 3, {265, 265, 265, 265, 0, 265, 265, 0, 265}); // Leggings
    addRecipe({341, 1}, 3, 3, {265, 0, 265, 265, 0, 265, 0, 0, 0}); // Boots

    // Armor - Diamond
    addRecipe({342, 1}, 3, 3, {264, 264, 264, 264, 0, 264, 0, 0, 0}); // Helmet
    addRecipe({343, 1}, 3, 3, {264, 0, 264, 264, 264, 264, 264, 264, 264}); // Chestplate
    addRecipe({344, 1}, 3, 3, {264, 264, 264, 264, 0, 264, 264, 0, 264}); // Leggings
    addRecipe({345, 1}, 3, 3, {264, 0, 264, 264, 0, 264, 0, 0, 0}); // Boots

    // Armor - Gold
    addRecipe({346, 1}, 3, 3, {266, 266, 266, 266, 0, 266, 0, 0, 0}); // Helmet
    addRecipe({347, 1}, 3, 3, {266, 0, 266, 266, 266, 266, 266, 266, 266}); // Chestplate
    addRecipe({348, 1}, 3, 3, {266, 266, 266, 266, 0, 266, 266, 0, 266}); // Leggings
    addRecipe({349, 1}, 3, 3, {266, 0, 266, 266, 0, 266, 0, 0, 0}); // Boots

    // Food
    addShapelessRecipe({282, 1}, {281, 39, 40});  // Bowl + Brown Mushroom + Red Mushroom -> Mushroom Stew
    addShapelessRecipe({282, 1}, {281, 40, 39});  // Other ordering
    addRecipe({297, 1}, 3, 1, {296, 296, 296});   // 3 Wheat -> Bread

    // Block conversions
    addRecipe({41, 1}, 3, 3, {266, 266, 266, 266, 266, 266, 266, 266, 266}); // 9 Gold Ingots -> Gold Block
    addShapelessRecipe({266, 9}, {41});            // Gold Block -> 9 Gold Ingots (shapeless)
    addRecipe({42, 1}, 3, 3, {265, 265, 265, 265, 265, 265, 265, 265, 265}); // 9 Iron Ingots -> Iron Block
    addShapelessRecipe({265, 9}, {42});            // Iron Block -> 9 Iron Ingots
    addRecipe({57, 1}, 3, 3, {264, 264, 264, 264, 264, 264, 264, 264, 264}); // 9 Diamonds -> Diamond Block
    addShapelessRecipe({264, 9}, {57});            // Diamond Block -> 9 Diamonds

    // Blocks from items
    addRecipe({46, 1}, 3, 3, {289, 12, 289, 12, 0, 12, 289, 12, 289}); // TNT (gunpowder + sand)

    // Utils
    addRecipe({259, 1}, 1, 2, {265, 318});           // Iron Ingot + Flint -> Flint & Steel
    addRecipe({325, 1}, 3, 3, {0, 265, 0, 265, 0, 265, 0, 265, 0}); // 3 Iron Ingots -> Bucket
    addRecipe({328, 1}, 3, 3, {265, 0, 265, 265, 265, 265, 0, 0, 0}); // 5 Iron Ingots -> Minecart
    addRecipe({66, 16}, 3, 3, {265, 0, 265, 265, 280, 265, 265, 0, 265}); // 6 Iron + Stick -> 16 Tracks
    addRecipe({322, 1}, 3, 3, {266, 266, 266, 266, 260, 266, 266, 266, 266}); // 8 Gold Blocks + Apple -> Golden Apple

    // Sort recipes by size (largest/most specific first)
    std::sort(m_recipes.begin(), m_recipes.end(), [](const Recipe& a, const Recipe& b) {
        int sizeA = a.shapeless ? 99 : a.width + a.height;
        int sizeB = b.shapeless ? 99 : b.width + b.height;
        if (sizeA != sizeB) return sizeA > sizeB;
        return false;
    });
}

void CraftingManager::addRecipe(ItemStack result, int w, int h, const std::vector<int>& ingredients) {
    Recipe r;
    r.result = result;
    r.width = w;
    r.height = h;
    r.ingredients = ingredients;
    r.shapeless = false;
    m_recipes.push_back(r);
}

void CraftingManager::addShapelessRecipe(ItemStack result, const std::vector<int>& ingredients) {
    Recipe r;
    r.result = result;
    r.width = 0;
    r.height = 0;
    r.ingredients = ingredients;
    r.shapeless = true;
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

static bool isGridEmptyExcept(const ItemStack* grid, int gridW, int gridH,
                               int ox, int oy, int rw, int rh) {
    for (int gx = 0; gx < gridW; ++gx) {
        for (int gy = 0; gy < gridH; ++gy) {
            int rx = gx - ox;
            int ry = gy - oy;
            if (rx < 0 || ry < 0 || rx >= rw || ry >= rh) {
                if (!grid[gy * gridW + gx].isEmpty()) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool CraftingManager::matches(const Recipe& recipe, const ItemStack* grid, int gridW, int gridH) {
    if (recipe.shapeless) {
        // Count ingredients needed and items available
        std::vector<int> needed = recipe.ingredients;
        std::vector<int> available;
        for (int i = 0; i < gridW * gridH; ++i) {
            if (!grid[i].isEmpty()) {
                available.push_back(grid[i].itemID);
            }
        }
        if (needed.size() != available.size()) return false;

        std::sort(needed.begin(), needed.end());
        std::sort(available.begin(), available.end());
        return needed == available;
    }

    for (int ox = 0; ox <= gridW - recipe.width; ++ox) {
        for (int oy = 0; oy <= gridH - recipe.height; ++oy) {
            bool match = true;

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

            if (match && isGridEmptyExcept(grid, gridW, gridH, ox, oy, recipe.width, recipe.height)) {
                return true;
            }
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
