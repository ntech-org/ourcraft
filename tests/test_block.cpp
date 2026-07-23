#include <doctest/doctest.h>
#include "world/Block.hpp"
#include "world/Material.hpp"

TEST_CASE("Block initialization") {
    Block::init();
    CHECK(Block::blocksList[1] != nullptr);
}

TEST_CASE("Block hardness") {
    Block::init();
    CHECK(Block::getHardness(1) >= 0.0f);
}

TEST_CASE("Material types") {
    CHECK_FALSE(Material::air.isSolid());
    CHECK_FALSE(Material::air.isLiquid());

    CHECK(Material::rock.isSolid());
    CHECK_FALSE(Material::rock.isLiquid());

    CHECK(Material::water.isLiquid());
    CHECK_FALSE(Material::water.isSolid());

    CHECK(Material::lava.isLiquid());
    CHECK_FALSE(Material::lava.isSolid());

    CHECK(Material::ground.isSolid());
}
