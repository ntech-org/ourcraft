#include "entities/InventoryPlayer.hpp"
#include "inventory/CraftingManager.hpp"
#include <algorithm>

InventoryPlayer::InventoryPlayer() {
    for (int i = 0; i < TOTAL_SIZE; ++i) {
        mainInventory[i] = {0, 0, 0};
    }
    cursorStack = {0, 0, 0};
}

int InventoryPlayer::addItemReturningRemainder(int itemID, int count, uint8_t metadata) {
    if (itemID <= 0 || count <= 0) {
        return count > 0 ? count : 0;
    }

    int remaining = count;
    for (int i = 0; i < INVENTORY_SIZE && remaining > 0; ++i) {
        ItemStack& slot = mainInventory[i];
        if (slot.itemID == itemID && slot.metadata == metadata && slot.count < MAX_STACK_SIZE) {
            const int canAdd = MAX_STACK_SIZE - slot.count;
            const int toAdd = remaining < canAdd ? remaining : canAdd;
            slot.count += toAdd;
            remaining -= toAdd;
        }
    }

    for (int i = 0; i < INVENTORY_SIZE && remaining > 0; ++i) {
        ItemStack& slot = mainInventory[i];
        if (slot.isEmpty()) {
            const int toAdd = remaining < MAX_STACK_SIZE ? remaining : MAX_STACK_SIZE;
            slot.itemID = itemID;
            slot.count = toAdd;
            slot.metadata = metadata;
            remaining -= toAdd;
        }
    }

    return remaining;
}

bool InventoryPlayer::addItem(int itemID, int count, uint8_t metadata) {
    return addItemReturningRemainder(itemID, count, metadata) == 0;
}

bool InventoryPlayer::consumeCurrentItem(int count) {
    if (count <= 0 || currentSlot < 0 || currentSlot >= HOTBAR_SIZE) {
        return false;
    }

    ItemStack& slot = mainInventory[currentSlot];
    if (slot.isEmpty() || slot.count < count) {
        return false;
    }

    slot.count -= count;
    if (slot.count == 0) {
        slot = {0, 0, 0};
    }
    return true;
}

int InventoryPlayer::getCurrentItemID() const {
    if (currentSlot < 0 || currentSlot >= HOTBAR_SIZE) {
        return 0;
    }
    const ItemStack& stack = mainInventory[currentSlot];
    return stack.isEmpty() ? 0 : stack.itemID;
}

int InventoryPlayer::getCurrentItemCount() const {
    if (currentSlot < 0 || currentSlot >= HOTBAR_SIZE) {
        return 0;
    }
    return mainInventory[currentSlot].count;
}

const ItemStack& InventoryPlayer::getCurrentStack() const {
    return mainInventory[currentSlot];
}

ItemStack& InventoryPlayer::getCurrentStack() {
    return mainInventory[currentSlot];
}

void InventoryPlayer::nextSlot() {
    currentSlot = (currentSlot + 1) % HOTBAR_SIZE;
}

void InventoryPlayer::prevSlot() {
    currentSlot = (currentSlot + HOTBAR_SIZE - 1) % HOTBAR_SIZE;
}

void InventoryPlayer::setSlot(int slot) {
    if (slot >= 0 && slot < HOTBAR_SIZE) currentSlot = slot;
}

void InventoryPlayer::handleCraftingResult(int resultSlot) {
    if (resultSlot == RESULT_SLOT) {
        CraftingManager::getInstance().consumeRecipe(&mainInventory[CRAFT_START], 2, 2);
    } else if (resultSlot == WORKBENCH_RESULT) {
        CraftingManager::getInstance().consumeRecipe(&mainInventory[WORKBENCH_START], 3, 3);
    }
    updateCrafting();
}

void InventoryPlayer::handleClick(int slot, bool rightClick) {
    if (slot < -1 || slot >= TOTAL_SIZE) return;

    if (slot == RESULT_SLOT || slot == WORKBENCH_RESULT) {
        if (!mainInventory[slot].isEmpty()) {
            if (cursorStack.isEmpty()) {
                cursorStack = mainInventory[slot];
                handleCraftingResult(slot);
            } else if (cursorStack.itemID == mainInventory[slot].itemID &&
                       cursorStack.metadata == mainInventory[slot].metadata &&
                       cursorStack.count + mainInventory[slot].count <= MAX_STACK_SIZE) {
                cursorStack.count += mainInventory[slot].count;
                handleCraftingResult(slot);
            }
        }
        return;
    }

    if (slot >= 0) {
        ItemStack& target = mainInventory[slot];
        const bool cursorEmpty = cursorStack.isEmpty();
        const bool targetEmpty = target.isEmpty();

        if (rightClick) {
            if (cursorEmpty) {
                if (!targetEmpty) {
                    const int take = (target.count + 1) / 2;
                    cursorStack = target;
                    cursorStack.count = take;
                    target.count -= take;
                    if (target.count <= 0) target = {0, 0, 0};
                }
            } else {
                if (targetEmpty) {
                    target = cursorStack;
                    target.count = 1;
                    cursorStack.count -= 1;
                    if (cursorStack.count <= 0) cursorStack = {0, 0, 0};
                } else if (target.itemID == cursorStack.itemID &&
                           target.metadata == cursorStack.metadata &&
                           target.count < MAX_STACK_SIZE) {
                    target.count += 1;
                    cursorStack.count -= 1;
                    if (cursorStack.count <= 0) cursorStack = {0, 0, 0};
                }
            }
        } else {
            if (cursorEmpty) {
                if (!targetEmpty) {
                    cursorStack = target;
                    target = {0, 0, 0};
                }
            } else if (targetEmpty) {
                target = cursorStack;
                cursorStack = {0, 0, 0};
            } else if (target.itemID == cursorStack.itemID && target.metadata == cursorStack.metadata) {
                const int free = MAX_STACK_SIZE - target.count;
                const int moved = std::min(free, cursorStack.count);
                target.count += moved;
                cursorStack.count -= moved;
                if (cursorStack.count <= 0) cursorStack = {0, 0, 0};
            } else {
                std::swap(target, cursorStack);
            }
        }
        
        if ((slot >= CRAFT_START && slot < CRAFT_START + 4) || 
            (slot >= WORKBENCH_START && slot < WORKBENCH_START + 9)) {
            updateCrafting();
        }
    } else if (slot == -1) {
        if (!cursorStack.isEmpty()) {
            if (rightClick) {
                cursorStack.count -= 1;
                if (cursorStack.count <= 0) cursorStack = {0, 0, 0};
            } else {
                cursorStack = {0, 0, 0};
            }
        }
    }
}

void InventoryPlayer::handleCreativeClick(int slot, bool rightClick) {
    if (slot == -1) {
        cursorStack = {0, 0, 0};
        return;
    }
    handleClick(slot, rightClick);
}

void InventoryPlayer::updateCrafting() {
    mainInventory[RESULT_SLOT] = CraftingManager::getInstance().findMatchingRecipe(&mainInventory[CRAFT_START], 2, 2);
    mainInventory[WORKBENCH_RESULT] = CraftingManager::getInstance().findMatchingRecipe(&mainInventory[WORKBENCH_START], 3, 3);
}

