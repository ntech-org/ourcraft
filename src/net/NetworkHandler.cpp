#include "net/NetworkHandler.hpp"
#include "net/NetworkManager.hpp"
#include "net/Packets.hpp"
#include "entities/EntityZombie.hpp"
#include <stdexcept>
#include <cstring>

NetworkHandler::NetworkHandler(World& world, EntityPlayer& player)
    : m_world(world), m_player(player) 
{
    NetworkManager::init();
    m_server = std::make_unique<IntegratedServer>();
    m_server->start();

    m_client = std::make_unique<Client>();
    m_client->onPacketReceived = [this](const uint8_t* data, size_t size) {
        this->onPacketReceived(data, size);
    };
}

NetworkHandler::~NetworkHandler() {}

bool NetworkHandler::connect(const std::string& address, int port) {
    if (!m_client->connect(address, port)) return false;

    PacketLogin loginPacket;
    loginPacket.username = "Player";
    loginPacket.protocolVersion = 1;
    m_client->sendPacket(loginPacket);
    return true;
}

void NetworkHandler::update() {
    m_client->poll();
}

void NetworkHandler::stopServer() {
    m_server.reset();
}

void NetworkHandler::sendPlayerPosition(const EntityPlayer& player) {
    PacketPlayerPosition posPacket;
    posPacket.x = player.posX;
    posPacket.y = player.posY;
    posPacket.z = player.posZ;
    posPacket.yaw = player.rotationYaw;
    posPacket.pitch = player.rotationPitch;
    posPacket.onGround = player.onGround;
    m_client->sendPacket(posPacket, false);
}

void NetworkHandler::sendDigging(DiggingAction action, int x, int y, int z, int face) {
    PacketPlayerDigging packet;
    packet.action = action;
    packet.x = x; packet.y = y; packet.z = z;
    packet.face = (uint8_t)face;
    m_client->sendPacket(packet, true);
}

void NetworkHandler::sendPlacement(int x, int y, int z, int face, int id, int meta) {
    PacketBlockPlacement packet;
    packet.x = x; packet.y = y; packet.z = z;
    packet.face = (uint8_t)face;
    packet.blockID = (uint8_t)id;
    packet.metadata = (uint8_t)meta;
    m_client->sendPacket(packet, true);
}

void NetworkHandler::onPacketReceived(const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);

    if (type == PacketType::LoginResponse) {
        PacketLoginResponse packet;
        packet.deserialize(ptr, size - 1);
        m_playerID = packet.entityID;
        m_player.entityID = m_playerID;
    } else if (type == PacketType::SpawnEntity) {
        PacketSpawnEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.id == m_playerID) return;

        std::unique_ptr<Entity> entity;
        if (packet.type == 1) {
            entity = std::make_unique<EntityZombie>(m_world);
        } else {
            entity = std::make_unique<EntityPlayer>(m_world);
        }
        entity->entityID = packet.id;
        entity->setPosition(packet.x, packet.y, packet.z);
        entity->rotationYaw = packet.yaw;
        entity->rotationPitch = packet.pitch;
        entity->handlePhysics = false;
        m_world.spawnEntity(std::move(entity));
    } else if (type == PacketType::MoveEntity) {
        PacketMoveEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.id == m_playerID) return;

        for (auto& entity : m_world.getEntities()) {
            if (entity->entityID == packet.id) {
                entity->prevPosX = entity->posX;
                entity->prevPosY = entity->posY;
                entity->prevPosZ = entity->posZ;
                entity->prevRotationYaw = entity->rotationYaw;
                entity->prevRotationPitch = entity->rotationPitch;

                entity->posX = packet.x; entity->posY = packet.y; entity->posZ = packet.z;
                entity->rotationYaw = packet.yaw; entity->rotationPitch = packet.pitch;
                float w2 = entity->width / 2.0f;
                entity->boundingBox = AxisAlignedBB(entity->posX - w2, entity->posY, entity->posZ - w2,
                                                   entity->posX + w2, entity->posY + entity->height, entity->posZ + w2);
                break;
            }
        }
    } else if (type == PacketType::ChunkData) {
        PacketChunkData packet;
        packet.deserialize(ptr, size - 1);
        
        std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(packet.x)) << 32) | static_cast<std::uint32_t>(packet.z);
        m_world.m_pendingRequests.erase(key);
        m_world.m_pendingChunks.erase(key);

        std::shared_ptr<Chunk> chunk = m_world.getChunk(packet.x, packet.z);
        bool isNew = false;
        if (!chunk) {
            chunk = std::make_shared<Chunk>(packet.x, packet.z);
            isNew = true;
        }

        std::memcpy(chunk->getBlocks(), packet.blocks.data(), packet.blocks.size());
        std::memcpy(chunk->getMetadata(), packet.metadata.data(), packet.metadata.size());
        std::memcpy(chunk->getSkylight(), packet.skylight.data(), packet.skylight.size());
        std::memcpy(chunk->getBlocklight(), packet.blocklight.data(), packet.blocklight.size());
        
        bool hasLight = false;
        for (uint8_t b : packet.skylight) if (b != 0) { hasLight = true; break; }
        if (!hasLight) {
            for (uint8_t b : packet.blocklight) if (b != 0) { hasLight = true; break; }
        }

        if (!hasLight) {
            m_world.predictLighting(*chunk);
        }

        chunk->setState(ChunkState::Complete);
        chunk->generateHeightMap();
        chunk->generateBitmask();
        
        if (isNew) {
            m_world.addChunk(chunk);
        }
        
        // Mark all sections as dirty so they rebuild meshes
        for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
            chunk->markSectionDirtyInternal(i);
            
            // Also touch neighbors to fix seams/lighting at boundaries
            if (auto n = m_world.getChunk(packet.x - 1, packet.z)) n->touchSection(i);
            if (auto n = m_world.getChunk(packet.x + 1, packet.z)) n->touchSection(i);
            if (auto n = m_world.getChunk(packet.x, packet.z - 1)) n->touchSection(i);
            if (auto n = m_world.getChunk(packet.x, packet.z + 1)) n->touchSection(i);
        }
    } else if (type == PacketType::BlockChange) {
        PacketBlockChange packet;
        packet.deserialize(ptr, size - 1);
        m_world.setBlockAndMetadataWithNotify(packet.x, packet.y, packet.z, packet.blockID, packet.metadata);
    } else if (type == PacketType::DestroyEntity) {
        PacketDestroyEntity packet;
        packet.deserialize(ptr, size - 1);
        m_world.removeEntity(packet.id);
    } else if (type == PacketType::ChunkUnload) {
        PacketChunkUnload packet;
        packet.deserialize(ptr, size - 1);
        m_world.removeChunk(packet.x, packet.z);
    }
}
