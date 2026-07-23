#pragma once

#include "entities/Entity.hpp"
#include <cstdint>

class World;

class SimulationTick {
public:
    static void voidProtection(World& world);
    static void serverVoidProtection(World& world);
};
