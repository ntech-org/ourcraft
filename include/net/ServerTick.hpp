#pragma once

#include "net/Server.hpp"
#include "net/PlayerSession.hpp"
#include "world/World.hpp"
#include <map>

struct _ENetPeer;
typedef struct _ENetPeer ENetPeer;

void saveAllPlayers(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players);
void broadcastEntityPositions(World& world, Server& server);
void handleRespawns(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players);
void handleItemPickups(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                       const std::unordered_map<int32_t, Entity*>& entitiesById);
void spawnNewEntities(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players);
void pushChunksToPlayers(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                         const std::unordered_map<int32_t, Entity*>& entitiesById, int chunkKeepDistance);
void unloadFarChunks(World& world, Server& server, std::map<ENetPeer*, PlayerSession>& players,
                     const std::unordered_map<int32_t, Entity*>& entitiesById, int keepDistance);
void spawnMobs(World& world, std::map<ENetPeer*, PlayerSession>& players,
               const std::unordered_map<int32_t, Entity*>& entitiesById);
