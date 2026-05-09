#include "net/Server.hpp"
#include <stdexcept>
#include <iostream>

Server::Server(uint16_t port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    std::cout << "[Server] Starting server on port " << port << "..." << std::endl;

    // Set reasonable bandwidth limits (e.g. 20MB/s)
    m_server = enet_host_create(&address, 32, 2, 20 * 1024 * 1024, 20 * 1024 * 1024);
    if (m_server == NULL) {
        std::cerr << "[Server] Failed to create ENet host on port " << port << std::endl;
        throw std::runtime_error("An error occurred while trying to create an ENet server host.");
    }
    std::cout << "[Server] Server started successfully." << std::endl;
}

Server::~Server() {
    std::cout << "[Server] Stopping server..." << std::endl;
    enet_host_destroy(m_server);
}

void Server::poll() {
    ENetEvent event;
    while (enet_host_service(m_server, &event, 0) > 0) {
        char host[128];
        enet_address_get_host_ip(&event.peer->address, host, 128);

        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "[Server] Client connected from " << host << ":" << event.peer->address.port << std::endl;
                if (onClientConnected) onClientConnected(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (onPacketReceived) onPacketReceived(event.peer, event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.data != 0) {
                    std::cout << "[Server] Client from " << host << ":" << event.peer->address.port << " timed out or lost connection." << std::endl;
                } else {
                    std::cout << "[Server] Client disconnected from " << host << ":" << event.peer->address.port << std::endl;
                }
                if (onClientDisconnected) onClientDisconnected(event.peer);
                break;
            default:
                break;
        }
    }
}

void Server::sendPacket(ENetPeer* peer, const Packet& packet, bool reliable) {
    std::vector<uint8_t> buffer;
    packet.serialize(buffer);
    
    ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(peer, 0, enetPacket);
}

void Server::broadcastPacket(const Packet& packet, bool reliable) {
    std::vector<uint8_t> buffer;
    packet.serialize(buffer);
    
    ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_host_broadcast(m_server, 0, enetPacket);
}
