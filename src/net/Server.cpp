#include "net/Server.hpp"
#include <stdexcept>

Server::Server(uint16_t port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_server = enet_host_create(&address, 32, 2, 0, 0);
    if (m_server == NULL) {
        throw std::runtime_error("An error occurred while trying to create an ENet server host.");
    }
}

Server::~Server() {
    enet_host_destroy(m_server);
}

void Server::poll() {
    ENetEvent event;
    while (enet_host_service(m_server, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (onClientConnected) onClientConnected(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (onPacketReceived) onPacketReceived(event.peer, event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
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
