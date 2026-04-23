#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <enet/enet.h>

enum class PacketType : uint8_t {
    Login = 0,
    Disconnect = 1,
    ChunkData = 2,
    SpawnEntity = 3,
    MoveEntity = 4,
    PlayerPosition = 5,
    LoginResponse = 6
};

class Packet {
public:
    virtual ~Packet() = default;
    virtual PacketType getType() const = 0;
    virtual void serialize(std::vector<uint8_t>& buffer) const = 0;
    virtual void deserialize(const uint8_t* data, size_t size) = 0;

    static void writeInt(std::vector<uint8_t>& buffer, int32_t value);
    static void writeFloat(std::vector<uint8_t>& buffer, float value);
    static void writeDouble(std::vector<uint8_t>& buffer, double value);
    static void writeString(std::vector<uint8_t>& buffer, const std::string& value);
    static void writeByte(std::vector<uint8_t>& buffer, uint8_t value);

    static int32_t readInt(const uint8_t*& data);
    static float readFloat(const uint8_t*& data);
    static double readDouble(const uint8_t*& data);
    static std::string readString(const uint8_t*& data);
    static uint8_t readByte(const uint8_t*& data);
};
