#pragma once

#include <vector>
#include <cstdint>

struct ItemStack {
    int itemID = 0;
    int count = 0;
};

class InventoryPlayer {
public:
    InventoryPlayer();

    void addItem(int itemID, int count);
    int getCurrentItemID() const;
    void nextSlot();
    void prevSlot();
    void setSlot(int slot);

    ItemStack mainInventory[36];
    int currentSlot = 0;
};
