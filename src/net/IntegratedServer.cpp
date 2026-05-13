#include "net/IntegratedServer.hpp"
#include "net/ServerTick.hpp"
#include "net/Packets.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityLiving.hpp"
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
            // Clamp old saves that used y=128 as spawn
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

void IntegratedServer::broadcastSound(const std::string& name, double x, double y, double z, float volume, float pitch, ENetPeer* excludePeer) {
    if (!m_server) return;
    PacketPlaySound packet;
    packet.name = name;
    packet.x = x; packet.y = y; packet.z = z;
    packet.volume = volume; packet.pitch = pitch;
    m_server->broadcastPacket(packet, false, excludePeer);
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
            // Sync health to client whenever it changes (server is authoritative)
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
        std::cout << "Server: Player " << packet.username << " (" << packet.uuid << ") logged in." << std::endl;

        auto player = std::make_unique<EntityPlayer>(*m_world);
        player->username = packet.username;
        player->uuid = packet.uuid;

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
                if (levelData.spawnY > 100 || levelData.spawnY < 5) levelData.spawnY = 66;
                player->setPosition(levelData.spawnX, levelData.spawnY, levelData.spawnZ);
            } else {
                player->setPosition(0.0, 66.0, 0.0);
            }
        }

        EntityPlayer* pPtr = player.get();
        player->onPlaySound = [this, peer, pPtr](const std::string& name, float vol, float pitch) {
            broadcastSound(name, pPtr->posX, pPtr->posY, pPtr->posZ, vol, pitch, peer);
        };
        m_world->spawnEntity(std::move(player));

        int32_t eid = pPtr->entityID;
        m_players[peer] = {eid, packet.username, packet.uuid, {}, {}, pPtr->posX, pPtr->posY, pPtr->posZ, 0.0f, false, {}};
        PlayerSession& session = m_players[peer];
        session.lastSentY = pPtr->posY;

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

        session.lastHealth = pPtr->health;

        PacketUpdateHealth hpPacket;
        hpPacket.health = pPtr->health;
        hpPacket.maxHealth = pPtr->maxHealth;
        m_server->sendPacket(peer, hpPacket, true);

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
            } else if (auto* p = dynamic_cast<EntityPlayer*>(entity.get())) {
                spawn.username = p->username;
                spawn.uuid = p->uuid;
            }
            m_server->sendPacket(peer, spawn);
            m_players[peer].sentEntities.insert(entity->entityID);
        }
    } else if (type == PacketType::PlayerPosition || type == PacketType::PlayerRotation || type == PacketType::PlayerPosLook) {
        if (m_players.count(peer)) {
            PlayerSession& session = m_players[peer];
            int32_t eid = session.entityID;
            for (auto& entity : m_world->getEntities()) {
                if (entity->entityID == eid) {
                    if (type == PacketType::PlayerPosition || type == PacketType::PlayerPosLook) {
                        PacketPlayerPosition p;
                        p.deserialize(ptr, size - 1);
                        
                        // Track fall distance from client position packets
                        // (independent of server physics simulation)
                        double yDiff = p.y - session.lastSentY;
                        if (yDiff < -0.001) {
                            // Falling: accumulate distance
                            session.accumulatedFall += (float)(-yDiff);
                        } else {
                            // Stationary or moving up: apply pending fall damage
                            if (session.accumulatedFall > 3.0f) {
                                int dmg = (int)std::ceil(session.accumulatedFall - 3.0f);
                                if (auto* living = dynamic_cast<EntityLiving*>(entity.get())) {
                                    living->attackEntityFrom(nullptr, dmg);
                                }
                            }
                            session.accumulatedFall = 0.0f;
                        }
                        session.lastSentY = p.y;
                        entity->fallDistance = 0.0f;
                        entity->onGround = (std::abs(yDiff) < 0.001);
                        
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
            
            if (const Block* b = Block::blocksList[oldID]) {
                broadcastSound(b->stepSound->getBreakSound(), packet.x + 0.5, packet.y + 0.5, packet.z + 0.5, 1.0f, 1.0f, peer);
            }

            if (oldID > 0 && Block::getHardness(oldID) >= 0.0f) {
                int dropID = oldID;
                int dropCount = 1;
                if (const Block* b = Block::blocksList[oldID]) {
                    dropID = b->idDropped(oldMeta);
                    dropCount = b->quantityDropped();
                }
                if (dropID > 0 && dropCount > 0) {
                    auto item = std::make_unique<EntityItem>(*m_world, dropID, dropCount, 0);
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
        }
    } else if (type == PacketType::BlockPlacement) {
        PacketBlockPlacement packet;
        packet.deserialize(ptr, size - 1);
        int x = packet.x, y = packet.y, z = packet.z;
        if (packet.face == 0) y--; else if (packet.face == 1) y++;
        else if (packet.face == 2) z--; else if (packet.face == 3) z++;
        else if (packet.face == 4) x--; else if (packet.face == 5) x++;
        m_world->setBlockAndMetadataWithNotify(x, y, z, packet.blockID, packet.metadata);

        if (const Block* b = Block::blocksList[packet.blockID]) {
            broadcastSound(b->stepSound->getBreakSound(), x + 0.5, y + 0.5, z + 0.5, 1.0f, 0.8f, peer);
        }

        // Client-side handles inventory consumption for placement
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

            // Sync back affected slots (like result slots or if server logic differs)
            // For now, let's sync the clicked slot and the result slots to be safe.
            std::vector<int> slotsToSync = { packet.slot, InventoryPlayer::RESULT_SLOT, InventoryPlayer::WORKBENCH_RESULT };
            for (int s : slotsToSync) {
                if (s < 0 || s >= InventoryPlayer::TOTAL_SIZE) continue;
                PacketSetSlot setSlot;
                setSlot.windowId = packet.windowId;
                setSlot.slot = s;
                setSlot.itemID = session.inventory.mainInventory[s].itemID;
                setSlot.count = session.inventory.mainInventory[s].count;
                setSlot.metadata = session.inventory.mainInventory[s].metadata;
                m_server->sendPacket(peer, setSlot, true);
            }
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
                    broadcastSound("damage.hit", target->posX, target->posY + target->height * 0.5, target->posZ, 1.0f, 1.0f, peer);
                }
            }
        }
    }
}
