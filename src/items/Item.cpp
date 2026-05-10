#include "items/Item.hpp"
#include "items/ItemFood.hpp"

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

Item::Item(int id) : itemID(id) {
    if (id >= 0 && id < 1024) {
        itemsList[id] = this;
    }
}

void Item::init() {
    shovelSteel = new Item(256); shovelSteel->name = "shovelSteel"; shovelSteel->iconIndex = 82;
    pickaxeSteel = new Item(257); pickaxeSteel->name = "pickaxeSteel"; pickaxeSteel->iconIndex = 98;
    axeSteel = new Item(258); axeSteel->name = "axeSteel"; axeSteel->iconIndex = 114;
    striker = new Item(259); striker->name = "striker"; striker->iconIndex = 5;
    appleRed = new ItemFood(260, 4); appleRed->name = "appleRed"; appleRed->iconIndex = 10;
    bow = new Item(261); bow->name = "bow"; bow->iconIndex = 21;
    arrow = new Item(262); arrow->name = "arrow"; arrow->iconIndex = 37;
    coal = new Item(263); coal->name = "coal"; coal->iconIndex = 7;
    diamond = new Item(264); diamond->name = "diamond"; diamond->iconIndex = 55;
    ingotIron = new Item(265); ingotIron->name = "ingotIron"; ingotIron->iconIndex = 23;
    ingotGold = new Item(266); ingotGold->name = "ingotGold"; ingotGold->iconIndex = 39;
    swordSteel = new Item(267); swordSteel->name = "swordSteel"; swordSteel->iconIndex = 66;
    swordWood = new Item(268); swordWood->name = "swordWood"; swordWood->iconIndex = 64;
    shovelWood = new Item(269); shovelWood->name = "shovelWood"; shovelWood->iconIndex = 80;
    pickaxeWood = new Item(270); pickaxeWood->name = "pickaxeWood"; pickaxeWood->iconIndex = 96;
    axeWood = new Item(271); axeWood->name = "axeWood"; axeWood->iconIndex = 112;
    swordStone = new Item(272); swordStone->name = "swordStone"; swordStone->iconIndex = 65;
    shovelStone = new Item(273); shovelStone->name = "shovelStone"; shovelStone->iconIndex = 81;
    pickaxeStone = new Item(274); pickaxeStone->name = "pickaxeStone"; pickaxeStone->iconIndex = 97;
    axeStone = new Item(275); axeStone->name = "axeStone"; axeStone->iconIndex = 113;
    swordDiamond = new Item(276); swordDiamond->name = "swordDiamond"; swordDiamond->iconIndex = 67;
    shovelDiamond = new Item(277); shovelDiamond->name = "shovelDiamond"; shovelDiamond->iconIndex = 83;
    pickaxeDiamond = new Item(278); pickaxeDiamond->name = "pickaxeDiamond"; pickaxeDiamond->iconIndex = 99;
    axeDiamond = new Item(279); axeDiamond->name = "axeDiamond"; axeDiamond->iconIndex = 115;
    stick = new Item(280); stick->name = "stick"; stick->iconIndex = 53;
    bowlEmpty = new Item(281); bowlEmpty->name = "bowlEmpty"; bowlEmpty->iconIndex = 71;
    bowlSoup = new ItemFood(282, 10); bowlSoup->name = "bowlSoup"; bowlSoup->iconIndex = 72;
    swordGold = new Item(283); swordGold->name = "swordGold"; swordGold->iconIndex = 68;
    shovelGold = new Item(284); shovelGold->name = "shovelGold"; shovelGold->iconIndex = 84;
    pickaxeGold = new Item(285); pickaxeGold->name = "pickaxeGold"; pickaxeGold->iconIndex = 100;
    axeGold = new Item(286); axeGold->name = "axeGold"; axeGold->iconIndex = 116;
    silk = new Item(287); silk->name = "silk"; silk->iconIndex = 8;
    feather = new Item(288); feather->name = "feather"; feather->iconIndex = 24;
    gunpowder = new Item(289); gunpowder->name = "gunpowder"; gunpowder->iconIndex = 40;
    hoeWood = new Item(290); hoeWood->name = "hoeWood"; hoeWood->iconIndex = 128;
    hoeStone = new Item(291); hoeStone->name = "hoeStone"; hoeStone->iconIndex = 129;
    hoeSteel = new Item(292); hoeSteel->name = "hoeSteel"; hoeSteel->iconIndex = 130;
    hoeDiamond = new Item(293); hoeDiamond->name = "hoeDiamond"; hoeDiamond->iconIndex = 131;
    hoeGold = new Item(294); hoeGold->name = "hoeGold"; hoeGold->iconIndex = 132;
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
}
