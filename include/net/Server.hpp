#pragma once

#include <enet/enet.h>
#include <vector>
#include <functional>
#include "net/Packet.hpp"

class Server {
public:
    Server(uint16_t port);
    ~Server();

    void poll();
    void sendPacket(ENetPeer* peer, const Packet& packet, bool reliable = true);
    void broadcastPacket(const Packet& packet, bool reliable = true);
    void kick(ENetPeer* peer, const std::string& reason);

    std::function<void(ENetPeer*)> onClientConnected;
    std::function<void(ENetPeer*)> onClientDisconnected;
    std::function<void(ENetPeer*, const uint8_t*, size_t)> onPacketReceived;

private:
    ENetHost* m_server = nullptr;
};
