#include "net/ServerTick.hpp"
#include "net/IntegratedServer.hpp"
#include "net/Server.hpp"
#include "net/Packets.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include "entities/EntityLiving.hpp"
#include "items/Item.hpp"
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

int getEntitySpawnType(EntityType type) {
    switch (type) {
        case EntityType::Zombie: return 1;
        case EntityType::Item: return 2;
        default: return 0;
    }
}

} // namespace

void saveAllPlayers(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players) {
    auto saveHandler = world.getSaveHandler();
    if (!saveHandler) return;

    LevelData data;
    if (!saveHandler->loadLevelData(data)) {
        data.seed = 1772835215;
        data.spawnX = 0; data.spawnY = 66; data.spawnZ = 0;
    }
    data.time = world.getWorldTime();
    saveHandler->saveLevelData(data);

    for (auto& [peer, session] : players) {
        for (const auto& entity : world.getEntities()) {
            if (entity->entityID == session.entityID) {
                auto* player = dynamic_cast<EntityPlayer*>(entity.get());
                if (!player) break;
                PlayerSaveData pData;
                pData.name = session.username;
                pData.x = entity->posX; pData.y = entity->posY; pData.z = entity->posZ;
                pData.yaw = entity->rotationYaw; pData.pitch = entity->rotationPitch;
                if (auto* living = dynamic_cast<EntityLiving*>(entity.get())) pData.health = living->health;
                if (auto* p = dynamic_cast<EntityPlayer*>(entity.get())) pData.gameMode = (p->gameMode == GameMode::CREATIVE) ? 1 : 0;
                for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) pData.inventory[i] = player->inventory.mainInventory[i];
                saveHandler->savePlayerData(pData);
                break;
            }
        }
    }
    saveHandler->flush();
}

void broadcastEntityPositions(World& world, Server& server) {
    for (const auto& entity : world.getEntities()) {
        if (entity->posX != entity->prevPosX || entity->posY != entity->prevPosY || entity->posZ != entity->prevPosZ ||
            entity->rotationYaw != entity->prevRotationYaw || entity->rotationPitch != entity->prevRotationPitch) {

            PacketMoveEntity move;
            move.id = entity->entityID;
            move.x = entity->posX;
            move.y = entity->posY;
            move.z = entity->posZ;
            move.yaw = entity->rotationYaw;
            move.pitch = entity->rotationPitch;
            server.broadcastPacket(move, false);
        }
    }
}

void handleRespawns(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players) {
    struct PendingSpawn {
        int itemID, count;
        uint8_t metadata;
        double posX, posY, posZ;
    };
    std::vector<PendingSpawn> pendingSpawns;

    for (auto& [peer, session] : players) {
        for (const auto& entity : world.getEntities()) {
            if (entity->entityID == session.entityID) {
                auto* living = dynamic_cast<EntityLiving*>(entity.get());
                auto* player = dynamic_cast<EntityPlayer*>(entity.get());
                if (living && living->health <= 0) {
                    double dropX = entity->posX;
                    double dropY = entity->posY;
                    double dropZ = entity->posZ;

                    if (session.gameMode == GameMode::SURVIVAL && player) {
                        for (int i = 0; i < InventoryPlayer::INVENTORY_SIZE; ++i) {
                            ItemStack& slot = player->inventory.mainInventory[i];
                            if (!slot.isEmpty()) {
                                pendingSpawns.push_back({slot.itemID, slot.count, slot.metadata,
                                    dropX + ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.8,
                                    dropY + ((double)(std::rand() % 1000) / 1000.0) * 0.5 + 0.5,
                                    dropZ + ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.8
                                });
                            }
                            slot = {0, 0, 0};
                        }
                        player->inventory.cursorStack = {0, 0, 0};

                        PacketWindowItems invPacket;
                        invPacket.windowId = 0;
                        for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) {
                            invPacket.items.push_back({player->inventory.mainInventory[i].itemID,
                                                      player->inventory.mainInventory[i].count,
                                                      player->inventory.mainInventory[i].metadata});
                        }
                        server.sendPacket(peer, invPacket, true);

                        PacketSetSlot cursorPacket;
                        cursorPacket.windowId = 0;
                        cursorPacket.slot = -1;
                        cursorPacket.itemID = 0;
                        cursorPacket.count = 0;
                        cursorPacket.metadata = 0;
                        server.sendPacket(peer, cursorPacket, true);
                    }

                    LevelData levelData;
                    auto saveHandler = world.getSaveHandler();
                    if (saveHandler && saveHandler->loadLevelData(levelData)) {
                        if (levelData.spawnY > 100 || levelData.spawnY < 5) levelData.spawnY = 66;
                        entity->setPosition(levelData.spawnX, levelData.spawnY, levelData.spawnZ);
                    } else {
                        entity->setPosition(0.0, 66.0, 0.0);
                    }
                    living->health = living->maxHealth;
                    living->deathTime = 0;
                    entity->motionX = entity->motionY = entity->motionZ = 0;

                    PacketPlayerPosLook respawnPos;
                    respawnPos.x = entity->posX; respawnPos.y = entity->posY; respawnPos.z = entity->posZ;
                    respawnPos.yaw = entity->rotationYaw; respawnPos.pitch = entity->rotationPitch;
                    server.sendPacket(peer, respawnPos, true);
                    PacketUpdateHealth hpPkt;
                    hpPkt.health = living->health;
                    hpPkt.maxHealth = living->maxHealth;
                    server.sendPacket(peer, hpPkt, true);
                    session.lastHealth = living->health;
                }
                break;
            }
        }
    }

    for (const auto& spawn : pendingSpawns) {
        auto item = std::make_unique<EntityItem>(world, spawn.itemID, spawn.count, spawn.metadata);
        item->setPosition(spawn.posX, spawn.posY, spawn.posZ);
        item->motionX = ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.2;
        item->motionY = 0.2 + (double)(std::rand() % 1000) / 1000.0 * 0.4;
        item->motionZ = ((double)(std::rand() % 1000) / 1000.0 - 0.5) * 0.2;
        world.spawnEntity(std::move(item));
    }
}

void handleItemPickups(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                       const std::unordered_map<int32_t, Entity*>& entitiesById) {
    struct PendingPickup {
        ENetPeer* peer;
        int collectorEntityID;
        int itemEntityID;
        int itemID;
        int count;
        uint8_t metadata;
    };
    std::vector<PendingPickup> pickups;
    for (const auto& entity : world.getEntities()) {
        if (entity->getType() != EntityType::Item) continue;
        auto* item = static_cast<EntityItem*>(entity.get());
        if (item->delayBeforeCanPickup > 0) continue;

        for (auto& [peer, session] : players) {
            auto it = entitiesById.find(session.entityID);
            if (it == entitiesById.end()) continue;
            auto* player = dynamic_cast<EntityPlayer*>(it->second);
            if (!player) continue;

            AxisAlignedBB pickupBox = player->boundingBox.expand(0.6, 0.6, 0.6);
            if (!pickupBox.intersectsWith(item->boundingBox)) continue;

            pickups.push_back({peer, player->entityID, item->entityID, item->itemID, item->count, item->metadata});
            break;
        }
    }
    for (const PendingPickup& pickup : pickups) {
        if (players.count(pickup.peer)) {
            auto& session = players[pickup.peer];
            auto it = entitiesById.find(session.entityID);
            EntityPlayer* player = (it != entitiesById.end()) ? dynamic_cast<EntityPlayer*>(it->second) : nullptr;
            if (player) {
                player->inventory.addItem(pickup.itemID, pickup.count, pickup.metadata);
                PacketWindowItems packet;
                packet.windowId = 0;
                for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) {
                    packet.items.push_back({player->inventory.mainInventory[i].itemID,
                                          player->inventory.mainInventory[i].count,
                                          player->inventory.mainInventory[i].metadata});
                }
                server.sendPacket(pickup.peer, packet, true);

                PacketSetSlot cursorPacket;
                cursorPacket.windowId = 0;
                cursorPacket.slot = -1;
                cursorPacket.itemID = player->inventory.cursorStack.itemID;
                cursorPacket.count = player->inventory.cursorStack.count;
                cursorPacket.metadata = player->inventory.cursorStack.metadata;
                server.sendPacket(pickup.peer, cursorPacket, true);
            }
        }
        PacketCollectItem collectPacket;
        collectPacket.itemEntityID = pickup.itemEntityID;
        collectPacket.collectorEntityID = pickup.collectorEntityID;
        server.broadcastPacket(collectPacket, true);
        world.removeEntity(pickup.itemEntityID, false);
    }
}

void spawnNewEntities(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players) {
    for (auto& [peer, session] : players) {
        for (const auto& entity : world.getEntities()) {
            if (!session.sentEntities.insert(entity->entityID).second) continue;

            PacketSpawnEntity spawn;
            spawn.id = entity->entityID;
            spawn.type = getEntitySpawnType(entity->getType());
            spawn.x = entity->posX;
            spawn.y = entity->posY;
            spawn.z = entity->posZ;
            spawn.yaw = entity->rotationYaw;
            spawn.pitch = entity->rotationPitch;
            if (entity->getType() == EntityType::Item) {
                auto* item = static_cast<EntityItem*>(entity.get());
                spawn.dataA = item->itemID;
                spawn.dataB = item->count;
                spawn.dataC = item->metadata;
            } else if (entity->getType() == EntityType::Player) {
                auto* p = static_cast<EntityPlayer*>(entity.get());
                spawn.username = p->username;
                spawn.uuid = p->uuid;
            }
            server.sendPacket(peer, spawn, true);
        }
    }
}

void pushChunksToPlayers(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                         const std::unordered_map<int32_t, Entity*>& entitiesById, int chunkKeepDistance) {
    for (auto& [peer, session] : players) {
        auto playerIt = entitiesById.find(session.entityID);
        Entity* player = playerIt == entitiesById.end() ? nullptr : playerIt->second;
        if (!player) continue;

        int px = (int)std::floor(player->posX / 16.0);
        int pz = (int)std::floor(player->posZ / 16.0);
        int viewRadius = chunkKeepDistance;
        int requestRadius = chunkKeepDistance + 2;
        const auto& offsets = getChunkOffsetsForRadius(requestRadius);

        int chunksSentThisTick = 0;
        int chunkLimitPerTick = 8 + (chunkKeepDistance / 4);

        for (const ChunkOffset& off : offsets) {
            const int cx = px + off.dx;
            const int cz = pz + off.dz;
            const bool inView = std::abs(off.dx) <= viewRadius && std::abs(off.dz) <= viewRadius;

            uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) | static_cast<uint32_t>(cz);

            auto chunk = world.getChunk(cx, cz);
            if (!chunk) {
                world.requestChunk(cx, cz);
                continue;
            }

            if (!inView) continue;
            if (chunksSentThisTick >= chunkLimitPerTick) break;

            ChunkState currentState = chunk->getState();
            if (currentState != ChunkState::Complete) continue;

            auto it = session.sentChunks.find(key);
            if (it == session.sentChunks.end()) {
                PacketChunkData packet;
                packet.x = cx; packet.z = cz;
                packet.primaryBitmask = 0xFF;
                packet.blockPtr = chunk->getBlocks();
                packet.metaPtr = chunk->getMetadata();
                packet.skyPtr = chunk->getSkylight();
                packet.blockLightPtr = chunk->getBlocklight();
                server.sendPacket(peer, packet, true);
                session.sentChunks[key] = currentState;
                chunksSentThisTick++;
            }
        }
    }
}

void unloadFarChunks(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                     const std::unordered_map<int32_t, Entity*>& entitiesById, int keepDistance) {
    std::vector<std::pair<int, int>> toUnload;
    for (const auto& chunk : world.getAllChunks()) {
        bool keep = false;
        for (auto& [peer, session] : players) {
            auto playerIt = entitiesById.find(session.entityID);
            Entity* player = playerIt == entitiesById.end() ? nullptr : playerIt->second;
            if (!player) continue;

            int dx = std::abs(chunk->getX() - (int)std::floor(player->posX / 16.0));
            int dz = std::abs(chunk->getZ() - (int)std::floor(player->posZ / 16.0));
            if (dx <= keepDistance && dz <= keepDistance) {
                keep = true;
                break;
            }
        }
        if (!keep) {
            toUnload.push_back({chunk->getX(), chunk->getZ()});
        }
    }

    for (auto& pos : toUnload) {
        world.removeChunk(pos.first, pos.second);
        PacketChunkUnload packet;
        packet.x = pos.first; packet.z = pos.second;
        server.broadcastPacket(packet, true);
        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(pos.first)) << 32) | static_cast<uint32_t>(pos.second);
        for (auto& [peer, session] : players) session.sentChunks.erase(key);
    }
}

void spawnMobs(World& world, std::map<ENetPeer*, PlayerSession>& players,
               const std::unordered_map<int32_t, Entity*>& entitiesById) {
    for (auto& [peer, session] : players) {
        auto it = entitiesById.find(session.entityID);
        if (it == entitiesById.end()) continue;
        Entity* player = it->second;

        for (int i = 0; i < 3; ++i) {
            int rx = (std::rand() % 64) - 32;
            int rz = (std::rand() % 64) - 32;
            int x = (int)std::floor(player->posX) + rx;
            int z = (int)std::floor(player->posZ) + rz;

            int y = 0;
            for (y = 127; y > 0; --y) {
                if (world.getBlockID(x, y, z) != 0) break;
            }
            y++;

            if (y > 0 && y < 128) {
                int light = world.getSavedLightValue(LightType::Block, x, y, z);
                int skyLight = world.getSavedLightValue(LightType::Sky, x, y, z);

                if (light < 7 && skyLight < 7) {
                    auto zombie = std::make_unique<EntityZombie>(world);
                    zombie->setPosition(x + 0.5, y, z + 0.5);
                    if (world.getCollidingBoundingBoxes(zombie->boundingBox).empty()) {
                        world.spawnEntity(std::move(zombie));
                    }
                }
            }
        }
    }
}
