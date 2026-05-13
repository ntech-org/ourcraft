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

    Server* getServer() const { return m_server.get(); }
    World* getWorld() const { return m_world.get(); }

    void broadcastSound(const std::string& name, double x, double y, double z, float volume, float pitch, ENetPeer* excludePeer = nullptr);

    struct PlayerSession {
        int32_t entityID;
        std::string username;
        std::string uuid;
        InventoryPlayer inventory;
        ItemStack cursorStack;
        double lastX = 0, lastY = 0, lastZ = 0;
        float footstepAccum = 0;
        bool wasInWater = false;
        std::unordered_map<uint64_t, ChunkState> sentChunks;
        std::unordered_set<int32_t> sentEntities;
        int lastHealth = 20;
        float accumulatedFall = 0.0f;
        double lastSentY = 0.0;
    };

private:
    void run();
    void onPacketReceived(ENetPeer* peer, const uint8_t* data, size_t size);

    std::unique_ptr<Server> m_server;
    std::unique_ptr<World> m_world;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    bool m_isDedicated = false;

    std::map<ENetPeer*, PlayerSession> m_players;
};
