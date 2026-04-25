#pragma once

#include "net/Packet.hpp"
#include "util/Compression.hpp"
#include <cstring>

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

class PacketChunkData : public Packet {
public:
    int32_t x, z;
    uint8_t primaryBitmask = 0;
    
    // Use pointers to avoid massive copies if possible
    const uint8_t* blockPtr = nullptr;
    const uint8_t* metaPtr = nullptr;
    const uint8_t* skyPtr = nullptr;
    const uint8_t* blockLightPtr = nullptr;

    // Owned data for client-side
    std::vector<uint8_t> blocks;
    std::vector<uint8_t> metadata;
    std::vector<uint8_t> skylight;
    std::vector<uint8_t> blocklight;

    PacketType getType() const override { return PacketType::ChunkData; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, x);
        writeInt(buffer, z);
        writeByte(buffer, primaryBitmask);

        std::vector<uint8_t> uncompressed;
        int sectionCount = 0;
        for(int i = 0; i < 8; ++i) if(primaryBitmask & (1 << i)) sectionCount++;

        uncompressed.reserve(sectionCount * (4096 + 2048 + 2048 + 2048));

        const uint8_t* b = blockPtr ? blockPtr : blocks.data();
        const uint8_t* m = metaPtr ? metaPtr : metadata.data();
        const uint8_t* s = skyPtr ? skyPtr : skylight.data();
        const uint8_t* bl = blockLightPtr ? blockLightPtr : blocklight.data();

        // 1. Blocks
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                const uint8_t* src = b + i * 4096;
                uncompressed.insert(uncompressed.end(), src, src + 4096);
            }
        }

        // 2. Metadata (already packed in memory)
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                const uint8_t* src = m + i * 2048;
                uncompressed.insert(uncompressed.end(), src, src + 2048);
            }
        }

        // 3. Skylight
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                const uint8_t* src = s + i * 2048;
                uncompressed.insert(uncompressed.end(), src, src + 2048);
            }
        }

        // 4. Blocklight
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                const uint8_t* src = bl + i * 2048;
                uncompressed.insert(uncompressed.end(), src, src + 2048);
            }
        }

        std::vector<uint8_t> compressed;
        Compression::compress(uncompressed, compressed);

        writeInt(buffer, (int32_t)compressed.size());
        writeBytes(buffer, compressed.data(), compressed.size());
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readInt(data);
        z = readInt(data);
        primaryBitmask = readByte(data);
        
        int32_t compressedSize = readInt(data);
        const uint8_t* compressedPtr = data;
        
        int sectionCount = 0;
        for(int i = 0; i < 8; ++i) if(primaryBitmask & (1 << i)) sectionCount++;

        size_t totalExpected = sectionCount * (4096 + 2048 + 2048 + 2048);
        std::vector<uint8_t> decompressed;
        Compression::decompress(compressedPtr, compressedSize, decompressed, totalExpected);

        blocks.assign(16 * 128 * 16, 0);
        metadata.assign(16 * 128 * 16 / 2, 0);
        skylight.assign(16 * 128 * 16 / 2, 0);
        blocklight.assign(16 * 128 * 16 / 2, 0);

        const uint8_t* ptr = decompressed.data();
        
        // 1. Blocks
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                std::memcpy(blocks.data() + i * 4096, ptr, 4096);
                ptr += 4096;
            }
        }

        // 2. Metadata
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                std::memcpy(metadata.data() + i * 2048, ptr, 2048);
                ptr += 2048;
            }
        }

        // 3. Skylight
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                std::memcpy(skylight.data() + i * 2048, ptr, 2048);
                ptr += 2048;
            }
        }

        // 4. Blocklight
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                std::memcpy(blocklight.data() + i * 2048, ptr, 2048);
                ptr += 2048;
            }
        }
    }
};

enum class DiggingAction : uint8_t {
    START = 0,
    STOP = 1,
    FINISH = 2
};

class PacketPlayerDigging : public Packet {
public:
    DiggingAction action;
    int32_t x, y, z;
    uint8_t face;

    PacketType getType() const override { return PacketType::PlayerDigging; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeByte(buffer, (uint8_t)action);
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, face);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        action = (DiggingAction)readByte(data);
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        face = readByte(data);
    }
};

class PacketBlockPlacement : public Packet {
public:
    int32_t x, y, z;
    uint8_t face;
    uint8_t blockID;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::BlockPlacement; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, face);
        writeByte(buffer, blockID);
        writeByte(buffer, metadata);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        face = readByte(data);
        blockID = readByte(data);
        metadata = readByte(data);
    }
};

class PacketBlockChange : public Packet {
public:
    int32_t x, y, z;
    uint8_t blockID;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::BlockChange; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, blockID);
        writeByte(buffer, metadata);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        blockID = readByte(data);
        metadata = readByte(data);
    }
};

class PacketDestroyEntity : public Packet {
public:
    int32_t id;

    PacketType getType() const override { return PacketType::DestroyEntity; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, id);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        id = readInt(data);
    }
};

class PacketChunkRequest : public Packet {
public:
    int32_t x, z;

    PacketType getType() const override { return PacketType::ChunkRequest; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, x);
        writeInt(buffer, z);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readInt(data);
        z = readInt(data);
    }
};

class PacketChunkUnload : public Packet {
public:
    int32_t x, z;

    PacketType getType() const override { return PacketType::ChunkUnload; }

    void serialize(std::vector<uint8_t>& buffer) const override {
        writeByte(buffer, (uint8_t)getType());
        writeInt(buffer, x);
        writeInt(buffer, z);
    }

    void deserialize(const uint8_t* data, size_t size) override {
        x = readInt(data);
        z = readInt(data);
    }
};