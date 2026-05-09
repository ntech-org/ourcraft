#include "net/Client.hpp"
#include <iostream>

Client::Client() {
    m_client = enet_host_create(NULL, 1, 2, 0, 0);
    if (m_client == NULL) {
        throw std::runtime_error("An error occurred while trying to create an ENet client host.");
    }
}

Client::~Client() {
    disconnect();
    enet_host_destroy(m_client);
}

bool Client::connect(const std::string& address, uint16_t port) {
    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;

    std::cout << "[Client] Connecting to " << address << ":" << port << "..." << std::endl;

    m_peer = enet_host_connect(m_client, &addr, 2, 0);
    if (m_peer == NULL) {
        std::cerr << "[Client] Failed to create ENet peer for connection." << std::endl;
        return false;
    }

    ENetEvent event;
    if (enet_host_service(m_client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "[Client] Connected to server successfully." << std::endl;
        return true;
    } else {
        std::cerr << "[Client] Connection attempt timed out or failed." << std::endl;
        enet_peer_reset(m_peer);
        m_peer = nullptr;
        return false;
    }
}

void Client::disconnect() {
    if (m_peer) {
        std::cout << "[Client] Disconnecting from server..." << std::endl;
        enet_peer_disconnect(m_peer, 0);
        ENetEvent event;
        while (enet_host_service(m_client, &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[Client] Disconnected from server." << std::endl;
                    m_peer = nullptr;
                    return;
                default:
                    break;
            }
        }
        std::cerr << "[Client] Disconnect timed out, resetting peer." << std::endl;
        enet_peer_reset(m_peer);
        m_peer = nullptr;
    }
}

void Client::sendPacket(const Packet& packet, bool reliable) {
    if (!m_peer) return;
    
    std::vector<uint8_t> buffer;
    packet.serialize(buffer);
    
    ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (enet_peer_send(m_peer, 0, enetPacket) < 0) {
        std::cerr << "[Client] Failed to send packet." << std::endl;
    }
}

void Client::poll() {
    ENetEvent event;
    while (enet_host_service(m_client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                if (onPacketReceived) {
                    onPacketReceived(event.packet->data, event.packet->dataLength);
                }
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                // ENet sets event.data to a non-zero value (often 0xFFFFFFFF) on timeouts
                if (event.data != 0) {
                    std::cout << "[Client] Connection timed out or lost." << std::endl;
                } else {
                    std::cout << "[Client] Server disconnected us." << std::endl;
                }
                m_peer = nullptr;
                break;
            default:
                break;
        }
    }
}
