#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include "entities/InventoryPlayer.hpp"
#include "world/Chunk.hpp"
#include "entities/EntityPlayer.hpp"

struct PlayerSession {
    int32_t entityID;
    std::string username;
    std::string uuid;
    GameMode gameMode = GameMode::SURVIVAL;
    double lastX = 0, lastY = 0, lastZ = 0;
    float footstepAccum = 0;
    bool wasInWater = false;
    std::unordered_map<uint64_t, ChunkState> sentChunks;
    std::unordered_set<int32_t> sentEntities;
    int lastHealth = 20;
    float accumulatedFall = 0.0f;
    double lastSentY = 0.0;
    int streamChunkX = std::numeric_limits<int>::min();
    int streamChunkZ = std::numeric_limits<int>::min();
    bool chunkStreamPending = true;
    int openChestX[2] = {0, 0};
    int openChestY[2] = {0, 0};
    int openChestZ[2] = {0, 0};
    int openChestCount = 0;
};
