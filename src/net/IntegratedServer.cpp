#include "net/IntegratedServer.hpp"
#include "net/ServerTick.hpp"
#include "net/Packets.hpp"
#include "net/ServerPacketHandler.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "items/ItemTool.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unordered_map>

IntegratedServer::IntegratedServer() : m_permissions("world") {
    m_world = std::make_unique<World>();
    m_world->initSaveHandler("world");
    m_permissions.load();

    LevelData levelData;
    auto saveHandler = m_world->getSaveHandler();
    if (saveHandler && saveHandler->loadLevelData(levelData)) {
        m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(levelData.seed));
        m_world->setWorldTime(levelData.time);
        if (levelData.spawnY > 100 || levelData.spawnY < 5) levelData.spawnY = 66;
    } else {
        int64_t seed = 1772835215;
        m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(seed));
        if (saveHandler) {
            levelData.seed = seed;
            levelData.spawnX = 0; levelData.spawnY = 66; levelData.spawnZ = 0;
            levelData.time = 6000;
            saveHandler->saveLevelData(levelData);
        }
    }

    m_world->onBlockChanged = [this](int x, int y, int z, uint8_t id, uint8_t meta) {
        PacketBlockChange packet;
        packet.x = x; packet.y = y; packet.z = z;
        packet.blockID = id; packet.metadata = meta;
        if (m_server) m_server->broadcastPacket(packet, true);
    };

    auto zombie = std::make_unique<EntityZombie>(*m_world);
    zombie->setPosition(16.0, 100.0, 16.0);
    m_world->spawnEntity(std::move(zombie));
}

IntegratedServer::~IntegratedServer() {
    stop();
}

void IntegratedServer::broadcastSound(const std::string& name, double x, double y, double z, float volume, float pitch, ENetPeer* excludePeer) {
    if (!m_server) return;
    PacketPlaySound packet;
    packet.name = name;
    packet.x = x; packet.y = y; packet.z = z;
    packet.volume = volume; packet.pitch = pitch;
    m_server->broadcastPacket(packet, false, excludePeer);
}

void IntegratedServer::broadcastChat(const std::string& sender, const std::string& message, ENetPeer* excludePeer) {
    if (!m_server) return;
    PacketChatMessage packet;
    packet.sender = sender;
    packet.message = message;
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    m_server->broadcastPacket(packet, true, excludePeer);
}

void IntegratedServer::start() {
    if (m_running) return;
    m_running = true;
    if (m_isDedicated) {
        run();
    } else {
        m_thread = std::thread(&IntegratedServer::run, this);
    }
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
    m_packetHandler = std::make_unique<ServerPacketHandler>(*this, *m_world, *m_server, m_players, m_permissions, m_commandHandler);

    m_server->onPacketReceived = [this](ENetPeer* peer, const uint8_t* data, size_t size) {
        m_packetHandler->handle(peer, data, size);
    };
    m_server->onClientDisconnected = [this](ENetPeer* peer) {
        m_players.erase(peer);
    };

    auto lastTick = std::chrono::steady_clock::now();

    while (m_running) {
        m_server->poll();
        m_world->pollGeneratedChunks();

        auto now = std::chrono::steady_clock::now();
        if (m_paused) {
            lastTick = now;
        } else if (now - lastTick >= std::chrono::milliseconds(50)) {
            tick();
            lastTick = now;
        }

        m_server->poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (m_world) {
        m_world->saveAllChunks();
        saveAllPlayers(*m_world, *m_server, m_players);
    }
}

void IntegratedServer::tick() {
    m_tickCounter++;

    if (m_tickCounter % 6000 == 0) {
        saveAllPlayers(*m_world, *m_server, m_players);
    }

    std::unordered_map<int32_t, Entity*> entitiesById;
    entitiesById.reserve(m_world->getEntities().size());
    for (const auto& entity : m_world->getEntities()) {
        entitiesById[entity->entityID] = entity.get();
    }

    pushChunksToPlayers(*m_world, *m_server, m_players, entitiesById);
    m_world->popNewChunks();

    auto removedEntities = m_world->popRemovedEntities();
    for (int32_t id : removedEntities) {
        PacketDestroyEntity packet;
        packet.id = id;
        m_server->broadcastPacket(packet, true);
        for (auto& [peer, session] : m_players) {
            session.sentEntities.erase(id);
        }
    }

    if (++m_unloadTimer >= 20 * 10) {
        m_unloadTimer = 0;
        unloadFarChunks(*m_world, *m_server, m_players, entitiesById, m_chunkKeepDistance);
    }

    broadcastEntityPositions(*m_world, *m_server);
    m_world->update(0.05f);

    // Server-side void protection
    for (auto& entity : m_world->getEntities()) {
        if (entity->posY < -64.0) {
            entity->setPosition(entity->posX, 66.0, entity->posY);
            entity->motionY = 0.0;
            entity->fallDistance = 0.0f;
            if (auto* living = dynamic_cast<EntityLiving*>(entity.get())) {
                if (living->health < living->maxHealth / 2) {
                    living->health = living->maxHealth;
                }
            }
        }
    }

    // Sync health to client
    for (auto& [peer, session] : m_players) {
        for (const auto& entity : m_world->getEntities()) {
            if (entity->entityID == session.entityID) {
                if (auto* living = dynamic_cast<EntityLiving*>(entity.get())) {
                    if (living->health != session.lastHealth) {
                        session.lastHealth = living->health;
                        PacketUpdateHealth hpPacket;
                        hpPacket.health = living->health;
                        hpPacket.maxHealth = living->maxHealth;
                        m_server->sendPacket(peer, hpPacket, true);
                    }
                }
                break;
            }
        }
    }

    handleRespawns(*m_world, *m_server, m_players);
    handleItemPickups(*m_world, *m_server, m_players, entitiesById);
    spawnNewEntities(*m_world, *m_server, m_players);

    if (++m_spawnTimer >= 20 * 20) {
        m_spawnTimer = 0;
        spawnMobs(*m_world, m_players, entitiesById);
    }
}
