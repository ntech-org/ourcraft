#pragma once

#include <enet/enet.h>
#include <string>
#include <functional>
#include "net/Packet.hpp"

class Client {
public:
    Client();
    ~Client();

    bool connect(const std::string& address, uint16_t port);
    void disconnect();
    void sendPacket(const Packet& packet, bool reliable = true);
    void poll();

    std::function<void(const uint8_t*, size_t)> onPacketReceived;

private:
    ENetHost* m_client = nullptr;
    ENetPeer* m_peer = nullptr;
};
