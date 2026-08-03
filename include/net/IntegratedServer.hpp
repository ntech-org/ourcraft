#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <map>
#include <string>
#include "net/Server.hpp"
#include "net/Permissions.hpp"
#include "net/CommandHandler.hpp"
#include "net/PlayerSession.hpp"
#include "net/RegistrationManager.hpp"
#include "net/ServerConfig.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"

class ServerPacketHandler;

class IntegratedServer {
public:
    explicit IntegratedServer(const std::string& worldName = "world");
    ~IntegratedServer();

    void start();
    void stop();
    void setDedicated(bool dedicated) { m_isDedicated = dedicated; }
    void setPaused(bool paused) { m_paused = paused; }
    void setChunkKeepDistance(int dist) { m_chunkKeepDistance.store(dist, std::memory_order_release); }
    bool isPaused() const { return m_paused; }
    bool isRunning() const { return m_running; }
    bool isDedicated() const { return m_isDedicated; }
    int getPlayerCount() const { return (int)m_players.size(); }

    Server* getServer() const { return m_server.get(); }
    World* getWorld() const { return m_world.get(); }
    Permissions& getPermissions() { return m_permissions; }
    CommandHandler& getCommandHandler() { return m_commandHandler; }
    RegistrationManager& getRegistrationManager() { return m_registrationManager; }
    ServerConfig& getConfig() { return m_config; }
    std::map<ENetPeer*, PlayerSession>& getPlayers() { return m_players; }

    void setHostUsername(const std::string& name) { m_hostUsername = name; }
    const std::string& getHostUsername() const { return m_hostUsername; }

    void broadcastSound(const std::string& name, double x, double y, double z, float volume, float pitch, ENetPeer* excludePeer = nullptr);
    void broadcastChat(const std::string& sender, const std::string& message, ENetPeer* excludePeer = nullptr);

private:
    void run();
    void tick();

    std::unique_ptr<Server> m_server;
    std::unique_ptr<World> m_world;
    std::unique_ptr<ServerPacketHandler> m_packetHandler;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    bool m_isDedicated = false;

    std::map<ENetPeer*, PlayerSession> m_players;
    std::atomic<int> m_chunkKeepDistance{12};

    int m_tickCounter = 0;
    int m_unloadTimer = 0;
    int m_spawnTimer = 0;
    std::string m_hostUsername;

    ServerConfig m_config;
    Permissions m_permissions;
    CommandHandler m_commandHandler;
    RegistrationManager m_registrationManager;
};
