#include "net/ServerPacketHandler.hpp"
#include "net/Server.hpp"
#include "net/Packets.hpp"
#include "net/Permissions.hpp"
#include "net/CommandHandler.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "items/ItemTool.hpp"
#include "entities/EntityPlayer.hpp"
#include "entities/EntityLiving.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdlib>

ServerPacketHandler::ServerPacketHandler(IntegratedServer& integratedServer, World& world, Server& server,
                                         std::map<ENetPeer*, PlayerSession>& players,
                                         Permissions& permissions, CommandHandler& commandHandler)
    : m_integratedServer(integratedServer), m_world(world), m_server(server), m_players(players), m_permissions(permissions), m_commandHandler(commandHandler) {}

EntityPlayer* ServerPacketHandler::findPlayer(int32_t entityID) {
    for (auto& entity : m_world.getEntities()) {
        if (entity->entityID == entityID) {
            return dynamic_cast<EntityPlayer*>(entity.get());
        }
    }
    return nullptr;
}

void ServerPacketHandler::handle(ENetPeer* peer, const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);

    switch (type) {
        case PacketType::Login:
            handleLogin(peer, ptr, size - 1);
            break;
        case PacketType::PlayerPosition:
        case PacketType::PlayerRotation:
        case PacketType::PlayerPosLook:
            handlePlayerPosition(peer, type, ptr, size - 1);
            break;
        case PacketType::PlayerDigging:
            handlePlayerDigging(peer, ptr, size - 1);
            break;
        case PacketType::BlockPlacement:
            handleBlockPlacement(peer, ptr, size - 1);
            break;
        case PacketType::ChunkRequest:
            handleChunkRequest(peer, ptr, size - 1);
            break;
        case PacketType::ClickWindow:
            handleClickWindow(peer, ptr, size - 1);
            break;
        case PacketType::UseEntity:
            handleUseEntity(peer, ptr, size - 1);
            break;
        case PacketType::ChatMessage:
            handleChatMessage(peer, ptr, size - 1);
            break;
        case PacketType::HeldItemChange:
            handleHeldItemChange(peer, ptr, size - 1);
            break;
        default:
            break;
    }
}

void ServerPacketHandler::handleLogin(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketLogin packet;
    packet.deserialize(data, size);
    std::cout << "Server: Player " << packet.username << " (" << packet.uuid << ") logged in." << std::endl;

    auto player = std::make_unique<EntityPlayer>(m_world);
    player->username = packet.username;
    player->uuid = packet.uuid;

    PlayerSaveData pData;
    auto saveHandler = m_world.getSaveHandler();
    if (saveHandler && saveHandler->loadPlayerData(packet.username, pData)) {
        std::cout << "Server: Loaded player data for " << packet.username << " at (" << pData.x << ", " << pData.y << ", " << pData.z << ")" << std::endl;
        player->setPosition(pData.x, pData.y, pData.z);
        player->rotationYaw = pData.yaw;
        player->rotationPitch = pData.pitch;
        player->health = pData.health;
        for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) player->inventory.mainInventory[i] = pData.inventory[i];
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
    pPtr->onPlaySound = [this, peer, pPtr](const std::string& name, float vol, float pitch) {
        PacketPlaySound ps;
        ps.name = name;
        ps.x = pPtr->posX; ps.y = pPtr->posY; ps.z = pPtr->posZ;
        ps.volume = vol; ps.pitch = pitch;
        m_server.broadcastPacket(ps, false, peer);
    };
    m_world.spawnEntity(std::move(player));

    int32_t eid = pPtr->entityID;
    m_players[peer] = {eid, packet.username, packet.uuid, GameMode::SURVIVAL, pPtr->posX, pPtr->posY, pPtr->posZ, 0.0f, false, {}, {}, 0, 0.0f, pPtr->posY};
    PlayerSession& session = m_players[peer];
    session.lastSentY = pPtr->posY;

    m_permissions.addOp(packet.username);

    PacketLoginResponse resp;
    resp.entityID = eid;
    m_server.sendPacket(peer, resp);

    PacketWindowItems invPacket;
    invPacket.windowId = 0;
    for (int i = 0; i < InventoryPlayer::TOTAL_SIZE; ++i) {
        invPacket.items.push_back({pPtr->inventory.mainInventory[i].itemID,
                                 pPtr->inventory.mainInventory[i].count,
                                 pPtr->inventory.mainInventory[i].metadata});
    }
    m_server.sendPacket(peer, invPacket);

    session.lastHealth = pPtr->health;

    PacketUpdateHealth hpPacket;
    hpPacket.health = pPtr->health;
    hpPacket.maxHealth = pPtr->maxHealth;
    m_server.sendPacket(peer, hpPacket, true);

    PacketPlayerPosLook posPacket;
    posPacket.x = pPtr->posX; posPacket.y = pPtr->posY; posPacket.z = pPtr->posZ;
    posPacket.yaw = pPtr->rotationYaw; posPacket.pitch = pPtr->rotationPitch;
    posPacket.onGround = pPtr->onGround;
    m_server.sendPacket(peer, posPacket);

    for (const auto& entity : m_world.getEntities()) {
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
        m_server.sendPacket(peer, spawn);
        m_players[peer].sentEntities.insert(entity->entityID);
    }
}

void ServerPacketHandler::handlePlayerPosition(ENetPeer* peer, PacketType type, const uint8_t* data, size_t size) {
    if (!m_players.count(peer)) return;
    PlayerSession& session = m_players[peer];
    int32_t eid = session.entityID;

    for (auto& entity : m_world.getEntities()) {
        if (entity->entityID != eid) continue;

        if (type == PacketType::PlayerPosition || type == PacketType::PlayerPosLook) {
            PacketPlayerPosition p;
            p.deserialize(data, size);

            double yDiff = p.y - session.lastSentY;

            int fx = (int)std::floor(p.x), fz = (int)std::floor(p.z);
            int waterCheckY = (int)std::floor(p.y + 0.5);
            uint8_t feetBlock = m_world.getBlockID(fx, waterCheckY, fz);
            if (feetBlock == 8 || feetBlock == 9) {
                session.accumulatedFall = 0.0f;
            }

            if (yDiff < -0.001) {
                session.accumulatedFall += (float)(-yDiff);
            } else {
                if (session.accumulatedFall > 3.0f && session.gameMode != GameMode::CREATIVE) {
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
            p.deserialize(data, size);
            entity->rotationYaw = p.yaw;
            entity->rotationPitch = p.pitch;
        }
        break;
    }
}

void ServerPacketHandler::handlePlayerDigging(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketPlayerDigging packet;
    packet.deserialize(data, size);

    if (packet.action == DiggingAction::DROP_ITEM) {
        if (!m_players.count(peer)) return;
        PlayerSession& session = m_players[peer];
        if (session.gameMode == GameMode::CREATIVE) return;

        EntityPlayer* player = findPlayer(session.entityID);
        if (!player) return;

        ItemStack& held = player->inventory.getCurrentStack();
        if (held.isEmpty()) return;

        auto item = std::make_unique<EntityItem>(m_world, held.itemID, 1, held.metadata);
        item->setPosition(player->posX, player->posY + 1.0, player->posZ);
        double yawRad   = (double)player->rotationYaw   * 3.14159265358979323846 / 180.0;
        double pitchRad = (double)player->rotationPitch * 3.14159265358979323846 / 180.0;
        item->motionX = -std::sin(yawRad) * std::cos(pitchRad) * 0.3;
        item->motionY = -std::sin(pitchRad) * 0.3 + 0.1;
        item->motionZ =  std::cos(yawRad) * std::cos(pitchRad) * 0.3;
        m_world.spawnEntity(std::move(item));

        held.count -= 1;
        if (held.count <= 0) held = {0, 0, 0, 0};

        PacketSetSlot setSlot;
        setSlot.windowId = 0;
        setSlot.slot = player->inventory.currentSlot;
        setSlot.itemID = held.itemID;
        setSlot.count = held.count;
        setSlot.metadata = held.metadata;
        m_server.sendPacket(peer, setSlot, true);
        return;
    }

    if (packet.action == DiggingAction::FINISH) {
        const uint8_t oldID = m_world.getBlockID(packet.x, packet.y, packet.z);
        const uint8_t oldMeta = m_world.getBlockMetadata(packet.x, packet.y, packet.z);
        m_world.setBlockWithNotify(packet.x, packet.y, packet.z, 0);

        if (const Block* b = Block::blocksList[oldID]) {
            PacketPlaySound ps;
            ps.name = b->stepSound->getBreakSound();
            ps.x = packet.x + 0.5; ps.y = packet.y + 0.5; ps.z = packet.z + 0.5;
            ps.volume = 1.0f; ps.pitch = 1.0f;
            m_server.broadcastPacket(ps, false, peer);
        }

        if (oldID > 0 && Block::getHardness(oldID) >= 0.0f) {
            int dropID = oldID;
            int dropCount = 1;
            if (const Block* b = Block::blocksList[oldID]) {
                dropID = b->idDropped(oldMeta);
                dropCount = b->quantityDropped();
            }
            if (dropID > 0 && dropCount > 0) {
                auto item = std::make_unique<EntityItem>(m_world, dropID, dropCount, 0);
                double spread = 0.7;
                item->setPosition(
                    packet.x + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5,
                    packet.y + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5,
                    packet.z + ((double)(std::rand() % 1000) / 1000.0) * spread + (1.0 - spread) * 0.5
                );
                item->delayBeforeCanPickup = 6;
                m_world.spawnEntity(std::move(item));
            }
        }

        if (m_players.count(peer)) {
            PlayerSession& session = m_players[peer];
            if (session.gameMode == GameMode::SURVIVAL) {
                EntityPlayer* player = findPlayer(session.entityID);
                if (player) {
                    ItemStack& held = player->inventory.getCurrentStack();
                    if (!held.isEmpty() && Item::itemsList[held.itemID]) {
                        if (auto* tool = dynamic_cast<ItemTool*>(Item::itemsList[held.itemID])) {
                            held.damage += 1;
                            if (held.damage >= tool->maxDamage) {
                                held = {0, 0, 0, 0};
                            }
                            PacketSetSlot setSlot;
                            setSlot.windowId = 0;
                            setSlot.slot = player->inventory.currentSlot;
                            setSlot.itemID = held.itemID;
                            setSlot.count = held.count;
                            setSlot.metadata = held.metadata;
                            m_server.sendPacket(peer, setSlot, true);
                        }
                    }
                }
            }
        }
    }
}

void ServerPacketHandler::handleBlockPlacement(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketBlockPlacement packet;
    packet.deserialize(data, size);
    int x = packet.x, y = packet.y, z = packet.z;
    if (packet.face == 0) y--; else if (packet.face == 1) y++;
    else if (packet.face == 2) z--; else if (packet.face == 3) z++;
    else if (packet.face == 4) x--; else if (packet.face == 5) x++;
    m_world.setBlockAndMetadataWithNotify(x, y, z, packet.blockID, packet.metadata);

    if (const Block* b = Block::blocksList[packet.blockID]) {
        PacketPlaySound ps;
        ps.name = b->stepSound->getBreakSound();
        ps.x = x + 0.5; ps.y = y + 0.5; ps.z = z + 0.5;
        ps.volume = 1.0f; ps.pitch = 0.8f;
        m_server.broadcastPacket(ps, false, peer);
    }

    if (m_players.count(peer)) {
        PlayerSession& session = m_players[peer];
        if (session.gameMode == GameMode::SURVIVAL && packet.blockID > 0) {
            EntityPlayer* player = findPlayer(session.entityID);
            if (player) {
                player->inventory.consumeCurrentItem(1);
                PacketSetSlot setSlot;
                setSlot.windowId = 0;
                setSlot.slot = player->inventory.currentSlot;
                setSlot.itemID = player->inventory.mainInventory[player->inventory.currentSlot].itemID;
                setSlot.count = player->inventory.mainInventory[player->inventory.currentSlot].count;
                setSlot.metadata = player->inventory.mainInventory[player->inventory.currentSlot].metadata;
                m_server.sendPacket(peer, setSlot, true);
            }
        }
    }
}

void ServerPacketHandler::handleChunkRequest(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketChunkRequest packet;
    packet.deserialize(data, size);
    m_world.requestChunk(packet.x, packet.z);
}

void ServerPacketHandler::handleClickWindow(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketClickWindow packet;
    packet.deserialize(data, size);
    if (!m_players.count(peer)) return;

    PlayerSession& session = m_players[peer];
    EntityPlayer* player = findPlayer(session.entityID);
    if (!player) return;

    InventoryPlayer& inv = player->inventory;

    if (packet.slot == -1 && !inv.cursorStack.isEmpty()) {
        if (session.gameMode == GameMode::SURVIVAL) {
            ItemStack dropped = inv.cursorStack;
            int dropCount = (packet.button != 0) ? 1 : dropped.count;
            auto item = std::make_unique<EntityItem>(m_world, dropped.itemID, dropCount, dropped.metadata);
            item->setPosition(player->posX, player->posY + 1.0, player->posZ);
            double yawRad = (double)player->rotationYaw * 3.14159265358979323846 / 180.0;
            item->motionX = -std::sin(yawRad) * 0.1;
            item->motionZ =  std::cos(yawRad) * 0.1;
            item->motionY = 0.2;
            m_world.spawnEntity(std::move(item));

            inv.cursorStack.count -= dropCount;
            if (inv.cursorStack.count <= 0) inv.cursorStack = {0, 0, 0};
        } else {
            inv.cursorStack = {0, 0, 0};
        }

        PacketConfirmTransaction resp;
        resp.windowId = packet.windowId;
        resp.actionId = packet.actionId;
        resp.accepted = true;
        m_server.sendPacket(peer, resp, true);

        PacketSetSlot cursorPacket;
        cursorPacket.windowId = 0;
        cursorPacket.slot = -1;
        cursorPacket.itemID = inv.cursorStack.itemID;
        cursorPacket.count = inv.cursorStack.count;
        cursorPacket.metadata = inv.cursorStack.metadata;
        m_server.sendPacket(peer, cursorPacket, true);
        return;
    }

    inv.handleClick(packet.slot, packet.button != 0);

    PacketConfirmTransaction resp;
    resp.windowId = packet.windowId;
    resp.actionId = packet.actionId;
    resp.accepted = true;
    m_server.sendPacket(peer, resp, true);

    std::vector<int> slotsToSync = { packet.slot, InventoryPlayer::RESULT_SLOT, InventoryPlayer::WORKBENCH_RESULT };
    for (int s : slotsToSync) {
        if (s < 0 || s >= InventoryPlayer::TOTAL_SIZE) continue;
        PacketSetSlot setSlot;
        setSlot.windowId = packet.windowId;
        setSlot.slot = s;
        setSlot.itemID = inv.mainInventory[s].itemID;
        setSlot.count = inv.mainInventory[s].count;
        setSlot.metadata = inv.mainInventory[s].metadata;
        m_server.sendPacket(peer, setSlot, true);
    }

    PacketSetSlot cursorPacket;
    cursorPacket.windowId = 0;
    cursorPacket.slot = -1;
    cursorPacket.itemID = inv.cursorStack.itemID;
    cursorPacket.count = inv.cursorStack.count;
    cursorPacket.metadata = inv.cursorStack.metadata;
    m_server.sendPacket(peer, cursorPacket, true);
}

void ServerPacketHandler::handleUseEntity(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketUseEntity packet;
    packet.deserialize(data, size);
    if (!packet.leftClick || !m_players.count(peer)) return;

    Entity* target = nullptr;
    for (auto& entity : m_world.getEntities()) {
        if (entity->entityID == packet.targetEntityID) {
            target = entity.get();
            break;
        }
    }

    if (!target) return;

    int damage = 1;
    auto& session = m_players[peer];
    EntityPlayer* player = findPlayer(session.entityID);
    int itemID = player ? player->inventory.getCurrentItemID() : 0;

    if (itemID == 268) damage = 4;
    else if (itemID == 272) damage = 5;
    else if (itemID == 267) damage = 6;
    else if (itemID == 283) damage = 5;
    else if (itemID == 276) damage = 7;
    else if (itemID >= 270 && itemID <= 279) damage = 2;

    if (auto* living = dynamic_cast<EntityLiving*>(target)) {
        living->attackEntityFrom(nullptr, damage);
        PacketPlaySound ps;
        ps.name = "damage.hit";
        ps.x = target->posX; ps.y = target->posY + target->height * 0.5; ps.z = target->posZ;
        ps.volume = 1.0f; ps.pitch = 1.0f;
        m_server.broadcastPacket(ps, false, peer);
    }
}

void ServerPacketHandler::handleChatMessage(ENetPeer* peer, const uint8_t* data, size_t size) {
    if (!m_players.count(peer)) return;

    PacketChatMessage packet;
    packet.deserialize(data, size);
    PlayerSession& session = m_players[peer];

    if (m_commandHandler.isCommand(packet.message)) {
        CommandHandler::CommandContext ctx;
        ctx.server = &m_integratedServer;
        ctx.senderPeer = peer;
        ctx.senderName = session.username;

        std::string result = m_commandHandler.execute(packet.message, ctx, m_permissions.isOp(session.username));
        if (!result.empty()) {
            PacketChatMessage resp;
            resp.sender = "";
            resp.message = result;
            resp.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            m_server.sendPacket(peer, resp, true);
        }

        std::string cmdName = packet.message;
        if (cmdName.size() > 1) {
            cmdName = cmdName.substr(1);
            auto space = cmdName.find(' ');
            if (space != std::string::npos) cmdName = cmdName.substr(0, space);
        }
        if (cmdName == "say") {
            std::string msg = packet.message;
            auto space = msg.find(' ');
            if (space != std::string::npos) {
                msg = msg.substr(space + 1);
            } else {
                msg = "";
            }
            if (!msg.empty()) {
                PacketChatMessage chat;
                chat.sender = session.username;
                chat.message = msg;
                chat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                m_server.broadcastPacket(chat, true);
            }
        }
    } else {
        PacketChatMessage chat;
        chat.sender = session.username;
        chat.message = packet.message;
        chat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        m_server.broadcastPacket(chat, true);
    }
}

void ServerPacketHandler::handleHeldItemChange(ENetPeer* peer, const uint8_t* data, size_t size) {
    PacketHeldItemChange packet;
    packet.deserialize(data, size);
    if (!m_players.count(peer)) return;

    PlayerSession& session = m_players[peer];
    EntityPlayer* player = findPlayer(session.entityID);
    if (player) {
        player->inventory.setSlot(packet.slot);
    }
}
