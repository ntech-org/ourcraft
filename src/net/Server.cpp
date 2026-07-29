#include "net/Server.hpp"
#include "net/Packets.hpp"
#include <iostream>

Server::Server(uint16_t port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    std::cout << "[Server] Starting server on port " << port << "..." << std::endl;

    m_server = enet_host_create(&address, 32, 2, 20 * 1024 * 1024, 20 * 1024 * 1024);
    if (m_server == NULL) {
        std::cerr << "[Server] Failed to create ENet host on port " << port << std::endl;
    } else {
        std::cout << "[Server] Server started successfully." << std::endl;
    }
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
            case ENET_EVENT_TYPE_RECEIVE: {
                // Check if it's a disconnect packet
                const uint8_t* ptr = event.packet->data;
                PacketType type = (PacketType)Packet::readByte(ptr);
                if (type == PacketType::Disconnect) {
                    PacketDisconnect p;
                    p.deserialize(ptr, event.packet->dataLength - 1);
                    std::cout << "[Server] Client " << host << ":" << event.peer->address.port << " disconnected: " << p.reason << std::endl;
                } else if (onPacketReceived) {
                    onPacketReceived(event.peer, event.packet->data, event.packet->dataLength);
                }
                enet_packet_destroy(event.packet);
                break;
            }
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

void Server::broadcastPacket(const Packet& packet, bool reliable, ENetPeer* excludePeer) {
    std::vector<uint8_t> buffer;
    packet.serialize(buffer);
    
    if (excludePeer == nullptr) {
        ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        enet_host_broadcast(m_server, 0, enetPacket);
    } else {
        for (size_t i = 0; i < m_server->peerCount; ++i) {
            ENetPeer* peer = &m_server->peers[i];
            if (peer->state != ENET_PEER_STATE_CONNECTED || peer == excludePeer) continue;
            
            ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
            enet_peer_send(peer, 0, enetPacket);
        }
    }
}

void Server::kick(ENetPeer* peer, const std::string& reason) {
    PacketDisconnect packet;
    packet.reason = reason;
    sendPacket(peer, packet, true);
    enet_peer_disconnect_later(peer, 0);
}

bool Server::isLocalhost(ENetPeer* peer) const {
    if (!peer || !m_server) return false;
    ENetAddress addr = peer->address;
    return addr.host == ENET_HOST_ANY || addr.host == 0x0100007F || addr.host == 0x00000000;
}
