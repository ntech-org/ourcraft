#pragma once
#include <cstdint>
#include <utility>

enum class LightType {
    Sky,
    Block
};

class IBlockAccess {
public:
    virtual ~IBlockAccess() = default;
    virtual uint8_t getBlockID(int x, int y, int z) const = 0;
    virtual uint8_t getBlockMetadata(int x, int y, int z) const = 0;
    virtual const class Material& getBlockMaterial(int x, int y, int z) const = 0;
    virtual std::pair<int, int> getLightPair(int x, int y, int z) const = 0;
};
