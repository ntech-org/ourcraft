#pragma once

#include <memory>
#include <string>
#include <functional>
#include "net/Client.hpp"
#include "net/IntegratedServer.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

class NetworkHandler {
public:
    NetworkHandler(World& world, EntityPlayer& player);
    ~NetworkHandler();

    bool connect(const std::string& address, int port);
    void update();
    void sendPlayerPosition(const EntityPlayer& player);

    int32_t getPlayerID() const { return m_playerID; }

private:
    void onPacketReceived(const uint8_t* data, size_t size);

    World& m_world;
    EntityPlayer& m_player;
    
    std::unique_ptr<IntegratedServer> m_server;
    std::unique_ptr<Client> m_client;
    int32_t m_playerID = -1;
};
