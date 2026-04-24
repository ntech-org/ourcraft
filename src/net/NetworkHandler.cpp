#include "net/NetworkHandler.hpp"
#include "net/NetworkManager.hpp"
#include "net/Packets.hpp"
#include "entities/EntityZombie.hpp"
#include <stdexcept>

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
                entity->posX = packet.x; entity->posY = packet.y; entity->posZ = packet.z;
                entity->rotationYaw = packet.yaw; entity->rotationPitch = packet.pitch;
                float w2 = entity->width / 2.0f;
                entity->boundingBox = AxisAlignedBB(entity->posX - w2, entity->posY, entity->posZ - w2,
                                                   entity->posX + w2, entity->posY + entity->height, entity->posZ + w2);
                break;
            }
        }
    }
}
