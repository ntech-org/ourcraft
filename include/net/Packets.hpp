#pragma once

#include "net/Packet.hpp"
#include "util/Compression.hpp"
#include <cstring>

class PacketLogin : public AutoPacket<PacketLogin> {
public:
    std::string username;
    std::string uuid;
    std::string key;
    int32_t protocolVersion;

    PacketType getType() const override { return PacketType::Login; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeString(buffer, username);
        writeString(buffer, uuid);
        writeString(buffer, key);
        writeInt(buffer, protocolVersion);
    }

    void readImpl(const uint8_t*& data) {
        username = readString(data);
        uuid = readString(data);
        key = readString(data);
        protocolVersion = readInt(data);
    }
};

class PacketDisconnect : public AutoPacket<PacketDisconnect> {
public:
    std::string reason;

    PacketType getType() const override { return PacketType::Disconnect; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeString(buffer, reason);
    }

    void readImpl(const uint8_t*& data) {
        reason = readString(data);
    }
};

class PacketLoginResponse : public AutoPacket<PacketLoginResponse> {
public:
    int32_t entityID;
    std::string username;
    std::string uuid;

    PacketType getType() const override { return PacketType::LoginResponse; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, entityID);
        writeString(buffer, username);
        writeString(buffer, uuid);
    }

    void readImpl(const uint8_t*& data) {
        entityID = readInt(data);
        username = readString(data);
        uuid = readString(data);
    }
};

class PacketPlayerPosition : public AutoPacket<PacketPlayerPosition> {
public:
    double x, y, z;
    float yaw, pitch;
    bool onGround;

    PacketType getType() const override { return PacketType::PlayerPosition; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
        writeByte(buffer, onGround ? 1 : 0);
    }

    void readImpl(const uint8_t*& data) {
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
        onGround = readByte(data) != 0;
    }
};

class PacketPlayerRotation : public AutoPacket<PacketPlayerRotation> {
public:
    float yaw, pitch;
    bool onGround;

    PacketType getType() const override { return PacketType::PlayerRotation; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
        writeByte(buffer, onGround ? 1 : 0);
    }

    void readImpl(const uint8_t*& data) {
        yaw = readFloat(data);
        pitch = readFloat(data);
        onGround = readByte(data) != 0;
    }
};

class PacketPlayerPosLook : public AutoPacket<PacketPlayerPosLook> {
public:
    double x, y, z;
    float yaw, pitch;
    bool onGround;

    PacketType getType() const override { return PacketType::PlayerPosLook; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
        writeByte(buffer, onGround ? 1 : 0);
    }

    void readImpl(const uint8_t*& data) {
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
        onGround = readByte(data) != 0;
    }
};

class PacketSpawnEntity : public AutoPacket<PacketSpawnEntity> {
public:
    int32_t id;
    uint8_t type; // 0 = Player, 1 = Zombie, 2 = Item, 3 = Pig, 4 = Sheep, 5 = Skeleton, 6 = Spider, 7 = Creeper
    double x, y, z;
    float yaw, pitch;
    int32_t dataA = 0;
    int32_t dataB = 0;
    uint8_t dataC = 0;
    std::string username = "";
    std::string uuid = "";

    PacketType getType() const override { return PacketType::SpawnEntity; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, id);
        writeByte(buffer, type);
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
        writeInt(buffer, dataA);
        writeInt(buffer, dataB);
        writeByte(buffer, dataC);
        writeString(buffer, username);
        writeString(buffer, uuid);
    }

    void readImpl(const uint8_t*& data) {
        id = readInt(data);
        type = readByte(data);
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
        dataA = readInt(data);
        dataB = readInt(data);
        dataC = readByte(data);
        username = readString(data);
        uuid = readString(data);
    }
};

class PacketMoveEntity : public AutoPacket<PacketMoveEntity> {
public:
    int32_t id;
    double x, y, z;
    float yaw, pitch;

    PacketType getType() const override { return PacketType::MoveEntity; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, id);
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, yaw);
        writeFloat(buffer, pitch);
    }

    void readImpl(const uint8_t*& data) {
        id = readInt(data);
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        yaw = readFloat(data);
        pitch = readFloat(data);
    }
};

class PacketChunkData : public AutoPacket<PacketChunkData> {
public:
    int32_t x, z;
    uint8_t primaryBitmask = 0;
    uint8_t lightBitmask = 0;

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

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x);
        writeInt(buffer, z);
        writeByte(buffer, primaryBitmask);
        writeByte(buffer, lightBitmask);

        std::vector<uint8_t> uncompressed;
        int blockSectionCount = 0;
        int lightSectionCount = 0;
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) blockSectionCount++;
            if(lightBitmask & (1 << i)) lightSectionCount++;
        }

        const std::size_t uncompressedSize = blockSectionCount * (4096 + 2048)
            + lightSectionCount * (2048 + 2048);
        uncompressed.resize(uncompressedSize);
        uint8_t* out = uncompressed.data();

        const uint8_t* b = blockPtr ? blockPtr : blocks.data();
        const uint8_t* m = metaPtr ? metaPtr : metadata.data();
        const uint8_t* s = skyPtr ? skyPtr : skylight.data();
        const uint8_t* bl = blockLightPtr ? blockLightPtr : blocklight.data();

        // 1. Blocks
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    const uint8_t* src = b + (x << 11) + (z << 7) + i * 16;
                    std::memcpy(out, src, 16);
                    out += 16;
                }
            }
        }

        // 2. Metadata (already packed in memory)
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    const uint8_t* src = m + ((x << 11) + (z << 7) + i * 16) / 2;
                    std::memcpy(out, src, 8);
                    out += 8;
                }
            }
        }

        // 3. Skylight
        for(int i = 0; i < 8; ++i) {
            if(lightBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    const uint8_t* src = s + ((x << 11) + (z << 7) + i * 16) / 2;
                    std::memcpy(out, src, 8);
                    out += 8;
                }
            }
        }

        // 4. Blocklight
        for(int i = 0; i < 8; ++i) {
            if(lightBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    const uint8_t* src = bl + ((x << 11) + (z << 7) + i * 16) / 2;
                    std::memcpy(out, src, 8);
                    out += 8;
                }
            }
        }

        std::vector<uint8_t> compressed;
        Compression::compress(uncompressed, compressed);

        writeInt(buffer, (int32_t)compressed.size());
        writeBytes(buffer, compressed.data(), compressed.size());
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data);
        z = readInt(data);
        primaryBitmask = readByte(data);
        lightBitmask = readByte(data);

        int32_t compressedSize = readInt(data);
        const uint8_t* compressedPtr = data;

        int blockSectionCount = 0;
        int lightSectionCount = 0;
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) blockSectionCount++;
            if(lightBitmask & (1 << i)) lightSectionCount++;
        }

        size_t totalExpected = blockSectionCount * (4096 + 2048)
            + lightSectionCount * (2048 + 2048);
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
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    std::memcpy(blocks.data() + (x << 11) + (z << 7) + i * 16, ptr, 16);
                    ptr += 16;
                }
            }
        }

        // 2. Metadata
        for(int i = 0; i < 8; ++i) {
            if(primaryBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    std::memcpy(metadata.data() + ((x << 11) + (z << 7) + i * 16) / 2, ptr, 8);
                    ptr += 8;
                }
            }
        }

        // 3. Skylight
        for(int i = 0; i < 8; ++i) {
            if(lightBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    std::memcpy(skylight.data() + ((x << 11) + (z << 7) + i * 16) / 2, ptr, 8);
                    ptr += 8;
                }
            }
        }

        // 4. Blocklight
        for(int i = 0; i < 8; ++i) {
            if(lightBitmask & (1 << i)) {
                for (int x = 0; x < 16; ++x) for (int z = 0; z < 16; ++z) {
                    std::memcpy(blocklight.data() + ((x << 11) + (z << 7) + i * 16) / 2, ptr, 8);
                    ptr += 8;
                }
            }
        }
    }
};

enum class DiggingAction : uint8_t {
    START = 0,
    STOP = 1,
    FINISH = 2,
    DROP_ITEM = 3
};

class PacketPlayerDigging : public AutoPacket<PacketPlayerDigging> {
public:
    DiggingAction action;
    int32_t x, y, z;
    uint8_t face;

    PacketType getType() const override { return PacketType::PlayerDigging; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, (uint8_t)action);
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, face);
    }

    void readImpl(const uint8_t*& data) {
        action = (DiggingAction)readByte(data);
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        face = readByte(data);
    }
};

class PacketBlockPlacement : public AutoPacket<PacketBlockPlacement> {
public:
    int32_t x, y, z;
    uint8_t face;
    int32_t itemID;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::BlockPlacement; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, face);
        writeInt(buffer, itemID);
        writeByte(buffer, metadata);
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        face = readByte(data);
        itemID = readInt(data);
        metadata = readByte(data);
    }
};

class PacketBlockChange : public AutoPacket<PacketBlockChange> {
public:
    int32_t x, y, z;
    uint8_t blockID;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::BlockChange; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x);
        writeInt(buffer, y);
        writeInt(buffer, z);
        writeByte(buffer, blockID);
        writeByte(buffer, metadata);
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data);
        y = readInt(data);
        z = readInt(data);
        blockID = readByte(data);
        metadata = readByte(data);
    }
};

class PacketDestroyEntity : public AutoPacket<PacketDestroyEntity> {
public:
    int32_t id;

    PacketType getType() const override { return PacketType::DestroyEntity; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, id);
    }

    void readImpl(const uint8_t*& data) {
        id = readInt(data);
    }
};

class PacketCollectItem : public AutoPacket<PacketCollectItem> {
public:
    int32_t itemEntityID;
    int32_t collectorEntityID;

    PacketType getType() const override { return PacketType::CollectItem; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, itemEntityID);
        writeInt(buffer, collectorEntityID);
    }

    void readImpl(const uint8_t*& data) {
        itemEntityID = readInt(data);
        collectorEntityID = readInt(data);
    }
};

class PacketPlaySound : public AutoPacket<PacketPlaySound> {
public:
    std::string name;
    double x, y, z;
    float volume;
    float pitch;

    PacketType getType() const override { return PacketType::PlaySound; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeString(buffer, name);
        writeDouble(buffer, x);
        writeDouble(buffer, y);
        writeDouble(buffer, z);
        writeFloat(buffer, volume);
        writeFloat(buffer, pitch);
    }

    void readImpl(const uint8_t*& data) {
        name = readString(data);
        x = readDouble(data);
        y = readDouble(data);
        z = readDouble(data);
        volume = readFloat(data);
        pitch = readFloat(data);
    }
};

class PacketChunkRequest : public AutoPacket<PacketChunkRequest> {
public:
    int32_t x, z;

    PacketType getType() const override { return PacketType::ChunkRequest; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x);
        writeInt(buffer, z);
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data);
        z = readInt(data);
    }
};

class PacketChunkUnload : public AutoPacket<PacketChunkUnload> {
public:
    int32_t x, z;

    PacketType getType() const override { return PacketType::ChunkUnload; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x);
        writeInt(buffer, z);
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data);
        z = readInt(data);
    }
};

class PacketInventoryAdd : public AutoPacket<PacketInventoryAdd> {
public:
    int32_t itemID = 0;
    int32_t count = 0;
    uint8_t metadata = 0;

    PacketType getType() const override { return PacketType::InventoryAdd; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, itemID);
        writeInt(buffer, count);
        writeByte(buffer, metadata);
    }

    void readImpl(const uint8_t*& data) {
        itemID = readInt(data);
        count = readInt(data);
        metadata = readByte(data);
    }
};

class PacketWindowItems : public AutoPacket<PacketWindowItems> {
public:
    uint8_t windowId;
    struct Item { int32_t id; int32_t count; uint8_t metadata; };
    std::vector<Item> items;

    PacketType getType() const override { return PacketType::WindowItems; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, windowId);
        writeInt(buffer, (int32_t)items.size());
        for (const auto& item : items) {
            writeInt(buffer, item.id);
            writeInt(buffer, item.count);
            writeByte(buffer, item.metadata);
        }
    }

    void readImpl(const uint8_t*& data) {
        windowId = readByte(data);
        int32_t count = readInt(data);
        items.resize(count);
        for (int i = 0; i < count; ++i) {
            items[i].id = readInt(data);
            items[i].count = readInt(data);
            items[i].metadata = readByte(data);
        }
    }
};

class PacketOpenChest : public AutoPacket<PacketOpenChest> {
public:
    int32_t x, y, z;
    uint8_t rows = 3;
    std::vector<PacketWindowItems::Item> items;

    PacketType getType() const override { return PacketType::OpenChest; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, x); writeInt(buffer, y); writeInt(buffer, z);
        writeByte(buffer, rows);
        writeInt(buffer, (int32_t)items.size());
        for (const auto& item : items) {
            writeInt(buffer, item.id); writeInt(buffer, item.count); writeByte(buffer, item.metadata);
        }
    }

    void readImpl(const uint8_t*& data) {
        x = readInt(data); y = readInt(data); z = readInt(data);
        rows = readByte(data);
        int32_t count = readInt(data);
        items.resize(std::max(0, count));
        for (auto& item : items) {
            item.id = readInt(data); item.count = readInt(data); item.metadata = readByte(data);
        }
    }
};

class PacketSetSlot : public AutoPacket<PacketSetSlot> {
public:
    uint8_t windowId;
    int32_t slot;
    int32_t itemID;
    int32_t count;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::SetSlot; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, windowId);
        writeInt(buffer, slot);
        writeInt(buffer, itemID);
        writeInt(buffer, count);
        writeByte(buffer, metadata);
    }

    void readImpl(const uint8_t*& data) {
        windowId = readByte(data);
        slot = readInt(data);
        itemID = readInt(data);
        count = readInt(data);
        metadata = readByte(data);
    }
};

class PacketClickWindow : public AutoPacket<PacketClickWindow> {
public:
    uint8_t windowId;
    int32_t slot;
    uint8_t button;
    int16_t actionId;
    bool shift;
    int32_t itemID;
    int32_t count;
    uint8_t metadata;

    PacketType getType() const override { return PacketType::ClickWindow; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, windowId);
        writeInt(buffer, slot);
        writeByte(buffer, button);
        writeInt(buffer, (int32_t)actionId);
        writeByte(buffer, shift ? 1 : 0);
        writeInt(buffer, itemID);
        writeInt(buffer, count);
        writeByte(buffer, metadata);
    }

    void readImpl(const uint8_t*& data) {
        windowId = readByte(data);
        slot = readInt(data);
        button = readByte(data);
        actionId = (int16_t)readInt(data);
        shift = readByte(data) != 0;
        itemID = readInt(data);
        count = readInt(data);
        metadata = readByte(data);
    }
};

class PacketConfirmTransaction : public AutoPacket<PacketConfirmTransaction> {
public:
    uint8_t windowId;
    int16_t actionId;
    bool accepted;

    PacketType getType() const override { return PacketType::ConfirmTransaction; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, windowId);
        writeInt(buffer, (int32_t)actionId);
        writeByte(buffer, accepted ? 1 : 0);
    }

    void readImpl(const uint8_t*& data) {
        windowId = readByte(data);
        actionId = (int16_t)readInt(data);
        accepted = readByte(data) != 0;
    }
};

class PacketUpdateHealth : public AutoPacket<PacketUpdateHealth> {
public:
    int32_t health;
    int32_t maxHealth;

    PacketType getType() const override { return PacketType::UpdateHealth; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, health);
        writeInt(buffer, maxHealth);
    }

    void readImpl(const uint8_t*& data) {
        health = readInt(data);
        maxHealth = readInt(data);
    }
};

class PacketUseEntity : public AutoPacket<PacketUseEntity> {
public:
    int32_t userEntityID;
    int32_t targetEntityID;
    uint8_t leftClick;

    PacketType getType() const override { return PacketType::UseEntity; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, userEntityID);
        writeInt(buffer, targetEntityID);
        writeByte(buffer, leftClick);
    }

    void readImpl(const uint8_t*& data) {
        userEntityID = readInt(data);
        targetEntityID = readInt(data);
        leftClick = readByte(data);
    }
};

class PacketChatMessage : public AutoPacket<PacketChatMessage> {
public:
    std::string sender;
    std::string message;
    int64_t timestamp = 0;

    PacketType getType() const override { return PacketType::ChatMessage; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeString(buffer, sender);
        writeString(buffer, message);
        writeDouble(buffer, (double)timestamp);
    }

    void readImpl(const uint8_t*& data) {
        sender = readString(data);
        message = readString(data);
        timestamp = (int64_t)readDouble(data);
    }
};

class PacketGameModeChange : public AutoPacket<PacketGameModeChange> {
public:
    uint8_t gameMode;

    PacketType getType() const override { return PacketType::GameModeChange; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeByte(buffer, gameMode);
    }

    void readImpl(const uint8_t*& data) {
        gameMode = readByte(data);
    }
};

class PacketHeldItemChange : public AutoPacket<PacketHeldItemChange> {
public:
    int32_t slot;

    PacketType getType() const override { return PacketType::HeldItemChange; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, slot);
    }

    void readImpl(const uint8_t*& data) {
        slot = readInt(data);
    }
};

class PacketKeyResponse : public AutoPacket<PacketKeyResponse> {
public:
    std::string key;
    std::string uuid;
    std::string message;

    PacketType getType() const override { return PacketType::KeyResponse; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeString(buffer, key);
        writeString(buffer, uuid);
        writeString(buffer, message);
    }

    void readImpl(const uint8_t*& data) {
        key = readString(data);
        uuid = readString(data);
        message = readString(data);
    }
};

class PacketTimeUpdate : public AutoPacket<PacketTimeUpdate> {
public:
    double time;
    double timeOfDay;

    PacketType getType() const override { return PacketType::TimeUpdate; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeDouble(buffer, time);
        writeDouble(buffer, timeOfDay);
    }

    void readImpl(const uint8_t*& data) {
        time = readDouble(data);
        timeOfDay = readDouble(data);
    }
};

class PacketEntityHurt : public AutoPacket<PacketEntityHurt> {
public:
    int32_t entityID;
    int8_t damage;

    PacketType getType() const override { return PacketType::EntityHurt; }

    void writeImpl(std::vector<uint8_t>& buffer) const {
        writeInt(buffer, entityID);
        writeByte(buffer, (uint8_t)damage);
    }

    void readImpl(const uint8_t*& data) {
        entityID = readInt(data);
        damage = (int8_t)readByte(data);
    }
};
