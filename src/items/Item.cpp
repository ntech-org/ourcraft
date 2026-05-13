#include "items/Item.hpp"
#include "items/ItemFood.hpp"
#include "items/ItemPickaxe.hpp"
#include "items/ItemAxe.hpp"
#include "items/ItemSpade.hpp"
#include "items/ItemSword.hpp"
#include "items/ItemHoe.hpp"

Item* Item::itemsList[1024] = { nullptr };

Item* Item::shovelSteel = nullptr;
Item* Item::pickaxeSteel = nullptr;
Item* Item::axeSteel = nullptr;
Item* Item::striker = nullptr;
Item* Item::appleRed = nullptr;
Item* Item::bow = nullptr;
Item* Item::arrow = nullptr;
Item* Item::coal = nullptr;
Item* Item::diamond = nullptr;
Item* Item::ingotIron = nullptr;
Item* Item::ingotGold = nullptr;
Item* Item::swordSteel = nullptr;
Item* Item::swordWood = nullptr;
Item* Item::shovelWood = nullptr;
Item* Item::pickaxeWood = nullptr;
Item* Item::axeWood = nullptr;
Item* Item::swordStone = nullptr;
Item* Item::shovelStone = nullptr;
Item* Item::pickaxeStone = nullptr;
Item* Item::axeStone = nullptr;
Item* Item::swordDiamond = nullptr;
Item* Item::shovelDiamond = nullptr;
Item* Item::pickaxeDiamond = nullptr;
Item* Item::axeDiamond = nullptr;
Item* Item::stick = nullptr;
Item* Item::bowlEmpty = nullptr;
Item* Item::bowlSoup = nullptr;
Item* Item::swordGold = nullptr;
Item* Item::shovelGold = nullptr;
Item* Item::pickaxeGold = nullptr;
Item* Item::axeGold = nullptr;
Item* Item::silk = nullptr;
Item* Item::feather = nullptr;
Item* Item::gunpowder = nullptr;
Item* Item::hoeWood = nullptr;
Item* Item::hoeStone = nullptr;
Item* Item::hoeSteel = nullptr;
Item* Item::hoeDiamond = nullptr;
Item* Item::hoeGold = nullptr;
Item* Item::seeds = nullptr;
Item* Item::wheat = nullptr;
Item* Item::bread = nullptr;
Item* Item::flint = nullptr;
Item* Item::porkRaw = nullptr;
Item* Item::porkCooked = nullptr;
Item* Item::painting = nullptr;
Item* Item::appleGold = nullptr;
Item* Item::sign = nullptr;
Item* Item::door = nullptr;
Item* Item::bucketEmpty = nullptr;
Item* Item::bucketWater = nullptr;
Item* Item::bucketLava = nullptr;
Item* Item::minecart = nullptr;
Item* Item::saddle = nullptr;

Item* Item::helmetLeather = nullptr;
Item* Item::plateLeather = nullptr;
Item* Item::legsLeather = nullptr;
Item* Item::bootsLeather = nullptr;
Item* Item::helmetChain = nullptr;
Item* Item::plateChain = nullptr;
Item* Item::legsChain = nullptr;
Item* Item::bootsChain = nullptr;
Item* Item::helmetSteel = nullptr;
Item* Item::plateSteel = nullptr;
Item* Item::legsSteel = nullptr;
Item* Item::bootsSteel = nullptr;
Item* Item::helmetDiamond = nullptr;
Item* Item::plateDiamond = nullptr;
Item* Item::legsDiamond = nullptr;
Item* Item::bootsDiamond = nullptr;
Item* Item::helmetGold = nullptr;
Item* Item::plateGold = nullptr;
Item* Item::legsGold = nullptr;
Item* Item::bootsGold = nullptr;

Item::Item(int id) : itemID(id) {
    if (id >= 0 && id < 1024) {
        itemsList[id] = this;
    }
}

void Item::init() {
    shovelSteel = new ItemSpade(256, 2); shovelSteel->name = "shovelSteel"; shovelSteel->iconIndex = 82;
    pickaxeSteel = new ItemPickaxe(257, 2); pickaxeSteel->name = "pickaxeSteel"; pickaxeSteel->iconIndex = 98;
    axeSteel = new ItemAxe(258, 2); axeSteel->name = "axeSteel"; axeSteel->iconIndex = 114;
    striker = new Item(259); striker->name = "striker"; striker->iconIndex = 5;
    appleRed = new ItemFood(260, 4); appleRed->name = "appleRed"; appleRed->iconIndex = 10;
    bow = new Item(261); bow->name = "bow"; bow->iconIndex = 21;
    arrow = new Item(262); arrow->name = "arrow"; arrow->iconIndex = 37;
    coal = new Item(263); coal->name = "coal"; coal->iconIndex = 7;
    diamond = new Item(264); diamond->name = "diamond"; diamond->iconIndex = 55;
    ingotIron = new Item(265); ingotIron->name = "ingotIron"; ingotIron->iconIndex = 23;
    ingotGold = new Item(266); ingotGold->name = "ingotGold"; ingotGold->iconIndex = 39;
    swordSteel = new ItemSword(267, 2); swordSteel->name = "swordSteel"; swordSteel->iconIndex = 66;
    swordWood = new ItemSword(268, 0); swordWood->name = "swordWood"; swordWood->iconIndex = 64;
    shovelWood = new ItemSpade(269, 0); shovelWood->name = "shovelWood"; shovelWood->iconIndex = 80;
    pickaxeWood = new ItemPickaxe(270, 0); pickaxeWood->name = "pickaxeWood"; pickaxeWood->iconIndex = 96;
    axeWood = new ItemAxe(271, 0); axeWood->name = "axeWood"; axeWood->iconIndex = 112;
    swordStone = new ItemSword(272, 1); swordStone->name = "swordStone"; swordStone->iconIndex = 65;
    shovelStone = new ItemSpade(273, 1); shovelStone->name = "shovelStone"; shovelStone->iconIndex = 81;
    pickaxeStone = new ItemPickaxe(274, 1); pickaxeStone->name = "pickaxeStone"; pickaxeStone->iconIndex = 97;
    axeStone = new ItemAxe(275, 1); axeStone->name = "axeStone"; axeStone->iconIndex = 113;
    swordDiamond = new ItemSword(276, 3); swordDiamond->name = "swordDiamond"; swordDiamond->iconIndex = 67;
    shovelDiamond = new ItemSpade(277, 3); shovelDiamond->name = "shovelDiamond"; shovelDiamond->iconIndex = 83;
    pickaxeDiamond = new ItemPickaxe(278, 3); pickaxeDiamond->name = "pickaxeDiamond"; pickaxeDiamond->iconIndex = 99;
    axeDiamond = new ItemAxe(279, 3); axeDiamond->name = "axeDiamond"; axeDiamond->iconIndex = 115;
    stick = new Item(280); stick->name = "stick"; stick->iconIndex = 53;
    bowlEmpty = new Item(281); bowlEmpty->name = "bowlEmpty"; bowlEmpty->iconIndex = 71;
    bowlSoup = new ItemFood(282, 10); bowlSoup->name = "bowlSoup"; bowlSoup->iconIndex = 72;
    swordGold = new ItemSword(283, 4); swordGold->name = "swordGold"; swordGold->iconIndex = 68;
    shovelGold = new ItemSpade(284, 4); shovelGold->name = "shovelGold"; shovelGold->iconIndex = 84;
    pickaxeGold = new ItemPickaxe(285, 4); pickaxeGold->name = "pickaxeGold"; pickaxeGold->iconIndex = 100;
    axeGold = new ItemAxe(286, 4); axeGold->name = "axeGold"; axeGold->iconIndex = 116;
    silk = new Item(287); silk->name = "silk"; silk->iconIndex = 8;
    feather = new Item(288); feather->name = "feather"; feather->iconIndex = 24;
    gunpowder = new Item(289); gunpowder->name = "gunpowder"; gunpowder->iconIndex = 40;
    hoeWood = new ItemHoe(290, 0); hoeWood->name = "hoeWood"; hoeWood->iconIndex = 128;
    hoeStone = new ItemHoe(291, 1); hoeStone->name = "hoeStone"; hoeStone->iconIndex = 129;
    hoeSteel = new ItemHoe(292, 2); hoeSteel->name = "hoeSteel"; hoeSteel->iconIndex = 130;
    hoeDiamond = new ItemHoe(293, 3); hoeDiamond->name = "hoeDiamond"; hoeDiamond->iconIndex = 131;
    hoeGold = new ItemHoe(294, 4); hoeGold->name = "hoeGold"; hoeGold->iconIndex = 132;
    seeds = new Item(295); seeds->name = "seeds"; seeds->iconIndex = 9;
    wheat = new Item(296); wheat->name = "wheat"; wheat->iconIndex = 25;
    bread = new ItemFood(297, 5); bread->name = "bread"; bread->iconIndex = 41;
    flint = new Item(318); flint->name = "flint"; flint->iconIndex = 6;
    porkRaw = new ItemFood(319, 3); porkRaw->name = "porkRaw"; porkRaw->iconIndex = 87;
    porkCooked = new ItemFood(320, 8); porkCooked->name = "porkCooked"; porkCooked->iconIndex = 88;
    painting = new Item(321); painting->name = "painting"; painting->iconIndex = 26;
    appleGold = new ItemFood(322, 42); appleGold->name = "appleGold"; appleGold->iconIndex = 11;
    sign = new Item(323); sign->name = "sign"; sign->iconIndex = 42;
    door = new Item(324); door->name = "door"; door->iconIndex = 43;
    bucketEmpty = new Item(325); bucketEmpty->name = "bucketEmpty"; bucketEmpty->iconIndex = 74;
    bucketWater = new Item(326); bucketWater->name = "bucketWater"; bucketWater->iconIndex = 75;
    bucketLava = new Item(327); bucketLava->name = "bucketLava"; bucketLava->iconIndex = 76;
    minecart = new Item(328); minecart->name = "minecart"; minecart->iconIndex = 135;
    saddle = new Item(329); saddle->name = "saddle"; saddle->iconIndex = 104;

    // Armor items (plain Item for now, no special behavior)
    helmetLeather = new Item(330); helmetLeather->name = "helmetLeather"; helmetLeather->iconIndex = 106;
    plateLeather = new Item(331); plateLeather->name = "plateLeather"; plateLeather->iconIndex = 107;
    legsLeather = new Item(332); legsLeather->name = "legsLeather"; legsLeather->iconIndex = 108;
    bootsLeather = new Item(333); bootsLeather->name = "bootsLeather"; bootsLeather->iconIndex = 109;
    helmetChain = new Item(334); helmetChain->name = "helmetChain"; helmetChain->iconIndex = 110;
    plateChain = new Item(335); plateChain->name = "plateChain"; plateChain->iconIndex = 111;
    legsChain = new Item(336); legsChain->name = "legsChain"; legsChain->iconIndex = 112;
    bootsChain = new Item(337); bootsChain->name = "bootsChain"; bootsChain->iconIndex = 113;
    helmetSteel = new Item(338); helmetSteel->name = "helmetSteel"; helmetSteel->iconIndex = 114;
    plateSteel = new Item(339); plateSteel->name = "plateSteel"; plateSteel->iconIndex = 115;
    legsSteel = new Item(340); legsSteel->name = "legsSteel"; legsSteel->iconIndex = 116;
    bootsSteel = new Item(341); bootsSteel->name = "bootsSteel"; bootsSteel->iconIndex = 117;
    helmetDiamond = new Item(342); helmetDiamond->name = "helmetDiamond"; helmetDiamond->iconIndex = 118;
    plateDiamond = new Item(343); plateDiamond->name = "plateDiamond"; plateDiamond->iconIndex = 119;
    legsDiamond = new Item(344); legsDiamond->name = "legsDiamond"; legsDiamond->iconIndex = 120;
    bootsDiamond = new Item(345); bootsDiamond->name = "bootsDiamond"; bootsDiamond->iconIndex = 121;
    helmetGold = new Item(346); helmetGold->name = "helmetGold"; helmetGold->iconIndex = 122;
    plateGold = new Item(347); plateGold->name = "plateGold"; plateGold->iconIndex = 123;
    legsGold = new Item(348); legsGold->name = "legsGold"; legsGold->iconIndex = 124;
    bootsGold = new Item(349); bootsGold->name = "bootsGold"; bootsGold->iconIndex = 125;
}

float Item::getStrVsBlock(const Block& block) const {
    return 1.0f;
}

bool Item::canHarvestBlock(const Block& block) const {
    return false;
}
