#pragma once

#include "world/World.hpp"
#include "physics/AxisAlignedBB.hpp"
#include "entities/EntityPlayer.hpp"

class Minecraft;
class EntityPlayer;
class GameRenderer;
class InputHandler;
class NetworkHandler;
class SoundSystem;
class SoundPool;

class BlockBreakingSystem {
public:
    BlockBreakingSystem() = default;

    void tick(Minecraft& mc, EntityPlayer& player, World& world, GameRenderer& renderer,
              InputHandler& input, NetworkHandler* network, SoundSystem* sound, SoundPool& pool,
              float soundVolume, float partialTicks, GameMode gameMode);

    void resetBlockBreaking(bool sendStopPacket, Minecraft& mc, NetworkHandler* network, GameRenderer& renderer);
    float getBreakDeltaForBlock(uint8_t blockID, const EntityPlayer& player) const;
    bool finishBreakingCurrentBlock(Minecraft& mc, EntityPlayer& player, World& world,
                                    NetworkHandler* network, SoundSystem* sound, SoundPool& pool,
                                    float soundVolume);
    HitResult updateMouseOver(const EntityPlayer& player, World& world);

    bool isBreakingBlock() const { return m_isBreakingBlock; }
    int getBreakX() const { return m_breakX; }
    int getBreakY() const { return m_breakY; }
    int getBreakZ() const { return m_breakZ; }
    int getBreakFace() const { return m_breakFace; }
    float getBreakProgress() const { return m_breakProgress; }
    int getBreakSwingTick() const { return m_breakSwingTick; }
    int getHitDelayTimer() const { return m_hitDelayTimer; }
    int getRightClickDelayTimer() const { return m_rightClickDelayTimer; }
    const HitResult& getObjectMouseOver() const { return m_objectMouseOver; }
    HitResult& getObjectMouseOverRef() { return m_objectMouseOver; }

    void setHitDelayTimer(int v) { m_hitDelayTimer = v; }
    void setRightClickDelayTimer(int v) { m_rightClickDelayTimer = v; }

private:
    bool m_isBreakingBlock = false;
    int m_breakX = 0;
    int m_breakY = 0;
    int m_breakZ = 0;
    int m_breakFace = -1;
    float m_breakProgress = 0.0f;
    int m_breakSwingTick = 0;

    int m_hitDelayTimer = 0;
    int m_rightClickDelayTimer = 0;
    HitResult m_objectMouseOver;
};
