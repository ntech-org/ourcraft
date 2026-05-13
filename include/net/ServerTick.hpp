#pragma once

#include "net/IntegratedServer.hpp"
#include "net/Server.hpp"
#include "world/World.hpp"
#include <map>

void saveAllPlayers(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players);
void broadcastEntityPositions(World& world, Server& server);
void handleRespawns(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players);
void handleItemPickups(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players,
                       const std::unordered_map<int32_t, Entity*>& entitiesById);
void spawnNewEntities(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players);
void pushChunksToPlayers(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players,
                         const std::unordered_map<int32_t, Entity*>& entitiesById);
void unloadFarChunks(World& world, Server& server, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players,
                     const std::unordered_map<int32_t, Entity*>& entitiesById);
void spawnMobs(World& world, std::map<ENetPeer*, IntegratedServer::PlayerSession>& players,
               const std::unordered_map<int32_t, Entity*>& entitiesById);
