#pragma once

#include <cstdint>

class World;

class TileEntity {
public:
    int x = 0, y = 0, z = 0;
    World* world = nullptr;
    virtual ~TileEntity() = default;
    virtual void updateEntity() {}
    virtual int getBlockID() const { return 0; }
};
