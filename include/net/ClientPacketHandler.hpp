#pragma once

#include "net/NetworkHandler.hpp"
#include "net/Packets.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include "entities/EntityPlayer.hpp"
#include "Minecraft.hpp"
#include <cstring>

inline void handleClientPacket(NetworkHandler& handler, World& world, EntityPlayer& player, int32_t& playerID,
                                const uint8_t* ptr, size_t size, PacketType type) {
    if (type == PacketType::LoginResponse) {
        PacketLoginResponse packet;
        packet.deserialize(ptr, size - 1);
        playerID = packet.entityID;
        player.entityID = playerID;
    } else if (type == PacketType::SpawnEntity) {
        PacketSpawnEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.id == playerID) return;

        std::unique_ptr<Entity> entity;
        if (packet.type == 1) {
            entity = std::make_unique<EntityZombie>(world);
        } else if (packet.type == 2) {
            auto item = std::make_unique<EntityItem>(world, packet.dataA, packet.dataB, packet.dataC);
            item->pickupDelay = 0;
            item->handlePhysics = false;
            entity = std::move(item);
        } else {
            entity = std::make_unique<EntityPlayer>(world);
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

        world.spawnEntity(std::move(entity));
    } else if (type == PacketType::MoveEntity) {
        PacketMoveEntity packet;
        packet.deserialize(ptr, size - 1);
        if (packet.id == playerID) return;

        for (auto& entity : world.getEntities()) {
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
        world.m_pendingRequests.erase(key);
        world.m_pendingChunks.erase(key);

        std::shared_ptr<Chunk> chunk = world.getChunk(packet.x, packet.z);
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
            world.predictLighting(*chunk);
        } else {
            chunk->setLightWipeComplete(true);
        }

        chunk->setState(ChunkState::Complete);
        chunk->generateHeightMap();
        chunk->generateBitmask();

        if (isNew) {
            world.addChunk(chunk);
        }

        auto nW = world.getChunk(packet.x - 1, packet.z);
        auto nE = world.getChunk(packet.x + 1, packet.z);
        auto nN = world.getChunk(packet.x, packet.z - 1);
        auto nS = world.getChunk(packet.x, packet.z + 1);
        auto nNW = world.getChunk(packet.x - 1, packet.z - 1);
        auto nNE = world.getChunk(packet.x + 1, packet.z - 1);
        auto nSW = world.getChunk(packet.x - 1, packet.z + 1);
        auto nSE = world.getChunk(packet.x + 1, packet.z + 1);

        for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
            chunk->markSectionDirtyInternal(i);
            if (nW) nW->touchSection(i);
            if (nE) nE->touchSection(i);
            if (nN) nN->touchSection(i);
            if (nS) nS->touchSection(i);
            if (nNW) nNW->touchSection(i);
            if (nNE) nNE->touchSection(i);
            if (nSW) nSW->touchSection(i);
            if (nSE) nSE->touchSection(i);
        }
    } else if (type == PacketType::BlockChange) {
        PacketBlockChange packet;
        packet.deserialize(ptr, size - 1);
        world.setBlockAndMetadataWithNotify(packet.x, packet.y, packet.z, packet.blockID, packet.metadata);
    } else if (type == PacketType::DestroyEntity) {
        PacketDestroyEntity packet;
        packet.deserialize(ptr, size - 1);
        world.removeEntity(packet.id);
    } else if (type == PacketType::CollectItem) {
        PacketCollectItem packet;
        packet.deserialize(ptr, size - 1);
        double targetX = player.posX;
        double targetY = player.posY + player.height * 0.3;
        double targetZ = player.posZ;
        for (auto& entity : world.getEntities()) {
            if (entity->entityID != packet.collectorEntityID) continue;
            targetX = entity->posX;
            targetY = entity->posY + entity->height * 0.3;
            targetZ = entity->posZ;
            break;
        }
        targetX *= 0.995;
        targetZ *= 0.995;
        for (auto& entity : world.getEntities()) {
            if (entity->entityID == packet.itemEntityID) {
                if (auto* item = dynamic_cast<EntityItem*>(entity.get())) {
                    item->startPickupAnimation(targetX, targetY, targetZ);
                    // Play pickup sound via the player's minecraft instance
                    try {
                        auto& mc = player.getMinecraft();
                        if (auto* snd = mc.getSoundPool().getRandom("random.pop", mc.getSoundSystem()))
                            mc.getSoundSystem().play(snd, mc.getSettings().soundVolume, 1.0f);
                    } catch (...) {}
                }
                break;
            }
        }
    } else if (type == PacketType::ChunkUnload) {
        PacketChunkUnload packet;
        packet.deserialize(ptr, size - 1);
        world.removeChunk(packet.x, packet.z);
    } else if (type == PacketType::PlayerPosLook) {
        PacketPlayerPosLook packet;
        packet.deserialize(ptr, size - 1);
        player.setPosition(packet.x, packet.y, packet.z);
        player.rotationYaw = packet.yaw;
        player.rotationPitch = packet.pitch;
        player.onGround = packet.onGround;
    } else if (type == PacketType::InventoryAdd) {
        PacketInventoryAdd packet;
        packet.deserialize(ptr, size - 1);
        player.inventory.addItem(packet.itemID, packet.count, packet.metadata);
    } else if (type == PacketType::WindowItems) {
        PacketWindowItems packet;
        packet.deserialize(ptr, size - 1);
        if (packet.windowId == 0) {
            for (size_t i = 0; i < packet.items.size() && i < InventoryPlayer::TOTAL_SIZE; ++i) {
                player.inventory.mainInventory[i] = {packet.items[i].id, (int)packet.items[i].count, packet.items[i].metadata};
            }
        }
    } else if (type == PacketType::SetSlot) {
        PacketSetSlot packet;
        packet.deserialize(ptr, size - 1);
        if (packet.windowId == 0) {
            if (packet.slot == -1) {
                player.inventory.cursorStack = {packet.itemID, packet.count, packet.metadata};
            } else if (packet.slot >= 0 && packet.slot < InventoryPlayer::TOTAL_SIZE) {
                player.inventory.mainInventory[packet.slot] = {packet.itemID, packet.count, packet.metadata};
            }
        }
    } else if (type == PacketType::ConfirmTransaction) {
        PacketConfirmTransaction packet;
        packet.deserialize(ptr, size - 1);
    }
}
