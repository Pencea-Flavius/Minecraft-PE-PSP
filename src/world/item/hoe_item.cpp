#include "world/entity/local_player.h"
#include "world/item/hoe_item.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/tile/tile.h"
#include "world/entity/item_entity.h"
#include "world/inventory/inventory.h"
#include <stdlib.h>

bool HoeItem::useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float) {
    if (face == F_DOWN) return false;

    unsigned char targetId = worldBlock(world, x, y, z);
    unsigned char aboveId  = worldBlock(world, x, y + 1, z);

    if (aboveId == BLOCK_AIR && (targetId == BLOCK_GRASS || targetId == BLOCK_DIRT)) {

        {
            const SoundType& s = g_tileSounds[Tile::tiles[BLOCK_FARMLAND]->soundType];
            g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, s.stepSound,
                              (s.volume + 1.0f) / 2.0f, s.pitch * 0.8f);
        }
        worldSetBlockAndData(world, x, y, z, BLOCK_FARMLAND, 0);
        worldUpdateLights(world);
        worldRebuildAroundNow(world, x, y, z);

        g_level.player->inventory->hurtSelected(1);

        if (targetId == BLOCK_GRASS && rand() % 8 == 0) {
            const float s = 0.7f;
            float xo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
            float yo = (rand() / (float)RAND_MAX) * s + (1 - s) * 2.5f;
            float zo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
            ItemEntity* e = new ItemEntity(&g_level, x + xo, y + yo, z + zo,
                                           ItemInstance(ITEM_SEEDS_WHEAT, 1, 0));
            e->throwTime = 10;
            g_level.addEntity(e);
        }
        return true;
    }
    return false;
}
