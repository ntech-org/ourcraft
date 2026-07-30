#include <doctest/doctest.h>
#include "net/Packets.hpp"
#include "world/Chunk.hpp"

TEST_CASE("Chunk packet preserves column-major section data") {
    PacketChunkData outgoing;
    outgoing.x = -3;
    outgoing.z = 7;
    outgoing.primaryBitmask = (1u << 1) | (1u << 6);
    outgoing.lightBitmask = (1u << 1) | (1u << 4) | (1u << 6);

    std::vector<uint8_t> blocks(Chunk::SIZE, 0);
    std::vector<uint8_t> metadata(Chunk::SIZE / 2, 0);
    std::vector<uint8_t> skylight(Chunk::SIZE / 2, 0);
    std::vector<uint8_t> blocklight(Chunk::SIZE / 2, 0);

    auto index = [](int x, int y, int z) { return (x << 11) | (z << 7) | y; };
    auto setNibble = [](std::vector<uint8_t>& data, int i, uint8_t value) {
        if (i & 1) data[i >> 1] = (data[i >> 1] & 0x0F) | (value << 4);
        else data[i >> 1] = (data[i >> 1] & 0xF0) | value;
    };

    const int low = index(13, 23, 2);
    const int high = index(1, 111, 14);
    const int emptyLit = index(4, 70, 4);
    blocks[low] = 42;
    blocks[high] = 77;
    setNibble(metadata, low, 3);
    setNibble(metadata, high, 12);
    setNibble(skylight, low, 9);
    setNibble(skylight, high, 15);
    setNibble(blocklight, low, 6);
    setNibble(blocklight, high, 11);
    setNibble(skylight, emptyLit, 15);

    outgoing.blockPtr = blocks.data();
    outgoing.metaPtr = metadata.data();
    outgoing.skyPtr = skylight.data();
    outgoing.blockLightPtr = blocklight.data();

    std::vector<uint8_t> encoded;
    outgoing.serialize(encoded);
    const uint8_t* ptr = encoded.data() + 1;

    PacketChunkData incoming;
    incoming.deserialize(ptr, encoded.size() - 1);

    CHECK(incoming.x == outgoing.x);
    CHECK(incoming.z == outgoing.z);
    CHECK(incoming.primaryBitmask == outgoing.primaryBitmask);
    CHECK(incoming.lightBitmask == outgoing.lightBitmask);
    CHECK(incoming.blocks[low] == 42);
    CHECK(incoming.blocks[high] == 77);
    CHECK((incoming.metadata[low >> 1] >> 4) == 3);
    CHECK((incoming.metadata[high >> 1] >> 4) == 12);
    CHECK((incoming.skylight[low >> 1] >> 4) == 9);
    CHECK((incoming.skylight[high >> 1] >> 4) == 15);
    CHECK((incoming.blocklight[low >> 1] >> 4) == 6);
    CHECK((incoming.blocklight[high >> 1] >> 4) == 11);
    CHECK(incoming.blocks[emptyLit] == 0);
    CHECK((incoming.skylight[emptyLit >> 1] & 0x0F) == 15);
}
