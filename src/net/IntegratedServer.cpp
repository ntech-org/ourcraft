#include "net/IntegratedServer.hpp"
#include "net/Packets.hpp"
#include "world/InfdevWorldGenerator.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/Block.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unordered_map>

namespace {
struct ChunkOffset {
    int dx;
    int dz;
    int distSq;
};

const std::vector<ChunkOffset>& getChunkOffsetsForRadius(int radius) {
    static std::unordered_map<int, std::vector<ChunkOffset>> cache;
    auto cached = cache.find(radius);
    if (cached != cache.end()) {
        return cached->second;
    }

    std::vector<ChunkOffset> offsets;
    offsets.reserve((radius * 2 + 1) * (radius * 2 + 1));
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            offsets.push_back({dx, dz, dx * dx + dz * dz});
        }
    }
    std::sort(offsets.begin(), offsets.end(), [](const ChunkOffset& a, const ChunkOffset& b) {
        return a.distSq < b.distSq;
    });
    return cache.emplace(radius, std::move(offsets)).first->second;
}
}

IntegratedServer::IntegratedServer() {
    m_world = std::make_unique<World>();
    m_world->setGenerator(std::make_unique<InfdevWorldGenerator>(1772835215));
    
    m_world->onBlockChanged = [this](int x, int y, int z, uint8_t id, uint8_t meta) {
        PacketBlockChange packet;
        packet.x = x; packet.y = y; packet.z = z;
        packet.blockID = id; packet.metadata = meta;
        if (m_server) m_server->broadcastPacket(packet, true);
    };

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
    if (m_isDedicated) {
        run();
    } else {
        m_thread = std::thread(&IntegratedServer::run, this);
    }
}

void IntegratedServer::stop() {
    if (!m_running) return;
    m_running = false;
    
    if (m_world) {
        m_world->saveAllChunks();
    }

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
    while (m_running) {
        m_server->poll();
        m_world->pollGeneratedChunks();

        auto now = std::chrono::steady_clock::now();
        if (m_paused) {
            lastTick = now; // Prevent catch-up burst
        } else if (now - lastTick >= std::chrono::milliseconds(50)) { // 20 TPS
            tickCounter++;
            if (tickCounter % 6000 == 0) { // Every 5 minutes
                m_world->saveAllChunks();
            }

            std::unordered_map<int32_t, Entity*> entitiesById;
            entitiesById.reserve(m_world->getEntities().size());
            for (const auto& entity : m_world->getEntities()) {
                entitiesById[entity->entityID] = entity.get();
            }

            // Server-Authoritative Automatic Chunk Pushing
            for (auto& [peer, session] : m_players) {
                auto playerIt = entitiesById.find(session.entityID);
                Entity* player = playerIt == entitiesById.end() ? nullptr : playerIt->second;
                if (!player) continue;

                int px = (int)std::floor(player->posX / 16.0);
                int pz = (int)std::floor(player->posZ / 16.0);
                int viewRadius = 8;
                int requestRadius = 10;
                const auto& offsets = getChunkOffsetsForRadius(requestRadius);

                int chunksSentThisTick = 0;
                constexpr int chunkLimitPerTick = 8;

                for (const ChunkOffset& off : offsets) {
                    const int cx = px + off.dx;
                    const int cz = pz + off.dz;
                    const bool inView = std::abs(off.dx) <= viewRadius && std::abs(off.dz) <= viewRadius;
                    
                    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) | static_cast<uint32_t>(cz);

                    auto chunk = m_world->getChunk(cx, cz);
                    if (!chunk) {
                        m_world->requestChunk(cx, cz);
                        continue;
                    }
                    
                    if (!inView) continue;
                    if (chunksSentThisTick >= chunkLimitPerTick) break;

                    ChunkState currentState = chunk->getState();
                    if (currentState < ChunkState::Lighted) continue;

                    auto it = session.sentChunks.find(key);
                    if (it == session.sentChunks.end()) {
                        PacketChunkData packet;
                        packet.x = cx; packet.z = cz;
                        packet.primaryBitmask = 0xFF;
                        packet.blockPtr = chunk->getBlocks();
                        packet.metaPtr = chunk->getMetadata();
                        packet.skyPtr = chunk->getSkylight();
                        packet.blockLightPtr = chunk->getBlocklight();
                        m_server->sendPacket(peer, packet, true);
                        session.sentChunks[key] = currentState;
                        chunksSentThisTick++;
                    }
                }
            }

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

            // Periodic server-side chunk unloading
            static int unloadTimer = 0;
            if (++unloadTimer >= 20 * 10) { // Every 10 seconds
                unloadTimer = 0;
                std::vector<std::pair<int, int>> toUnload;
                for (const auto& chunk : m_world->getAllChunks()) {
                    bool keep = false;
                    for (auto& [peer, session] : m_players) {
                        auto playerIt = entitiesById.find(session.entityID);
                        Entity* player = playerIt == entitiesById.end() ? nullptr : playerIt->second;
                        if (!player) continue;

                        int dx = std::abs(chunk->getX() - (int)std::floor(player->posX / 16.0));
                        int dz = std::abs(chunk->getZ() - (int)std::floor(player->posZ / 16.0));
                        if (dx <= 12 && dz <= 12) {
                            keep = true;
                            break;
                        }
                    }
                    if (!keep) {
                        toUnload.push_back({chunk->getX(), chunk->getZ()});
                    }
                }

                for (auto& pos : toUnload) {
                    m_world->removeChunk(pos.first, pos.second);
                    PacketChunkUnload packet;
                    packet.x = pos.first; packet.z = pos.second;
                    m_server->broadcastPacket(packet, true);
                    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(pos.first)) << 32) | static_cast<uint32_t>(pos.second);
                    for (auto& [peer, session] : m_players) session.sentChunks.erase(key);
                }
            }

            // 1. Broadcast entity positions BEFORE world update resets prevPos
            for (const auto& entity : m_world->getEntities()) {
                if (entity->posX != entity->prevPosX || entity->posY != entity->prevPosY || entity->posZ != entity->prevPosZ ||
                    entity->rotationYaw != entity->prevRotationYaw || entity->rotationPitch != entity->prevRotationPitch) {
                    
                    PacketMoveEntity move;
                    move.id = entity->entityID;
                    move.x = entity->posX;
                    move.y = entity->posY;
                    move.z = entity->posZ;
                    move.yaw = entity->rotationYaw;
                    move.pitch = entity->rotationPitch;
                    m_server->broadcastPacket(move, false);
                }
            }

            // 2. Update world (this resets prevPos to currentPos)
            m_world->update(0.05f);

            // 3. Server-authoritative item pickup.
            struct PendingPickup {
                ENetPeer* peer;
                int itemID;
                int count;
                uint8_t metadata;
            };
            std::vector<int32_t> removeItemEntityIDs;
            std::vector<PendingPickup> pickups;
            for (const auto& entity : m_world->getEntities()) {
                auto* item = dynamic_cast<EntityItem*>(entity.get());
                if (!item || item->pickupDelay > 0) continue;

                for (auto& [peer, session] : m_players) {
                    auto it = entitiesById.find(session.entityID);
                    if (it == entitiesById.end()) continue;
                    auto* player = dynamic_cast<EntityPlayer*>(it->second);
                    if (!player) continue;

                    AxisAlignedBB pickupBox = player->boundingBox.expand(0.35, 0.35, 0.35);
                    if (!pickupBox.intersectsWith(item->boundingBox)) continue;

                    pickups.push_back({peer, item->itemID, item->count, item->metadata});
                    removeItemEntityIDs.push_back(item->entityID);
                    break;
                }
            }
            for (const PendingPickup& pickup : pickups) {
                if (m_players.count(pickup.peer)) {
                    PlayerSession& session = m_players[pickup.peer];
                    session.inventory.addItem(pickup.itemID, pickup.count, pickup.metadata);
                    PacketWindowItems packet;
                    packet.windowId = 0;
                    for (int i = 0; i < InventoryPlayer::INVENTORY_SIZE; ++i) {
                        packet.items.push_back({session.inventory.mainInventory[i].itemID, 
                                              session.inventory.mainInventory[i].count, 
                                              session.inventory.mainInventory[i].metadata});
                    }
                    m_server->sendPacket(pickup.peer, packet, true);
                }
            }
            for (int32_t id : removeItemEntityIDs) {
                m_world->removeEntity(id);
            }

            // 4. Spawn any new entities for each player.
            for (auto& [peer, session] : m_players) {
                for (const auto& entity : m_world->getEntities()) {
                    if (!session.sentEntities.insert(entity->entityID).second) continue;

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
                    m_server->sendPacket(peer, spawn, true);
                }
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
        
        auto player = std::make_unique<EntityPlayer>(*m_world);
        player->setPosition(8.0, 100.0, 8.0);
        EntityPlayer* pPtr = player.get();
        m_world->spawnEntity(std::move(player));
        
        int32_t eid = pPtr->entityID; 
        m_players[peer] = {eid, packet.username, {}, {}};
        PlayerSession& session = m_players[peer];

        PacketLoginResponse resp;
        resp.entityID = eid;
        m_server->sendPacket(peer, resp);

        PacketWindowItems invPacket;
        invPacket.windowId = 0;
        for (int i = 0; i < InventoryPlayer::INVENTORY_SIZE; ++i) {
            invPacket.items.push_back({session.inventory.mainInventory[i].itemID, 
                                     session.inventory.mainInventory[i].count, 
                                     session.inventory.mainInventory[i].metadata});
        }
        m_server->sendPacket(peer, invPacket, true);

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
                item->setPosition(packet.x + 0.5, packet.y + 0.35, packet.z + 0.5);
                item->motionX = ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.1;
                item->motionY = 0.18 + ((double)(std::rand() % 1000) / 1000.0) * 0.05;
                item->motionZ = ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.1;
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
    }
}
