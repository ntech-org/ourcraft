#include <doctest/doctest.h>
#include "entities/InventoryPlayer.hpp"

TEST_CASE("ItemStack isEmpty") {
    ItemStack stack;
    CHECK(stack.isEmpty());

    stack.itemID = 1;
    stack.count = 1;
    CHECK_FALSE(stack.isEmpty());

    stack.itemID = 0;
    stack.count = 5;
    CHECK(stack.isEmpty());
}

TEST_CASE("ItemStack splitStack") {
    ItemStack stack{1, 64, 0, 0};
    ItemStack split = stack.splitStack(32);
    CHECK(split.itemID == 1);
    CHECK(split.count == 32);
    CHECK(stack.count == 32);
    CHECK(stack.itemID == 1);

    ItemStack split2 = stack.splitStack(32);
    CHECK(split2.count == 32);
    CHECK(stack.isEmpty());
}

TEST_CASE("InventoryPlayer construction") {
    InventoryPlayer inv;
    CHECK(inv.currentSlot == 0);
    for (int i = 0; i < InventoryPlayer::HOTBAR_SIZE; ++i) {
        CHECK(inv.mainInventory[i].isEmpty());
    }
}

TEST_CASE("InventoryPlayer getCurrentStack") {
    InventoryPlayer inv;
    inv.mainInventory[0] = {1, 64, 0, 0};
    ItemStack& current = inv.getCurrentStack();
    CHECK(current.itemID == 1);
    CHECK(current.count == 64);
}

TEST_CASE("InventoryPlayer getCurrentItemID") {
    InventoryPlayer inv;
    CHECK(inv.getCurrentItemID() == 0);

    inv.mainInventory[0] = {42, 1, 0, 0};
    CHECK(inv.getCurrentItemID() == 42);
}

TEST_CASE("InventoryPlayer consumeCurrentItem") {
    InventoryPlayer inv;
    inv.mainInventory[0] = {1, 64, 0, 0};

    CHECK(inv.consumeCurrentItem(1));
    CHECK(inv.mainInventory[0].count == 63);

    for (int i = 0; i < 62; ++i) {
        inv.consumeCurrentItem(1);
    }
    CHECK(inv.mainInventory[0].count == 1);
    CHECK(inv.consumeCurrentItem(1));
    CHECK(inv.mainInventory[0].isEmpty());
}

TEST_CASE("InventoryPlayer addItem") {
    InventoryPlayer inv;
    CHECK(inv.addItem(1, 64, 0));
    CHECK(inv.mainInventory[0].itemID == 1);
    CHECK(inv.mainInventory[0].count == 64);
}

TEST_CASE("InventoryPlayer slot navigation") {
    InventoryPlayer inv;
    CHECK(inv.currentSlot == 0);
    inv.nextSlot();
    CHECK(inv.currentSlot == 1);
    inv.prevSlot();
    CHECK(inv.currentSlot == 0);
    inv.setSlot(5);
    CHECK(inv.currentSlot == 5);
}
