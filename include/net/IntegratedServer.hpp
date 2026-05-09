#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <map>
#include <string>
#include <unordered_set>
#include "net/Server.hpp"
#include "world/World.hpp"
#include "entities/InventoryPlayer.hpp"

class IntegratedServer {
public:
    IntegratedServer();
    ~IntegratedServer();

    void start();
    void stop();
    void setDedicated(bool dedicated) { m_isDedicated = dedicated; }
    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }
    int getPlayerCount() const { return (int)m_players.size(); }

private:
    void run();
    void onPacketReceived(ENetPeer* peer, const uint8_t* data, size_t size);

    std::unique_ptr<Server> m_server;
    std::unique_ptr<World> m_world;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    bool m_isDedicated = false;

    struct PlayerSession {
        int32_t entityID;
        std::string username;
        InventoryPlayer inventory;
        ItemStack cursorStack;
        std::unordered_map<uint64_t, ChunkState> sentChunks;
        std::unordered_set<int32_t> sentEntities;
    };
    std::map<ENetPeer*, PlayerSession> m_players;
};
