#pragma once

#include <vector>
#include <cstdint>

struct ItemStack {
    int itemID = 0;
    int count = 0;
    uint8_t metadata = 0;

    bool isEmpty() const {
        return itemID == 0 || count <= 0;
    }
};

class InventoryPlayer {
public:
    static constexpr int HOTBAR_SIZE = 9;
    static constexpr int MAIN_SIZE = 27;
    static constexpr int INVENTORY_SIZE = 36;
    
    static constexpr int ARMOR_START = 36;
    static constexpr int CRAFT_START = 40;
    static constexpr int RESULT_SLOT = 44;

    static constexpr int WORKBENCH_START = 45;
    static constexpr int WORKBENCH_RESULT = 54;
    static constexpr int TOTAL_SIZE = 55;

    static constexpr int MAX_STACK_SIZE = 64;

    InventoryPlayer();

    bool addItem(int itemID, int count, uint8_t metadata = 0);
    bool consumeCurrentItem(int count);
    int getCurrentItemID() const;
    int getCurrentItemCount() const;
    const ItemStack& getCurrentStack() const;
    ItemStack& getCurrentStack();
    void nextSlot();
    void prevSlot();
    void setSlot(int slot);
    void handleClick(int slot, bool rightClick);

    void updateCrafting();
    void handleCraftingResult(int resultSlot);

    ItemStack mainInventory[TOTAL_SIZE];
    ItemStack cursorStack;
    int currentSlot = 0;
};
