#pragma once

#include "net/NetworkHandler.hpp"
#include "net/Packets.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"
#include <cstdint>
#include <cstddef>

void handleClientPacket(NetworkHandler& handler, World& world, EntityPlayer& player, int32_t& playerID,
                        const uint8_t* ptr, size_t size, PacketType type);
