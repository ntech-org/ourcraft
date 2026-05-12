#include "net/IntegratedServer.hpp"
#include "net/ServerTick.hpp"
#include "net/Packets.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unordered_map>

IntegratedServer::IntegratedServer() {
    m_world = std::make_unique<World>();
    m_world->initSaveHandler("world");

    LevelData levelData;
    auto saveHandler = m_world->getSaveHandler();
    if (saveHandler && saveHandler->loadLevelData(levelData)) {
        m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(levelData.seed));
        m_world->setWorldTime(levelData.time);
    } else {
        int64_t seed = 1772835215;
        m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(seed));
        if (saveHandler) {
            levelData.seed = seed;
            levelData.spawnX = 0; levelData.spawnY = 128; levelData.spawnZ = 0;
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
    m_server->onPacketReceived = [this](ENetPeer* peer, const uint8_t* data, size_t size) {
        this->onPacketReceived(peer, data, size);
    };
    m_server->onClientDisconnected = [this](ENetPeer* peer) {
        m_players.erase(peer);
    };

    auto lastTick = std::chrono::steady_clock::now();
    int tickCounter = 0;
    int unloadTimer = 0;
    int spawnTimer = 0;

    while (m_running) {
        m_server->poll();
        m_world->pollGeneratedChunks();

        auto now = std::chrono::steady_clock::now();
        if (m_paused) {
            lastTick = now;
        } else if (now - lastTick >= std::chrono::milliseconds(50)) {
            tickCounter++;
            if (tickCounter % 6000 == 0) {
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

            if (++unloadTimer >= 20 * 10) {
                unloadTimer = 0;
                unloadFarChunks(*m_world, *m_server, m_players, entitiesById);
            }

            broadcastEntityPositions(*m_world, *m_server);
            m_world->update(0.05f);
            handleRespawns(*m_world, *m_server, m_players);
            handleItemPickups(*m_world, *m_server, m_players, entitiesById);
            spawnNewEntities(*m_world, *m_server, m_players);

            if (++spawnTimer >= 20 * 20) {
                spawnTimer = 0;
                spawnMobs(*m_world, m_players, entitiesById);
            }

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

void IntegratedServer::onPacketReceived(ENetPeer* peer, const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);

    if (type == PacketType::Login) {
        PacketLogin packet;
        packet.deserialize(ptr, size - 1);
        std::cout << "Server: Player " << packet.username << " logged in." << std::endl;

        auto player = std::make_unique<EntityPlayer>(*m_world);

        PlayerSaveData pData;
        auto saveHandler = m_world->getSaveHandler();
        if (saveHandler && saveHandler->loadPlayerData(packet.username, pData)) {
            std::cout << "Server: Loaded player data for " << packet.username << " at (" << pData.x << ", " << pData.y << ", " << pData.z << ")" << std::endl;
            player->setPosition(pData.x, pData.y, pData.z);
            player->rotationYaw = pData.yaw;
            player->rotationPitch = pData.pitch;
            player->health = pData.health;
            for (int i = 0; i < 45; ++i) player->inventory.mainInventory[i] = pData.inventory[i];
        } else {
            std::cout << "Server: No save data found for " << packet.username << ", using world spawn." << std::endl;
            LevelData levelData;
            if (saveHandler && saveHandler->loadLevelData(levelData)) {
                player->setPosition(levelData.spawnX, levelData.spawnY, levelData.spawnZ);
            } else {
                player->setPosition(0.0, 128.0, 0.0);
            }
        }

        EntityPlayer* pPtr = player.get();
        m_world->spawnEntity(std::move(player));

        int32_t eid = pPtr->entityID;
        m_players[peer] = {eid, packet.username, {}, {}};
        PlayerSession& session = m_players[peer];

        for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) {
            session.inventory.mainInventory[i] = pPtr->inventory.mainInventory[i];
        }

        PacketLoginResponse resp;
        resp.entityID = eid;
        m_server->sendPacket(peer, resp);

        PacketWindowItems invPacket;
        invPacket.windowId = 0;
        for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) {
            invPacket.items.push_back({session.inventory.mainInventory[i].itemID,
                                     session.inventory.mainInventory[i].count,
                                     session.inventory.mainInventory[i].metadata});
        }
        m_server->sendPacket(peer, invPacket);

        PacketPlayerPosLook posPacket;
        posPacket.x = pPtr->posX; posPacket.y = pPtr->posY; posPacket.z = pPtr->posZ;
        posPacket.yaw = pPtr->rotationYaw; posPacket.pitch = pPtr->rotationPitch;
        posPacket.onGround = pPtr->onGround;
        m_server->sendPacket(peer, posPacket);

        for (const auto& entity : m_world->getEntities()) {
            PacketSpawnEntity spawn;
            spawn.id = entity->entityID;
            spawn.type = dynamic_cast<EntityZombie*>(entity.get()) ? 1 : (dynamic_cast<EntityItem*>(entity.get()) ? 2 : 0);
            spawn.x = entity->posX;
            spawn.y = entity->posY;
            spawn.z = entity->posZ;
            spawn.yaw = entity->rotationYaw;
            spawn.pitch = entity->rotationPitch;
            if (auto* item = dynamic_cast<EntityItem*>(entity.get())) {
                spawn.dataA = item->itemID;
                spawn.dataB = item->count;
                spawn.dataC = item->metadata;
            }
            m_server->sendPacket(peer, spawn);
            m_players[peer].sentEntities.insert(entity->entityID);
        }
    } else if (type == PacketType::PlayerPosition || type == PacketType::PlayerRotation || type == PacketType::PlayerPosLook) {
        if (m_players.count(peer)) {
            int32_t eid = m_players[peer].entityID;
            for (auto& entity : m_world->getEntities()) {
                if (entity->entityID == eid) {
                    if (type == PacketType::PlayerPosition || type == PacketType::PlayerPosLook) {
                        PacketPlayerPosition p;
                        p.deserialize(ptr, size - 1);
                        entity->setPosition(p.x, p.y, p.z);
                        if (type == PacketType::PlayerPosLook) {
                            entity->rotationYaw = p.yaw;
                            entity->rotationPitch = p.pitch;
                        }
                    } else if (type == PacketType::PlayerRotation) {
                        PacketPlayerRotation p;
                        p.deserialize(ptr, size - 1);
                        entity->rotationYaw = p.yaw;
                        entity->rotationPitch = p.pitch;
                    }
                    break;
                }
            }
        }
    } else if (type == PacketType::PlayerDigging) {
        PacketPlayerDigging packet;
        packet.deserialize(ptr, size - 1);
        if (packet.action == DiggingAction::STOP) {
        } else if (packet.action == DiggingAction::FINISH) {
            const uint8_t oldID = m_world->getBlockID(packet.x, packet.y, packet.z);
            const uint8_t oldMeta = m_world->getBlockMetadata(packet.x, packet.y, packet.z);
            m_world->setBlockWithNotify(packet.x, packet.y, packet.z, 0);
            if (oldID > 0 && Block::getHardness(oldID) >= 0.0f) {
                auto item = std::make_unique<EntityItem>(*m_world, oldID, 1, oldMeta);
                double spread = 0.7;
                item->setPosition(
                    packet.x + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5,
                    packet.y + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5,
                    packet.z + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5
                );
                item->pickupDelay = 6;
                m_world->spawnEntity(std::move(item));
            }
        }
    } else if (type == PacketType::BlockPlacement) {
        PacketBlockPlacement packet;
        packet.deserialize(ptr, size - 1);
        int x = packet.x, y = packet.y, z = packet.z;
        if (packet.face == 0) y--; else if (packet.face == 1) y++;
        else if (packet.face == 2) z--; else if (packet.face == 3) z++;
        else if (packet.face == 4) x--; else if (packet.face == 5) x++;
        m_world->setBlockAndMetadataWithNotify(x, y, z, packet.blockID, packet.metadata);

        if (m_players.count(peer)) {
            PlayerSession& session = m_players[peer];
            session.inventory.consumeCurrentItem(1);
        }
    } else if (type == PacketType::ChunkRequest) {
        PacketChunkRequest packet;
        packet.deserialize(ptr, size - 1);
        m_world->requestChunk(packet.x, packet.z);
    } else if (type == PacketType::ChunkUnload) {
    } else if (type == PacketType::ClickWindow) {
        PacketClickWindow packet;
        packet.deserialize(ptr, size - 1);
        if (m_players.count(peer)) {
            PlayerSession& session = m_players[peer];
            session.inventory.handleClick(packet.slot, packet.button != 0);
            PacketConfirmTransaction resp;
            resp.windowId = packet.windowId;
            resp.actionId = packet.actionId;
            resp.accepted = true;
            m_server->sendPacket(peer, resp, true);
        }
    } else if (type == PacketType::UseEntity) {
        PacketUseEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.leftClick && m_players.count(peer)) {
            Entity* target = nullptr;
            for (auto& entity : m_world->getEntities()) {
                if (entity->entityID == packet.targetEntityID) {
                    target = entity.get();
                    break;
                }
            }

            if (target) {
                int damage = 1;
                auto& session = m_players[peer];
                int itemID = session.inventory.getCurrentItemID();

                if (itemID == 268) damage = 4;
                else if (itemID == 272) damage = 5;
                else if (itemID == 267) damage = 6;
                else if (itemID == 283) damage = 5;
                else if (itemID == 276) damage = 7;
                else if (itemID >= 270 && itemID <= 279) damage = 2;

                if (auto* living = dynamic_cast<EntityLiving*>(target)) {
                    living->attackEntityFrom(nullptr, damage);
                }
            }
        }
    }
}
