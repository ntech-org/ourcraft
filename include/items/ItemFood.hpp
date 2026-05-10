#pragma once

#include "items/Item.hpp"
#include "entities/EntityPlayer.hpp"

class ItemFood : public Item {
public:
    ItemFood(int id, int healAmount) : Item(id), m_healAmount(healAmount) {
        maxStackSize = 1; // Infdev food is non-stackable
    }

    ItemStack onItemRightClick(ItemStack stack, World& world, EntityPlayer& player) override {
        if (player.health < player.maxHealth) {
            player.health = std::min(player.maxHealth, player.health + m_healAmount);
            stack.count--;
            if (stack.count <= 0) stack = {0, 0, 0};
        }
        return stack;
    }

private:
    int m_healAmount;
};
