#include "simulation/BlockBreakingSystem.hpp"
#include "Minecraft.hpp"
#include "world/Block.hpp"
#include "items/Item.hpp"
#include "items/ItemTool.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityPlayer.hpp"
#include "net/NetworkHandler.hpp"
#include "net/Packets.hpp"
#include "renderer/GameRenderer.hpp"
#include "InputHandler.hpp"
#include "sound/SoundSystem.hpp"
#include "sound/SoundPool.hpp"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

void BlockBreakingSystem::tick(Minecraft& mc, EntityPlayer& player, World& world, GameRenderer& renderer,
                               InputHandler& input, NetworkHandler* network, SoundSystem* sound, SoundPool& pool,
                               float soundVolume, float partialTicks, GameMode gameMode) {
    const bool leftDown = input.isLeftMouseDown();
    const bool leftClick = input.isLeftClick();

    if ((!leftDown || m_objectMouseOver.type != HitType::BLOCK) && m_hitDelayTimer <= 0) {
        resetBlockBreaking(true, mc, network, renderer);
    } else if (leftDown && m_objectMouseOver.type == HitType::BLOCK && m_hitDelayTimer <= 0) {
        const bool sameTarget = m_isBreakingBlock &&
                                m_breakX == m_objectMouseOver.x &&
                                m_breakY == m_objectMouseOver.y &&
                                m_breakZ == m_objectMouseOver.z;

        if (!sameTarget) {
            resetBlockBreaking(true, mc, network, renderer);
            const uint8_t targetID = world.getBlockID(m_objectMouseOver.x, m_objectMouseOver.y, m_objectMouseOver.z);
            if (targetID > 0 && Block::getHardness(targetID) >= 0.0f) {
                m_isBreakingBlock = true;
                m_breakX = m_objectMouseOver.x;
                m_breakY = m_objectMouseOver.y;
                m_breakZ = m_objectMouseOver.z;
                m_breakFace = m_objectMouseOver.sideHit;
                m_breakProgress = 0.0f;
                m_breakSwingTick = 0;
                network->sendDigging(DiggingAction::START, m_objectMouseOver.x, m_objectMouseOver.y, m_objectMouseOver.z, m_objectMouseOver.sideHit);
                player.swing();
                if (gameMode != GameMode::CREATIVE) {
                    const Block* block = Block::blocksList[targetID];
                    if (block) {
                        const SoundBuffer* snd = pool.getRandom(block->stepSound->getBreakSound(), *sound);
                        if (snd) sound->play3D(snd, m_objectMouseOver.x, m_objectMouseOver.y, m_objectMouseOver.z, soundVolume, 1.0f);
                    }
                }
            }
        }

        if (m_isBreakingBlock) {
            const uint8_t targetID = world.getBlockID(m_breakX, m_breakY, m_breakZ);
            if (targetID == 0 || Block::getHardness(targetID) < 0.0f) {
                resetBlockBreaking(true, mc, network, renderer);
            } else if (gameMode == GameMode::CREATIVE) {
                finishBreakingCurrentBlock(mc, player, world, network, sound, pool, soundVolume);
                m_hitDelayTimer = 5;
            } else {
                m_breakProgress = std::min(1.0f, m_breakProgress + getBreakDeltaForBlock(targetID, player));
                if ((++m_breakSwingTick % 4) == 0) {
                    player.swing();
                }
                if (m_breakProgress >= 1.0f) {
                    finishBreakingCurrentBlock(mc, player, world, network, sound, pool, soundVolume);
                    if (gameMode == GameMode::SURVIVAL) {
                        ItemStack& held = player.inventory.getCurrentStack();
                        if (!held.isEmpty() && Item::itemsList[held.itemID]) {
                            Item* item = Item::itemsList[held.itemID];
                            if (auto* tool = dynamic_cast<ItemTool*>(item)) {
                                held.damage += 1;
                                if (held.damage >= tool->maxDamage) {
                                    const Block* b = Block::blocksList[targetID];
                                    if (b) {
                                        auto* snd = pool.getRandom("random.break", *sound);
                                        if (snd) sound->play3D(snd, m_breakX, m_breakY, m_breakZ, soundVolume, 1.0f);
                                    }
                                    held = {0, 0, 0};
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (m_isBreakingBlock) {
        renderer.setBlockBreakingOverlay(true, m_breakX, m_breakY, m_breakZ, m_breakProgress);
    } else {
        renderer.setBlockBreakingOverlay(false, 0, 0, 0, 0.0f);
    }
}

void BlockBreakingSystem::resetBlockBreaking(bool sendStopPacket, Minecraft& mc, NetworkHandler* network, GameRenderer& renderer) {
    if (sendStopPacket && m_isBreakingBlock && network) {
        network->sendDigging(DiggingAction::STOP, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
    }
    m_isBreakingBlock = false;
    m_breakFace = -1;
    m_breakProgress = 0.0f;
    m_breakSwingTick = 0;
    renderer.setBlockBreakingOverlay(false, 0, 0, 0, 0.0f);
}

float BlockBreakingSystem::getBreakDeltaForBlock(uint8_t blockID, const EntityPlayer& player) const {
    const float hardness = Block::getHardness(blockID);
    if (hardness <= 0.0f) {
        return 1.0f;
    }

    const Block* block = Block::blocksList[blockID];
    if (!block) return 1.0f;

    const ItemStack& held = player.inventory.getCurrentStack();

    if (!held.isEmpty()) {
        if (Item* item = Item::itemsList[held.itemID]) {
            if (item->canHarvestBlock(*block)) {
                float strength = item->getStrVsBlock(*block);
                if (player.inWater) strength /= 5.0f;
                if (!player.onGround) strength /= 5.0f;
                return strength / hardness / 30.0f;
            } else {
                return 1.0f / hardness / 30.0f;
            }
        }
    }

    return 1.0f / hardness / 30.0f;
}

bool BlockBreakingSystem::finishBreakingCurrentBlock(Minecraft& mc, EntityPlayer& player, World& world,
                                                     NetworkHandler* network, SoundSystem* sound, SoundPool& pool,
                                                     float soundVolume) {
    if (!m_isBreakingBlock) {
        return false;
    }

    const uint8_t targetID = world.getBlockID(m_breakX, m_breakY, m_breakZ);
    if (targetID == 0 || Block::getHardness(targetID) < 0.0f) {
        resetBlockBreaking(false, mc, network, mc.getGameRenderer());
        return false;
    }

    if (player.gameMode == GameMode::CREATIVE) {
        world.setBlockWithNotify(m_breakX, m_breakY, m_breakZ, 0);
    }

    m_objectMouseOver.type = HitType::NONE;
    network->sendDigging(DiggingAction::FINISH, m_breakX, m_breakY, m_breakZ, m_breakFace >= 0 ? m_breakFace : 1);
    player.swing();
    if (const Block* b = Block::blocksList[targetID]) {
        if (auto* snd = pool.getRandom(b->stepSound->getBreakSound(), *sound))
            sound->play3D(snd, m_breakX, m_breakY, m_breakZ, soundVolume, 1.0f);
    }
    resetBlockBreaking(false, mc, network, mc.getGameRenderer());
    return true;
}

HitResult BlockBreakingSystem::updateMouseOver(const EntityPlayer& player, World& world) {
    float reach = 5.0f;
    glm::dvec3 eyePos = glm::dvec3(player.posX, player.posY + 1.62f, player.posZ);
    float yaw = glm::radians(player.rotationYaw);
    float pitch = glm::radians(player.rotationPitch);
    glm::dvec3 lookDir = glm::dvec3(
        -std::sin(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::cos(yaw) * std::cos(pitch)
    );

    glm::dvec3 endPos = eyePos + lookDir * (double)reach;
    HitResult hit = world.rayTraceBlocks(eyePos, endPos, true);

    double dist = reach;
    if (hit.type == HitType::BLOCK) {
        dist = glm::distance(eyePos, hit.hitVec);
    }

    AxisAlignedBB reachBB = AxisAlignedBB(
        std::min(eyePos.x, endPos.x), std::min(eyePos.y, endPos.y), std::min(eyePos.z, endPos.z),
        std::max(eyePos.x, endPos.x), std::max(eyePos.y, endPos.y), std::max(eyePos.z, endPos.z)
    ).expand(1.0, 1.0, 1.0);

    std::vector<Entity*> entities = world.getEntitiesWithinAABB(reachBB);
    for (Entity* entity : entities) {
        if (entity == &player || dynamic_cast<EntityItem*>(entity)) continue;

        float border = 0.1f;
        AxisAlignedBB entityBB = entity->boundingBox.expand(border, border, border);
        auto intercept = entityBB.calculateIntercept(eyePos, endPos);
        if (intercept) {
            double d = glm::distance(eyePos, intercept->hitVec);
            if (d < dist) {
                hit.type = HitType::ENTITY;
                hit.entity = entity;
                hit.hitVec = intercept->hitVec;
                dist = d;
            }
        }
    }
    m_objectMouseOver = hit;
    return hit;
}
