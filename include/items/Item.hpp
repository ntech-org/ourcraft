#pragma once

#include "entities/InventoryPlayer.hpp"
#include <string>
#include <vector>

class World;
class EntityPlayer;

class Item {
public:
    static Item* itemsList[1024];
    
    static Item* shovelSteel;
    static Item* pickaxeSteel;
    static Item* axeSteel;
    static Item* striker;
    static Item* appleRed;
    static Item* bow;
    static Item* arrow;
    static Item* coal;
    static Item* diamond;
    static Item* ingotIron;
    static Item* ingotGold;
    static Item* swordSteel;
    static Item* swordWood;
    static Item* shovelWood;
    static Item* pickaxeWood;
    static Item* axeWood;
    static Item* swordStone;
    static Item* shovelStone;
    static Item* pickaxeStone;
    static Item* axeStone;
    static Item* swordDiamond;
    static Item* shovelDiamond;
    static Item* pickaxeDiamond;
    static Item* axeDiamond;
    static Item* stick;
    static Item* bowlEmpty;
    static Item* bowlSoup;
    static Item* swordGold;
    static Item* shovelGold;
    static Item* pickaxeGold;
    static Item* axeGold;
    static Item* silk;
    static Item* feather;
    static Item* gunpowder;
    static Item* hoeWood;
    static Item* hoeStone;
    static Item* hoeSteel;
    static Item* hoeDiamond;
    static Item* hoeGold;
    static Item* seeds;
    static Item* wheat;
    static Item* bread;
    static Item* flint;
    static Item* porkRaw;
    static Item* porkCooked;
    static Item* painting;
    static Item* appleGold;
    static Item* sign;
    static Item* door;
    static Item* bucketEmpty;
    static Item* bucketWater;
    static Item* bucketLava;
    static Item* minecart;
    static Item* saddle;

    static void init();

    Item(int id);
    virtual ~Item() = default;

    virtual bool onItemUse(ItemStack& stack, EntityPlayer& player, World& world, int x, int y, int z, int side) { return false; }
    virtual ItemStack onItemRightClick(ItemStack stack, World& world, EntityPlayer& player) { return stack; }

    int itemID;
    int iconIndex = 0;
    int maxStackSize = 64;
    std::string name;
};
