#pragma once

#include "Minecraft.hpp"
#include "world/World.hpp"
#include "entities/EntityPlayer.hpp"
#include "physics/AxisAlignedBB.hpp"

class Minecraft;
struct HitResult;

void handleBlockBreaking(Minecraft& mc, EntityPlayer& player, World& world, float partialTicks);
void handleBlockPlacement(Minecraft& mc, EntityPlayer& player, World& world);
HitResult updateMouseOver(Minecraft& mc, EntityPlayer& player, World& world);
