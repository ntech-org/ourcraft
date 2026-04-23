#pragma once

#include "net/Packet.hpp"

class PacketLogin : public Packet {
public:
    std::string username;
    int32_t protocolVersion;

    PacketType getType() const override { return PacketType::Login; }
    
    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeString(buffer, username);
        writeInt(buffer, protocolVersion);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        username = readString(data);
        protocolVersion = readInt(data);
    }
};

class PacketLoginResponse : public Packet {
public:
    int32_t entityID;

    PacketType getType() const override { return PacketType::LoginResponse; }
    
    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, entityID);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        entityID = readInt(data);
    }
};

class PacketPlayerPosition : public Packet {
public:
    double x, y, z;
    float yaw, pitch;
    bool onGround;

    PacketType getType() const override { return PacketType::PlayerPosition; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
        writeByte(buffer, onGround ? 1 : 0);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
        onGround = readByte(data) != 0;
    }
};

class PacketSpawnEntity : public Packet {
public:
    int32_t id;
    uint8_t type; // 0 = Player, 1 = Zombie
    double x, y, z;
    float yaw, pitch;

    PacketType getType() const override { return PacketType::SpawnEntity; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, id);
        writeByte(buffer, type);
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        id = readInt(data);
        type = readByte(data);
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
    }
};

class PacketMoveEntity : public Packet {
public:
    int32_t id;
    double x, y, z;
    float yaw, pitch;

    PacketType getType() const override { return PacketType::MoveEntity; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, id);
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        id = readInt(data);
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
    }
};
