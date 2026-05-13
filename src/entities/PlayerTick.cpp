#include "PlayerTick.hpp"
#include "Minecraft.hpp"
#include "net/Packets.hpp"
#include "world/Block.hpp"
#include "world/Material.hpp"
#include "entities/EntityItem.hpp"
#include "entities/EntityPlayer.hpp"
#include "items/Item.hpp"
#include "items/ItemTool.hpp"
#include <glm/geometric.hpp>
#include <algorithm>
#include <cmath>

void handleBlockBreaking(Minecraft& mc, EntityPlayer& player, World& world, float partialTicks) {
    const bool leftDown = mc.getInputHandler().isLeftMouseDown();
    const bool leftClick = mc.getInputHandler().isLeftClick();
    HitResult& hit = mc.getObjectMouseOverRef();

    if ((!leftDown || hit.type != HitType::BLOCK) && mc.getHitDelayTimer() <= 0) {
        mc.resetBlockBreaking(true);
    } else if (leftDown && hit.type == HitType::BLOCK && mc.getHitDelayTimer() <= 0) {
        const bool sameTarget = mc.isBreakingBlock() &&
                                mc.getBreakX() == hit.x &&
                                mc.getBreakY() == hit.y &&
                                mc.getBreakZ() == hit.z;

        if (!sameTarget) {
            mc.resetBlockBreaking(true);
            const uint8_t targetID = world.getBlockID(hit.x, hit.y, hit.z);
            if (targetID > 0 && Block::getHardness(targetID) >= 0.0f) {
                mc.setBreakingBlock(true);
                mc.setBreakX(hit.x);
                mc.setBreakY(hit.y);
                mc.setBreakZ(hit.z);
                mc.setBreakFace(hit.sideHit);
                mc.setBreakProgress(0.0f);
                mc.setBreakSwingTick(0);
                mc.getNetworkHandler()->sendDigging(DiggingAction::START, hit.x, hit.y, hit.z, hit.sideHit);
                player.swing();
                if (player.gameMode != GameMode::CREATIVE) {
                    const Block* block = Block::blocksList[targetID];
                    if (block) {
                        const SoundBuffer* snd = mc.getSoundPool().getRandom(block->stepSound->getBreakSound(), mc.getSoundSystem());
                        if (snd) mc.getSoundSystem().play3D(snd, (float)hit.x, (float)hit.y, (float)hit.z, mc.getSettings().soundVolume, 1.0f);
                    }
                }
            }
        }

        if (mc.isBreakingBlock()) {
            const uint8_t targetID = world.getBlockID(mc.getBreakX(), mc.getBreakY(), mc.getBreakZ());
            if (targetID == 0 || Block::getHardness(targetID) < 0.0f) {
                mc.resetBlockBreaking(true);
            } else if (player.gameMode == GameMode::CREATIVE) {
                world.setBlockWithNotify(mc.getBreakX(), mc.getBreakY(), mc.getBreakZ(), 0);
                player.swing();
                if (const Block* b = Block::blocksList[targetID]) {
                    if (auto* snd = mc.getSoundPool().getRandom(b->stepSound->getBreakSound(), mc.getSoundSystem()))
                        mc.getSoundSystem().play3D(snd, (float)mc.getBreakX(), (float)mc.getBreakY(), (float)mc.getBreakZ(), mc.getSettings().soundVolume, 1.0f);
                }
                mc.resetBlockBreaking(false);
                mc.setHitDelayTimer(5);
            } else {
                mc.setBreakProgress(std::min(1.0f, mc.getBreakProgress() + mc.getBreakDeltaForBlock(targetID)));
                if ((++mc.getBreakSwingTickRef() % 4) == 0) {
                    player.swing();
                }
                if (mc.getBreakProgress() >= 1.0f) {
                    mc.finishBreakingCurrentBlock();
                    // Damage the held tool
                    if (player.gameMode == GameMode::SURVIVAL) {
                        ItemStack& held = player.inventory.getCurrentStack();
                        if (!held.isEmpty() && Item::itemsList[held.itemID]) {
                            Item* item = Item::itemsList[held.itemID];
                            if (auto* tool = dynamic_cast<ItemTool*>(item)) {
                                held.damage += 1;
                                if (held.damage >= tool->maxDamage) {
                                    const Block* b = Block::blocksList[targetID];
                                    if (b) {
                                        auto* snd = mc.getSoundPool().getRandom("random.break", mc.getSoundSystem());
                                        if (snd) mc.getSoundSystem().play3D(snd, (float)mc.getBreakX(), (float)mc.getBreakY(), (float)mc.getBreakZ(), mc.getSettings().soundVolume, 1.0f);
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

    if (mc.isBreakingBlock()) {
        mc.getGameRenderer().setBlockBreakingOverlay(true, mc.getBreakX(), mc.getBreakY(), mc.getBreakZ(), mc.getBreakProgress());
    } else {
        mc.getGameRenderer().setBlockBreakingOverlay(false, 0, 0, 0, 0.0f);
    }
}

void handleBlockPlacement(Minecraft& mc, EntityPlayer& player, World& world) {
    if (!mc.getInputHandler().isRightMouseDown() || mc.getRightClickDelayTimer() > 0) return;

    mc.resetBlockBreaking(true);

    ItemStack& currentStack = player.inventory.getCurrentStack();
    if (!currentStack.isEmpty() && currentStack.itemID >= 256) {
        Item* item = Item::itemsList[currentStack.itemID];
        if (item) {
            int oldCount = currentStack.count;
            currentStack = item->onItemRightClick(currentStack, world, player);
            if (currentStack.count != oldCount || currentStack.isEmpty()) {
                mc.setRightClickDelayTimer(4);
                return;
            }
        }
    }

    HitResult& hit = mc.getObjectMouseOverRef();
    if (hit.type == HitType::BLOCK) {
        uint8_t targetID = world.getBlockID(hit.x, hit.y, hit.z);
        if (targetID > 0 && Block::blocksList[targetID]->onBlockActivated(world, hit.x, hit.y, hit.z, &player)) {
            mc.setRightClickDelayTimer(4);
        } else {
            int x = hit.x, y = hit.y, z = hit.z;
            int face = hit.sideHit;
            if (face == 0) y--; else if (face == 1) y++;
            else if (face == 2) z--; else if (face == 3) z++;
            else if (face == 4) x--; else if (face == 5) x++;

            AxisAlignedBB blockBB((double)x, (double)y, (double)z, (double)x + 1.0, (double)y + 1.0, (double)z + 1.0);
            if (!player.boundingBox.intersectsWith(blockBB)) {
                int itemID = player.inventory.getCurrentItemID();
                // Only place blocks (IDs < 256 and valid block), not tools/items
                if (itemID > 0 && itemID < 256 && Block::blocksList[itemID]) {
                    const bool shouldConsume = player.gameMode == GameMode::SURVIVAL;
                    if (!shouldConsume || player.inventory.consumeCurrentItem(1)) {
                        world.setBlockWithNotify(x, y, z, (uint8_t)itemID);
                        player.swing();
                        if (auto* b = Block::blocksList[itemID]) {
                            if (auto* snd = mc.getSoundPool().getRandom(b->stepSound->getBreakSound(), mc.getSoundSystem()))
                                mc.getSoundSystem().play3D(snd, (float)x, (float)y, (float)z, mc.getSettings().soundVolume, 0.8f);
                        }
                        mc.getNetworkHandler()->sendPlacement(hit.x, hit.y, hit.z, hit.sideHit, itemID, 0);
                        mc.setRightClickDelayTimer(4);
                    }
                }
            }
        }
    }
}

HitResult updateMouseOver(Minecraft& mc, EntityPlayer& player, World& world) {
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
    return hit;
}
