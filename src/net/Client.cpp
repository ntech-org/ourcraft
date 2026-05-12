#include "net/Client.hpp"
#include "net/Packets.hpp"
#include <iostream>
#include <chrono>

Client::Client() {
    m_client = enet_host_create(NULL, 1, 2, 0, 0);
    if (m_client == NULL) {
        throw std::runtime_error("An error occurred while trying to create an ENet client host.");
    }
    m_networkThread = std::thread(&Client::networkLoop, this);
}

Client::~Client() {
    m_running = false;
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
    if (m_peer) {
        enet_peer_reset(m_peer);
    }
    enet_host_destroy(m_client);
}

bool Client::connect(const std::string& address, uint16_t port) {
    if (m_connected || m_connecting) return false;

    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;

    std::cout << "[Client] Connecting to " << address << ":" << port << "..." << std::endl;

    m_connecting = true;
    m_connectStartTime = std::chrono::steady_clock::now();
    m_peer = enet_host_connect(m_client, &addr, 2, 0);
    if (m_peer == NULL) {
        std::cerr << "[Client] Failed to create ENet peer for connection." << std::endl;
        m_connecting = false;
        return false;
    }

    return true;
}

void Client::disconnect(const std::string& reason) {
    if (m_peer) {
        PacketDisconnect packet;
        packet.reason = reason;
        sendPacket(packet, true);

        // The network loop will handle the actual enet_peer_disconnect_later
        // after the packet is sent. Or we can just set a flag.
        // For now, let's just trigger it.
        enet_peer_disconnect_later(m_peer, 0);
        m_connected = false;
    }
}

void Client::sendPacket(const Packet& packet, bool reliable) {
    std::vector<uint8_t> buffer;
    packet.serialize(buffer);
    m_outgoingPackets.push({std::move(buffer), reliable});
}

void Client::poll() {
    // Process connection events
    while (auto success = m_connectEvents.pop()) {
        if (*success && onConnected) {
            onConnected();
        }
    }

    // Process incoming packets
    while (auto data = m_incomingPackets.pop()) {
        if (onPacketReceived) {
            onPacketReceived(data->data(), data->size());
        }
    }

    // Process disconnect events
    while (auto event = m_disconnectEvents.pop()) {
        if (onDisconnected) {
            onDisconnected(event->timeout, event->reason);
        }
    }
}

void Client::networkLoop() {
    while (m_running) {
        bool activity = false;

        // 1. Send outgoing packets
        while (auto queued = m_outgoingPackets.pop()) {
            if (m_peer) {
                ENetPacket* enetPacket = enet_packet_create(queued->data.data(), queued->data.size(), queued->reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
                enet_peer_send(m_peer, 0, enetPacket);
                activity = true;
            }
        }

        // 2. Poll ENet
        ENetEvent event;
        if (m_connecting) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_connectStartTime).count() > 5000) {
                std::cout << "[Client] Connection attempt timed out." << std::endl;
                m_connecting = false;
                if (m_peer) {
                    enet_peer_reset(m_peer);
                    m_peer = nullptr;
                }
                m_disconnectEvents.push({true, "Connection timed out"});
            }
        }

        while (enet_host_service(m_client, &event, 0) > 0) {
            activity = true;
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "[Client] Connected to server successfully." << std::endl;
                    m_connected = true;
                    m_connecting = false;
                    m_connectEvents.push(true);
                    break;
                case ENET_EVENT_TYPE_RECEIVE: {
                    const uint8_t* ptr = event.packet->data;
                    PacketType type = (PacketType)Packet::readByte(ptr);

                    if (type == PacketType::Disconnect) {
                        PacketDisconnect p;
                        p.deserialize(ptr, event.packet->dataLength - 1);
                        std::cout << "[Client] Server kicked us: " << p.reason << std::endl;
                        m_disconnectEvents.push({false, p.reason});
                    } else {
                        std::vector<uint8_t> data(event.packet->data, event.packet->data + event.packet->dataLength);
                        m_incomingPackets.push(std::move(data));
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    bool timeout = event.data != 0;
                    std::string reason = timeout ? "Connection timed out" : "Disconnected by server";
                    std::cout << "[Client] " << reason << std::endl;
                    m_peer = nullptr;
                    m_connected = false;
                    m_connecting = false;
                    m_disconnectEvents.push({timeout, reason});
                    break;
                }
                default:
                    break;
            }
        }

        if (!activity) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
