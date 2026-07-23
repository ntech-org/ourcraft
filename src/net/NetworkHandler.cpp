#include "net/NetworkHandler.hpp"
#include "net/ClientPacketHandler.hpp"
#include "net/NetworkManager.hpp"
#include "net/Packets.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityZombie.hpp"
#include <iostream>
#include <cstring>
#include <chrono>

NetworkHandler::NetworkHandler(World& world, EntityPlayer& player, bool startServer)
    : m_world(world), m_player(player)
{
    NetworkManager::init();

    if (startServer) {
        m_server = std::make_unique<IntegratedServer>();
        m_server->start();
    }

    m_client = std::make_unique<Client>();
    m_client->onPacketReceived = [this](const uint8_t* data, size_t size) {
        this->onPacketReceived(data, size);
    };
    m_client->onDisconnected = [this](bool timeout, const std::string& reason) {
        if (onDisconnected) onDisconnected(timeout, reason);
    };
    m_client->onConnected = [this]() {
        PacketLogin loginPacket;
        loginPacket.username = m_player.username;
        loginPacket.uuid = m_player.uuid;
        loginPacket.protocolVersion = 1;
        m_client->sendPacket(loginPacket);
    };
}

NetworkHandler::~NetworkHandler() {}

bool NetworkHandler::connect(const std::string& address, int port) {
    return m_client->connect(address, port);
}

void NetworkHandler::setRenderDistance(float dist) {
    if (m_server) {
        int keepDist = 4 + (int)(dist * 8);
        if (keepDist < 4) keepDist = 4;
        if (keepDist > 24) keepDist = 24;
        m_server->setChunkKeepDistance(keepDist);
    }
}

void NetworkHandler::update() {
    m_client->poll();

    if (++m_posUpdateTimer >= 20) {
        PacketPlayerRotation packet;
        packet.yaw = m_player.rotationYaw; packet.pitch = m_player.rotationPitch;
        packet.onGround = m_player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    }
}

void NetworkHandler::stopServer() {
    m_server.reset();
}

void NetworkHandler::setPaused(bool paused) {
    if (m_server) {
        m_server->setPaused(paused);
    }
}

bool NetworkHandler::isSingleplayer() const {
    return m_server != nullptr;
}

int NetworkHandler::getPlayerCount() const {
    return m_server ? m_server->getPlayerCount() : 0;
}

void NetworkHandler::sendPlayerPosition(const EntityPlayer& player) {
    double dx = player.posX - m_lastX;
    double dy = player.posY - m_lastY;
    double dz = player.posZ - m_lastZ;
    float dYaw = player.rotationYaw - m_lastYaw;
    float dPitch = player.rotationPitch - m_lastPitch;

    bool moved = (dx * dx + dy * dy + dz * dz) > 9e-4;
    bool turned = std::abs(dYaw) > 0.1f || std::abs(dPitch) > 0.1f;

    if (moved && turned) {
        PacketPlayerPosLook packet;
        packet.x = player.posX; packet.y = player.posY; packet.z = player.posZ;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch;
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    } else if (moved) {
        PacketPlayerPosition packet;
        packet.x = player.posX; packet.y = player.posY; packet.z = player.posZ;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch;
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    } else if (turned) {
        PacketPlayerRotation packet;
        packet.yaw = player.rotationYaw; packet.pitch = player.rotationPitch;
        packet.onGround = player.onGround;
        m_client->sendPacket(packet, false);
        m_posUpdateTimer = 0;
    }

    if (moved) { m_lastX = player.posX; m_lastY = player.posY; m_lastZ = player.posZ; }
    if (turned) { m_lastYaw = player.rotationYaw; m_lastPitch = player.rotationPitch; }
}

void NetworkHandler::sendDigging(DiggingAction action, int x, int y, int z, int face) {
    PacketPlayerDigging packet;
    packet.action = action;
    packet.x = x; packet.y = y; packet.z = z;
    packet.face = (uint8_t)face;
    m_client->sendPacket(packet, true);
}

void NetworkHandler::sendPlacement(int x, int y, int z, int face, int id, int meta) {
    PacketBlockPlacement packet;
    packet.x = x; packet.y = y; packet.z = z;
    packet.face = (uint8_t)face;
    packet.blockID = (uint8_t)id;
    packet.metadata = (uint8_t)meta;
    m_client->sendPacket(packet, true);
}

void NetworkHandler::sendPacket(const Packet& packet) {
    if (m_client) {
        m_client->sendPacket(packet);
    }
}

void NetworkHandler::sendChatMessage(const std::string& message) {
    if (m_client) {
        PacketChatMessage packet;
        packet.sender = m_player.username;
        packet.message = message;
        packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        m_client->sendPacket(packet);
    }
}

CommandHandler* NetworkHandler::getCommandHandler() {
    if (m_server) {
        return &m_server->getCommandHandler();
    }
    return nullptr;
}

void NetworkHandler::onPacketReceived(const uint8_t* data, size_t size) {
    const uint8_t* ptr = data;
    PacketType type = (PacketType)Packet::readByte(ptr);
    handleClientPacket(*this, m_world, m_player, m_playerID, ptr, size, type);
}
