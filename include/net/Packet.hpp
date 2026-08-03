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
    PlayerRotation =  6,
    PlayerPosLook = 7,
    LoginResponse = 8,
    PlayerDigging = 9,
    BlockPlacement = 10,
    BlockChange = 11,
    DestroyEntity = 12,
    ChunkRequest = 13,
    ChunkUnload = 14,
    InventoryAdd = 15,
    WindowItems = 16,
    SetSlot = 17,
    ClickWindow = 18,
    ConfirmTransaction = 19,
    UseEntity = 20,
    CollectItem = 21,
    PlaySound = 22,
    UpdateHealth = 23,
    ChatMessage = 24,
    GameModeChange = 25,
    HeldItemChange = 26,
    KeyResponse = 27,
    TimeUpdate = 28,
    OpenChest = 29,
    EntityHurt = 30
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
    static void writeBytes(std::vector<uint8_t>& buffer, const uint8_t* data, size_t size);

    static int32_t readInt(const uint8_t*& data);
    static float readFloat(const uint8_t*& data);
    static double readDouble(const uint8_t*& data);
    static std::string readString(const uint8_t*& data);
    static uint8_t readByte(const uint8_t*& data);
    static void readBytes(const uint8_t*& data, uint8_t* target, size_t size);
};

template<typename T>
class AutoPacket : public Packet {
public:
    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)static_cast<const T*>(this)->getType());
        static_cast<const T*>(this)->writeImpl(buffer);
    }
    void deserialize(const uint8_t* data, size_t size) override {
        (void)size;
        static_cast<T*>(this)->readImpl(data);
    }
};
