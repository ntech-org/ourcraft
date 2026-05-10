#include "net/NetworkHandler.hpp"
#include "net/NetworkManager.hpp"
#include "net/Packets.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include <iostream>
#include <cstring>

NetworkHandler::NetworkHandler(World& world, EntityPlayer& player, bool startServer)
    : m_world(world), m_player(player)
{
    NetworkManager::init();

    if (startServer) {
        m_server = std::make_unique<IntegratedServer>();
        m_server->start();
    }

    m_client = std::make_unique<Client>();
    m_client->onPacketReceived = [this](const uint8_t* data, size_t size) {
        this->onPacketReceived(data, size);
    };
    m_client->onDisconnected = [this](bool timeout, const std::string& reason) {
        if (onDisconnected) onDisconnected(timeout, reason);
    };
    m_client->onConnected = [this]() {
        PacketLogin loginPacket;
        loginPacket.username = "Player";
        loginPacket.protocolVersion = 1;
        m_client->sendPacket(loginPacket);
    };
}

NetworkHandler::~NetworkHandler() {}

bool NetworkHandler::connect(const std::string& address, int port) {
    return m_client->connect(address, port);
}

void NetworkHandler::update() {
    m_client->poll();

    if (++m_posUpdateTimer >= 20) {
        // Keep-alive/Sync every second
        PacketPlayerRotation packet;
        packet.yaw = m_player.rotationYaw; packet.pitch = m_player.rotationPitch;
        packet.onGround = m_player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    }
}

void NetworkHandler::stopServer() {
    m_server.reset();
}

void NetworkHandler::setPaused(bool paused) {
    if (m_server) {
        m_server->setPaused(paused);
    }
}

bool NetworkHandler::isSingleplayer() const {
    return m_server != nullptr;
}

int NetworkHandler::getPlayerCount() const {
    return m_server ? m_server->getPlayerCount() : 0;
}

void NetworkHandler::sendPlayerPosition(const EntityPlayer& player) {
    double dx = player.posX - m_lastX;
    double dy = player.posY - m_lastY;
    double dz = player.posZ - m_lastZ;
    float dYaw = player.rotationYaw - m_lastYaw;
    float dPitch = player.rotationPitch - m_lastPitch;

    bool moved = (dx * dx + dy * dy + dz * dz) > 9e-4; // 0.03 blocks
    bool turned = std::abs(dYaw) > 0.1f || std::abs(dPitch) > 0.1f;

    if (moved && turned) {
        PacketPlayerPosLook packet;
        packet.x = player.posX; packet.y = player.posY; packet.z = player.posZ;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch;
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0; // Reset timer since we just sent a packet
    } else if (moved) {
        PacketPlayerPosition packet;
        packet.x = player.posX; packet.y = player.posY; packet.z = player.posZ;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch; // Fallback for old servers
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    } else if (turned) {
        PacketPlayerRotation packet;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch;
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    }

    if (moved) { m_lastX = player.posX; m_lastY = player.posY; m_lastZ = player.posZ; }
    if (turned) { m_lastYaw = player.rotationYaw; m_lastPitch = player.rotationPitch; }
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

void NetworkHandler::sendPacket(const Packet& packet) {
    if (m_client) {
        m_client->sendPacket(packet);
    }
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
        } else if (packet.type == 2) {
            auto item = std::make_unique<EntityItem>(m_world, packet.dataA, packet.dataB, packet.dataC);
            item->pickupDelay = 0;
            item->handlePhysics = false;
            entity = std::move(item);
        } else {
            entity = std::make_unique<EntityPlayer>(m_world);
        }
        entity->entityID = packet.id;
        entity->setPosAndPrev(packet.x, packet.y, packet.z);
        entity->rotationYaw = packet.yaw;
        entity->rotationPitch = packet.pitch;
        entity->handlePhysics = false;

        entity->serverPosX = packet.x;
        entity->serverPosY = packet.y;
        entity->serverPosZ = packet.z;
        entity->serverYaw = packet.yaw;
        entity->serverPitch = packet.pitch;

        m_world.spawnEntity(std::move(entity));
    } else if (type == PacketType::MoveEntity) {
        PacketMoveEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.id == m_playerID) return;

        for (auto& entity : m_world.getEntities()) {
            if (entity->entityID == packet.id) {
                entity->serverPosX = packet.x;
                entity->serverPosY = packet.y;
                entity->serverPosZ = packet.z;
                entity->serverYaw = packet.yaw;
                entity->serverPitch = packet.pitch;
                entity->posRotationIncrements = 3;
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
        } else {
            chunk->setLightWipeComplete(true);
        }

        chunk->setState(ChunkState::Complete);
        chunk->generateHeightMap();
        chunk->generateBitmask();

        if (isNew) {
            m_world.addChunk(chunk);
        }

        // Cache neighbors to avoid repeated lookups in the loop
        auto nW = m_world.getChunk(packet.x - 1, packet.z);
        auto nE = m_world.getChunk(packet.x + 1, packet.z);
        auto nN = m_world.getChunk(packet.x, packet.z - 1);
        auto nS = m_world.getChunk(packet.x, packet.z + 1);

        // Mark all sections as dirty so they rebuild meshes
        for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
            chunk->markSectionDirtyInternal(i);

            // Also touch neighbors to fix seams/lighting at boundaries
            if (nW) nW->touchSection(i);
            if (nE) nE->touchSection(i);
            if (nN) nN->touchSection(i);
            if (nS) nS->touchSection(i);
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
    } else if (type == PacketType::PlayerPosLook) {
        PacketPlayerPosLook packet;
        packet.deserialize(ptr, size - 1);
        m_player.setPosition(packet.x, packet.y, packet.z);
        m_player.rotationYaw = packet.yaw;
        m_player.rotationPitch = packet.pitch;
        m_player.onGround = packet.onGround;

        m_lastX = packet.x; m_lastY = packet.y; m_lastZ = packet.z;
        m_lastYaw = packet.yaw; m_lastPitch = packet.pitch;
    } else if (type == PacketType::InventoryAdd) {
        PacketInventoryAdd packet;
        packet.deserialize(ptr, size - 1);
        m_player.inventory.addItem(packet.itemID, packet.count, packet.metadata);
    } else if (type == PacketType::WindowItems) {
        PacketWindowItems packet;
        packet.deserialize(ptr, size - 1);
        if (packet.windowId == 0) {
            for (size_t i = 0; i < packet.items.size() && i < InventoryPlayer::TOTAL_SIZE; ++i) {
                m_player.inventory.mainInventory[i] = {packet.items[i].id, (int)packet.items[i].count, packet.items[i].metadata};
            }
        }
    } else if (type == PacketType::SetSlot) {
        PacketSetSlot packet;
        packet.deserialize(ptr, size - 1);
        if (packet.windowId == 0) {
            if (packet.slot == -1) {
                m_player.inventory.cursorStack = {packet.itemID, packet.count, packet.metadata};
            } else if (packet.slot >= 0 && packet.slot < InventoryPlayer::TOTAL_SIZE) {
                m_player.inventory.mainInventory[packet.slot] = {packet.itemID, packet.count, packet.metadata};
            }
        }
    }
 else if (type == PacketType::ConfirmTransaction) {
        PacketConfirmTransaction packet;
        packet.deserialize(ptr, size - 1);
        // If rejected, the server should follow up with SetSlot/WindowItems to correct the client.
    }
}
