#include "entities/InventoryPlayer.hpp"

InventoryPlayer::InventoryPlayer() {
    for (int i = 0; i < 36; ++i) {
        mainInventory[i] = {0, 0};
    }
    // Give some starting items
    mainInventory[0] = {1, 1}; // Stone
    mainInventory[1] = {3, 1}; // Dirt
    mainInventory[2] = {2, 1}; // Grass
}

void InventoryPlayer::addItem(int itemID, int count) {
    // Basic add logic
    for (int i = 0; i < 36; ++i) {
        if (mainInventory[i].itemID == itemID) {
            mainInventory[i].count += count;
            return;
        }
    }
    for (int i = 0; i < 36; ++i) {
        if (mainInventory[i].itemID == 0) {
            mainInventory[i].itemID = itemID;
            mainInventory[i].count = count;
            return;
        }
    }
}

int InventoryPlayer::getCurrentItemID() const {
    return mainInventory[currentSlot].itemID;
}

void InventoryPlayer::nextSlot() {
    currentSlot = (currentSlot + 1) % 9;
}

void InventoryPlayer::prevSlot() {
    currentSlot = (currentSlot + 8) % 9;
}

void InventoryPlayer::setSlot(int slot) {
    if (slot >= 0 && slot < 9) currentSlot = slot;
}
