
#include "world/item/food_item.h"
#include "world/entity/player.h"
#include "world/level/level.h"
#include "client/gamemode/gamemode.h"
#include <stdlib.h>

extern Level g_level;

void FoodItem::use(ItemInstance* item, Player* player, World*) {
    if (g_gameMode && g_gameMode->isCreative()) return;
    if (player->health >= player->getMaxHealth()) return;
    player->startUsingItem(*item, getMaxUseDuration());
}

void FoodItem::useTimeDepleted(ItemInstance* item, Player* player) {
    --item->count;
    player->heal(nutrition);
    g_level.playSound(player, "random.burp", 0.5f, (rand() / (float)RAND_MAX) * 0.1f + 0.9f);
}

void BowlFoodItem::useTimeDepleted(ItemInstance* item, Player* player) {
    FoodItem::useTimeDepleted(item, player);
    *item = ItemInstance(ITEM_BOWL, 1, 0);
}
