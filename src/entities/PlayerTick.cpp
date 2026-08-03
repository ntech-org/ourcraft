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

void handleBlockPlacement(Minecraft& mc, EntityPlayer& player, World& world) {
    auto& bb = mc.getBlockBreaking();
    if (!mc.getInputHandler().isRightMouseDown() || bb.getRightClickDelayTimer() > 0) return;

    bb.resetBlockBreaking(true, mc, mc.getNetworkHandler(), mc.getGameRenderer());

    ItemStack& currentStack = player.inventory.getCurrentStack();
    if (!currentStack.isEmpty() && currentStack.itemID >= 256) {
        Item* item = Item::itemsList[currentStack.itemID];
        if (item) {
            int oldCount = currentStack.count;
            currentStack = item->onItemRightClick(currentStack, world, player);
            if (currentStack.count != oldCount || currentStack.isEmpty()) {
                bb.setRightClickDelayTimer(4);
                return;
            }
        }
    }

    HitResult& hit = bb.getObjectMouseOverRef();
    if (hit.type == HitType::BLOCK) {
        uint8_t targetID = world.getBlockID(hit.x, hit.y, hit.z);
        if (targetID > 0 && Block::blocksList[targetID]->onBlockActivated(world, hit.x, hit.y, hit.z, &player)) {
            mc.getNetworkHandler()->sendPlacement(hit.x, hit.y, hit.z, hit.sideHit, 0, 0);
            bb.setRightClickDelayTimer(4);
        } else {
            int itemID = player.inventory.getCurrentItemID();
            if (itemID >= 256 && Item::itemsList[itemID]) {
                mc.getNetworkHandler()->sendPlacement(hit.x, hit.y, hit.z, hit.sideHit, itemID, currentStack.metadata);
                if (Item::itemsList[itemID]->onItemUse(currentStack, player, world, hit.x, hit.y, hit.z, hit.sideHit)) {
                    player.swing();
                    bb.setRightClickDelayTimer(4);
                }
                return;
            }

            int x = hit.x, y = hit.y, z = hit.z;
            int face = hit.sideHit;
            if (face == 0) y--; else if (face == 1) y++;
            else if (face == 2) z--; else if (face == 3) z++;
            else if (face == 4) x--; else if (face == 5) x++;

            AxisAlignedBB blockBB((double)x, (double)y, (double)z, (double)x + 1.0, (double)y + 1.0, (double)z + 1.0);
            if (!player.boundingBox.intersectsWith(blockBB)) {
                itemID = player.inventory.getCurrentItemID();
                if (itemID > 0 && itemID < 256 && Block::blocksList[itemID]) {
                    Block* block = Block::blocksList[itemID];
                    if (!block->canPlaceBlockAt(world, x, y, z)) return;
                    const bool shouldConsume = player.gameMode == GameMode::SURVIVAL;
                    if (!shouldConsume || player.inventory.consumeCurrentItem(1)) {
                        world.setBlockAndMetadataWithNotify(x, y, z, (uint8_t)itemID, 0);
                        block->onBlockPlaced(world, x, y, z, face, 0.5f, 0.5f, 0.5f);
                        player.swing();
                        if (auto* b = Block::blocksList[itemID]) {
                            if (auto* snd = mc.getSoundPool().getRandom(b->stepSound->getBreakSound(), mc.getSoundSystem()))
                                mc.getSoundSystem().play3D(snd, x, y, z, mc.getSettings().soundVolume, 0.8f);
                        }
                        mc.getNetworkHandler()->sendPlacement(hit.x, hit.y, hit.z, hit.sideHit, itemID, 0);
                        bb.setRightClickDelayTimer(4);
                    }
                }
            }
        }
    }
}
