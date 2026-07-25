#pragma once

#include "net/Server.hpp"
#include "net/PlayerSession.hpp"
#include "net/Packet.hpp"
#include <map>

struct _ENetPeer;
typedef struct _ENetPeer ENetPeer;

class World;
class Permissions;
class CommandHandler;
class IntegratedServer;
class EntityPlayer;
class RegistrationManager;

class ServerPacketHandler {
public:
    ServerPacketHandler(IntegratedServer& integratedServer, World& world, Server& server,
                        std::map<ENetPeer*, PlayerSession>& players,
                        Permissions& permissions, CommandHandler& commandHandler,
                        RegistrationManager& registrationManager);

    void handle(ENetPeer* peer, const uint8_t* data, size_t size);

private:
    EntityPlayer* findPlayer(int32_t entityID);

    void handleLogin(ENetPeer* peer, const uint8_t* data, size_t size);
    void handlePlayerPosition(ENetPeer* peer, PacketType type, const uint8_t* data, size_t size);
    void handlePlayerDigging(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleBlockPlacement(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleChunkRequest(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleClickWindow(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleUseEntity(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleChatMessage(ENetPeer* peer, const uint8_t* data, size_t size);
    void handleHeldItemChange(ENetPeer* peer, const uint8_t* data, size_t size);

    IntegratedServer& m_integratedServer;
    World& m_world;
    Server& m_server;
    std::map<ENetPeer*, PlayerSession>& m_players;
    Permissions& m_permissions;
    CommandHandler& m_commandHandler;
    RegistrationManager& m_registrationManager;
};
