#include "net/IntegratedServer.hpp"
#include "net/Packets.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>

IntegratedServer::IntegratedServer() {
    m_world = std::make_unique<World>();
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(-1));
    
    // Spawn a zombie near the start
    auto zombie = std::make_unique<EntityZombie>(*m_world);
    zombie->setPosition(16.0, 100.0, 16.0);
    m_world->spawnEntity(std::move(zombie));
}

IntegratedServer::~IntegratedServer() {
    stop();
}

void IntegratedServer::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&IntegratedServer::run, this);
}

void IntegratedServer::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void IntegratedServer::run() {
    m_server = std::make_unique<Server>(25565);
    m_server->onPacketReceived = [this](ENetPeer* peer, const uint8_t* data, size_t size) {
        this->onPacketReceived(peer, data, size);
    };
    m_server->onClientDisconnected = [this](ENetPeer* peer) {
        m_players.erase(peer);
    };

    auto lastTick = std::chrono::steady_clock::now();
    while (m_running) {
        m_world->pollGeneratedChunks();

        auto now = std::chrono::steady_clock::now();
        if (now - lastTick >= std::chrono::milliseconds(50)) { // 20 TPS
            // Request chunks around entities to ensure they don't fall through
            for (const auto& entity : m_world->getEntities()) {
                int cx = (int)std::floor(entity->posX / 16.0);
                int cz = (int)std::floor(entity->posZ / 16.0);
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dz = -1; dz <= 1; ++dz) {
                        m_world->requestChunk(cx + dx, cz + dz);
                    }
                }
            }

            m_world->update(0.05f);
            
            // Broadcast entity positions
            for (const auto& entity : m_world->getEntities()) {
                PacketMoveEntity move;
                move.id = entity->entityID;
                move.x = entity->posX;
                move.y = entity->posY;
                move.z = entity->posZ;
                move.yaw = entity->rotationYaw;
                move.pitch = entity->rotationPitch;
                m_server->broadcastPacket(move, false);
            }
            lastTick = now;
        }

        m_server->poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void IntegratedServer::onPacketReceived(ENetPeer* peer, const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);

    if (type == PacketType::Login) {
        PacketLogin packet;
        packet.deserialize(ptr, size - 1);
        std::cout << "Server: Player " << packet.username << " logged in." << std::endl;
        
        // Create server-side player entity
        auto player = std::make_unique<EntityPlayer>(*m_world);
        player->setPosition(8.0, 100.0, 8.0);
        EntityPlayer* pPtr = player.get();
        m_world->spawnEntity(std::move(player));
        
        int32_t eid = pPtr->entityID; 
        m_players[peer] = {eid, packet.username};

        PacketLoginResponse resp;
        resp.entityID = eid;
        m_server->sendPacket(peer, resp);

        // Send all existing entities to new player
        for (const auto& entity : m_world->getEntities()) {
            PacketSpawnEntity spawn;
            spawn.id = entity->entityID;
            spawn.type = dynamic_cast<EntityZombie*>(entity.get()) ? 1 : 0;
            spawn.x = entity->posX;
            spawn.y = entity->posY;
            spawn.z = entity->posZ;
            spawn.yaw = entity->rotationYaw;
            spawn.pitch = entity->rotationPitch;
            m_server->sendPacket(peer, spawn);
        }
    } else if (type == PacketType::PlayerPosition) {
        PacketPlayerPosition packet;
        packet.deserialize(ptr, size - 1);
        
        if (m_players.count(peer)) {
            int32_t eid = m_players[peer].entityID;
            for (auto& entity : m_world->getEntities()) {
                if (entity->entityID == eid) {
                    entity->setPosition(packet.x, packet.y, packet.z);
                    entity->rotationYaw = packet.yaw;
                    entity->rotationPitch = packet.pitch;
                    break;
                }
            }
        }
    }
}
