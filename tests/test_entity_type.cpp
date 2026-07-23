#include <doctest/doctest.h>
#include "entities/Entity.hpp"

TEST_CASE("Entity type system") {
    CHECK(EntityType::Unknown != EntityType::Player);
    CHECK(EntityType::Player != EntityType::Zombie);
    CHECK(EntityType::Item != EntityType::Living);
}

TEST_CASE("EntityType enum values") {
    CHECK(static_cast<int>(EntityType::Unknown) == 0);
    CHECK(static_cast<int>(EntityType::Player) == 1);
    CHECK(static_cast<int>(EntityType::Living) == 2);
    CHECK(static_cast<int>(EntityType::Zombie) == 3);
    CHECK(static_cast<int>(EntityType::Item) == 4);
}
