#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <map>
#include <string>
#include "net/Server.hpp"
#include "world/World.hpp"

class IntegratedServer {
public:
    IntegratedServer();
    ~IntegratedServer();

    void start();
    void stop();

private:
    void run();
    void onPacketReceived(ENetPeer* peer, const uint8_t* data, size_t size);

    std::unique_ptr<Server> m_server;
    std::unique_ptr<World> m_world;
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    struct PlayerSession {
        int32_t entityID;
        std::string username;
    };
    std::map<ENetPeer*, PlayerSession> m_players;
};
