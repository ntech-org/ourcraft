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
            // Server-Authoritative Automatic Chunk Pushing (Rate-Limited & Distance-Sorted)
            for (auto& [peer, session] : m_players) {
                Entity* player = nullptr;
                for (auto& e : m_world->getEntities()) {
                    if (e->entityID == session.entityID) {
                        player = e.get();
                        break;
                    }
                }
                if (!player) continue;

                int px = (int)std::floor(player->posX / 16.0);
                int pz = (int)std::floor(player->posZ / 16.0);
                int viewRadius = 8;

                // Collect chunks to potentially send (static to reuse memory)
                struct SortablePos { int x, z; int distSq; };
                static std::vector<SortablePos> candidates;
                candidates.clear();

                for (int dx = -viewRadius; dx <= viewRadius; ++dx) {
                    for (int dz = -viewRadius; dz <= viewRadius; ++dz) {
                        candidates.push_back({px + dx, pz + dz, dx*dx + dz*dz});
                    }
                }
                std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
                    return a.distSq < b.distSq;
                });

                int chunksSentThisTick = 0;
                const int chunkLimitPerTick = 8; 

                for (const auto& pos : candidates) {
                    if (chunksSentThisTick >= chunkLimitPerTick) break;

                    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(pos.x)) << 32) | static_cast<uint32_t>(pos.z);

                    auto chunk = m_world->getChunk(pos.x, pos.z);
                    if (!chunk) {
                        m_world->requestChunk(pos.x, pos.z);
                        continue;
                    }

                    ChunkState currentState = chunk->getState();
                    if (currentState < ChunkState::Generated) continue;

                    auto it = session.sentChunks.find(key);
                    bool shouldSend = false;
                    if (it == session.sentChunks.end()) {
                        shouldSend = true;
                    } else if (it->second < ChunkState::Complete && currentState == ChunkState::Complete) {
                        shouldSend = true;
                    }

                    if (shouldSend) {
                        PacketChunkData packet;
                        packet.x = pos.x; packet.z = pos.z;
                        packet.blockPtr = chunk->getBlocks();
                        packet.metaPtr = chunk->getMetadata();

                        static const std::vector<uint8_t> zeroLight(Chunk::SIZE / 2, 0);

                        if (currentState == ChunkState::Complete) {
                            packet.skyPtr = chunk->getSkylight();
                            packet.blockLightPtr = chunk->getBlocklight();
                        } else {
                            packet.skyPtr = zeroLight.data();
                            packet.blockLightPtr = zeroLight.data();
                        }

                        packet.primaryBitmask = chunk->getPrimaryBitmask();

                        m_server->sendPacket(peer, packet, true);
                        session.sentChunks[key] = currentState;
                        chunksSentThisTick++;
                    }
                }
                }

            // Broadcast updates for chunks that just became Complete
        auto completeChunks = m_world->popCompleteChunks();
        for (auto& chunk : completeChunks) {
            PacketChunkData packet;
            packet.x = chunk->getX();
            packet.z = chunk->getZ();
            packet.blockPtr = chunk->getBlocks();
            packet.metaPtr = chunk->getMetadata();
            packet.skyPtr = chunk->getSkylight();
            packet.blockLightPtr = chunk->getBlocklight();
            packet.primaryBitmask = chunk->getPrimaryBitmask();

            m_server->broadcastPacket(packet, true);
                
                uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(packet.x)) << 32) | static_cast<uint32_t>(packet.z);
                for (auto& [p, s] : m_players) s.sentChunks[key] = ChunkState::Complete;
            }

            m_world->popNewChunks(); 

            auto removedEntities = m_world->popRemovedEntities();
            for (int32_t id : removedEntities) {
                PacketDestroyEntity packet;
                packet.id = id;
                m_server->broadcastPacket(packet, true);
            }

            // Periodic server-side chunk unloading
            static int unloadTimer = 0;
            if (++unloadTimer >= 20 * 10) { // Every 10 seconds
                unloadTimer = 0;
                
                std::vector<std::pair<int, int>> toUnload;
                for (const auto& chunk : m_world->getChunks()) {
                    bool keep = false;
                    for (auto& [peer, session] : m_players) {
                        Entity* player = nullptr;
                        for (auto& e : m_world->getEntities()) {
                            if (e->entityID == session.entityID) {
                                player = e.get();
                                break;
                            }
                        }
                        if (!player) continue;

                        int dx = std::abs(chunk->getX() - (int)std::floor(player->posX / 16.0));
                        int dz = std::abs(chunk->getZ() - (int)std::floor(player->posZ / 16.0));
                        if (dx <= 12 && dz <= 12) { // Keep slightly more than view radius
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
        m_players[peer] = {eid, packet.username, {}};

        PacketLoginResponse resp;
        resp.entityID = eid;
        m_server->sendPacket(peer, resp);

        // Chunks and entities will be handled by the main loop automatically

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
    } else if (type == PacketType::PlayerDigging) {
        PacketPlayerDigging packet;
        packet.deserialize(ptr, size - 1);
        if (packet.action == DiggingAction::FINISH) {
            m_world->setBlockWithNotify(packet.x, packet.y, packet.z, 0);
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
        // Handle unloading if necessary, or let world cleanup handle it
    }
}
