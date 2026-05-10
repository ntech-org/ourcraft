#pragma once

#include <memory>
#include <string>
#include <functional>
#include "net/Client.hpp"
#include "net/IntegratedServer.hpp"
#include "net/Packets.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

class NetworkHandler {
public:
    NetworkHandler(World& world, EntityPlayer& player, bool startServer = true);
    ~NetworkHandler();

    bool connect(const std::string& address, int port);
    void update();
    void stopServer();
    void setPaused(bool paused);
    bool isSingleplayer() const;
    int getPlayerCount() const;
    void sendPlayerPosition(const EntityPlayer& player);
    void sendDigging(DiggingAction action, int x, int y, int z, int face);
    void sendPlacement(int x, int y, int z, int face, int id, int meta);
    void sendPacket(const Packet& packet);

    int32_t getPlayerID() const { return m_playerID; }

    std::function<void(bool, const std::string&)> onDisconnected;

private:
    void onPacketReceived(const uint8_t* data, size_t size);

    World& m_world;
    EntityPlayer& m_player;
    
    std::unique_ptr<IntegratedServer> m_server;
    std::unique_ptr<Client> m_client;
    int32_t m_playerID = -1;

    double m_lastX = 0, m_lastY = 0, m_lastZ = 0;
    float m_lastYaw = 0, m_lastPitch = 0;
    int m_posUpdateTimer = 0;
};
