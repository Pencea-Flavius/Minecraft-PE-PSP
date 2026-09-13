
#include "world/item/bow_item.h"
#include "world/entity/player.h"
#include "world/entity/arrow.h"
#include "world/inventory/inventory.h"
#include "world/level/level.h"
#include "client/gamemode/gamemode.h"
#include <stdlib.h>

extern Level g_level;

float BowItem::_getLaunchPower(int duration) const {
    float p = (float)(getMaxUseDuration() - duration) / 20.0f;
    p = ((p * p) + p * 2.0f) / 3.0f;
    return (p > 1.0f) ? 1.0f : p;
}

void BowItem::use(ItemInstance* item, Player* player, World*) {
    if (!hasArrow(player)) return;
    player->startUsingItem(*item, getMaxUseDuration());
}

bool BowItem::hasArrow(Player* player) {
    if (player->inventory->isCreative()) return true;

    for (int i = player->inventory->firstGridSlot();
         i < player->inventory->getContainerSize(); i++) {
        ItemInstance* it = player->inventory->getItem(i);
        if (it && it->id == ITEM_ARROW && it->count > 0) return true;
    }
    return false;
}

void BowItem::releaseUsing(ItemInstance*, Player* player, int duration) {
    float pow = _getLaunchPower(duration);

    if (pow < 0.1f) return;
    if (player->inventory->removeResource(ItemInstance(ITEM_ARROW, 1, 0), true) != 0) return;

    g_level.addEntity(new Arrow(&g_level, player->x, player->y + player->getHeadHeight(),
                                player->z, player->yRot, player->xRot,
                                pow * 2.0f, pow >= 1.0f,  true));

    g_level.playSound(player, "random.bow", 1.0f,
                      1.0f / ((rand() / (float)RAND_MAX) * 0.4f + 1.2f) + pow * 0.5f);

    player->inventory->hurtSelected(1);
}
