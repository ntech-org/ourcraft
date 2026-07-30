#include "net/IntegratedServer.hpp"
#include "net/ServerTick.hpp"
#include "net/Packets.hpp"
#include "net/ServerPacketHandler.hpp"
#include "util/Profiler.hpp"
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
#include <filesystem>

namespace fs = std::filesystem;

IntegratedServer::IntegratedServer(const std::string& worldName)
    : m_config("server.toml"),
      m_permissions("server_data"),
      m_registrationManager("server_data")
{
    m_config.load();

    if (!fs::exists("server_data")) fs::create_directories("server_data");

    m_world = std::make_unique<World>();
    m_world->initSaveHandler("worlds/" + worldName);
    m_permissions.load();
    m_registrationManager.load();

    LevelData levelData;
    auto saveHandler = m_world->getSaveHandler();
    if (saveHandler && saveHandler->loadLevelData(levelData)) {
        auto generator = std::make_unique<InfdevWorldGenerator>(levelData.seed);
        generator->setFarLands(levelData.farLands);
        m_world->setGenerator(std::move(generator));
        m_world->setWorldTime(levelData.time);
        if (levelData.spawnY > 100 || levelData.spawnY < 5) levelData.spawnY = 66;
    } else {
        int64_t seed = 1772835215;
        auto generator = std::make_unique<InfdevWorldGenerator>(seed);
        m_world->setGenerator(std::move(generator));
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
    OC_THREAD_NAME("Server");
    int port = m_config.getInt("port", 25565);
    if (m_isDedicated) {
        m_chunkKeepDistance.store(m_config.getInt("view-distance", 12), std::memory_order_release);
    }
    m_server = std::make_unique<Server>(port);
    if (!m_server->isValid()) {
        std::cerr << "[IntegratedServer] Failed to start server on port " << port << ". Server will not run." << std::endl;
        m_running = false;
        return;
    }
    m_packetHandler = std::make_unique<ServerPacketHandler>(*this, *m_world, *m_server, m_players, m_permissions, m_commandHandler, m_registrationManager);

    m_server->onPacketReceived = [this](ENetPeer* peer, const uint8_t* data, size_t size) {
        m_packetHandler->handle(peer, data, size);
    };
    m_server->onClientDisconnected = [this](ENetPeer* peer) {
        if (m_players.count(peer)) {
            saveAllPlayers(*m_world, *m_server, m_players);
            int32_t eid = m_players[peer].entityID;
            m_world->removeEntity(eid, false);
            PacketDestroyEntity destroy;
            destroy.id = eid;
            m_server->broadcastPacket(destroy, true);
            std::cout << "Server: Player entity " << m_players[peer].username << " (eid " << eid << ") removed." << std::endl;
            m_players.erase(peer);
        }
    };

    auto lastTick = std::chrono::steady_clock::now();
    constexpr auto tickInterval = std::chrono::milliseconds(50);

    while (m_running) {
        OC_ZONE_SCOPED_N("ServerLoop");
        auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick);
        const auto untilTick = m_paused ? tickInterval
            : (elapsed >= tickInterval ? std::chrono::milliseconds(0) : tickInterval - elapsed);
        const auto maxPoll = m_world->hasPendingChunkWork() ? std::chrono::milliseconds(5) : tickInterval;
        const auto pollTimeout = std::clamp(untilTick, std::chrono::milliseconds(0), maxPoll);
        {
            OC_ZONE_SCOPED_N("ServerPoll");
            m_server->poll(static_cast<uint32_t>(pollTimeout.count()));
        }
        {
            OC_ZONE_SCOPED_N("ServerPollChunks");
            m_world->pollGeneratedChunks();
        }

        now = std::chrono::steady_clock::now();
        if (m_paused) {
            lastTick = now;
        } else if (now - lastTick >= tickInterval) {
            tick();
            lastTick = now;
            OC_FRAME_MARK_NAMED("ServerTick");
        }
    }

    if (m_world) {
        m_world->saveAllChunks();
        saveAllPlayers(*m_world, *m_server, m_players);
    }
}

void IntegratedServer::tick() {
    OC_ZONE_SCOPED;
    m_tickCounter++;

    if (m_tickCounter % 6000 == 0) {
        saveAllPlayers(*m_world, *m_server, m_players);
    }

    std::unordered_map<int32_t, Entity*> entitiesById;
    entitiesById.reserve(m_world->getEntities().size());
    for (const auto& entity : m_world->getEntities()) {
        entitiesById[entity->entityID] = entity.get();
    }

    const bool chunksChanged = !m_world->popNewChunks().empty();
    const int chunkKeepDistance = m_chunkKeepDistance.load(std::memory_order_acquire);
    pushChunksToPlayers(*m_world, *m_server, m_players, entitiesById, chunkKeepDistance, chunksChanged);

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
        unloadFarChunks(*m_world, *m_server, m_players, entitiesById, chunkKeepDistance + 2);
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

    if (m_tickCounter % 20 == 0) {
        PacketTimeUpdate timePacket;
        timePacket.time = m_world->getWorldTime();
        timePacket.timeOfDay = std::fmod(m_world->getWorldTime(), 24000.0);
        m_server->broadcastPacket(timePacket, true);
    }

    if (++m_spawnTimer >= 20 * 20) {
        m_spawnTimer = 0;
        spawnMobs(*m_world, m_players, entitiesById);
    }
}
