#pragma once

#include <enet/enet.h>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "net/Packet.hpp"
#include "util/ThreadSafeQueue.hpp"

class Client {
public:
    Client();
    ~Client();

    bool connect(const std::string& address, uint16_t port);
    void disconnect(const std::string& reason = "Disconnecting");
    void sendPacket(const Packet& packet, bool reliable = true);
    void poll();

    bool isConnected() const { return m_peer != nullptr && m_connected; }
    bool isConnecting() const { return m_connecting; }

    std::function<void()> onConnected;
    std::function<void(const uint8_t*, size_t)> onPacketReceived;
    std::function<void(bool, const std::string&)> onDisconnected;

private:
    void networkLoop();

    ENetHost* m_client = nullptr;
    ENetPeer* m_peer = nullptr;

    std::thread m_networkThread;
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_connecting{false};
    std::atomic<bool> m_disconnectPacketReceived{false};

    std::chrono::steady_clock::time_point m_connectStartTime;

    struct QueuedPacket {
        std::vector<uint8_t> data;
        bool reliable;
    };

    ThreadSafeQueue<QueuedPacket> m_outgoingPackets;
    ThreadSafeQueue<std::vector<uint8_t>> m_incomingPackets;
    ThreadSafeQueue<bool> m_connectEvents;
    
    struct DisconnectEvent {
        bool timeout;
        std::string reason;
    };
    ThreadSafeQueue<DisconnectEvent> m_disconnectEvents;
};
