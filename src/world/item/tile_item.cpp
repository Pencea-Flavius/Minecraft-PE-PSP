#include "world/inventory/inventory.h"
#include "world/item/tile_item.h"
#include "world/item/item_instance.h"
#include "world/level/tile/tile.h"
#include "world/level/tile/tile_behavior.h"
#include "world/level/tile/fire.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "world/entity/player.h"
#include "world/entity/local_player.h"
#include "client/player/player_state.h"
#include <math.h>

bool placeTileResolved(World* w, int nx, int ny, int nz, int tileId, int data, Player* placer) {
    if (!worldSetBlockAndData(w, nx, ny, nz, (unsigned char)tileId, (unsigned char)data)) return false;
    Tile* tile = Tile::tiles[tileId & 0xFF];
    tile->setPlacedBy(w, nx, ny, nz, placer);
    const SoundType& s = g_tileSounds[tile->soundType];
    if (s.stepSound)
        g_level.playSound(nx + 0.5f, ny + 0.5f, nz + 0.5f, s.stepSound,
                          (s.volume + 1.0f) / 2.0f, s.pitch * 0.8f);
    worldNotifyNeighborsChanged(w, nx, ny, nz);
    worldUpdateLights(w);
    worldRebuildAroundNow(w, nx, ny, nz);
    return true;
}

bool TileItem::useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                     float clickX, float clickY, float clickZ) {
    if (!item || item->isNull()) return false;

    int nx = x, ny = y, nz = z;
    if (!isReplaceable(worldBlock(world, x, y, z))) {
        nx += kFaceNeighbor[face][0];
        ny += kFaceNeighbor[face][1];
        nz += kFaceNeighbor[face][2];
    }

    Tile* tile = Tile::tiles[tileId & 0xFF];

    int data = tile->getPlacedOnFaceDataValue(world, nx, ny, nz, face, clickX, clickY, clickZ, item->data);
    if (!tileMayPlace(world, (unsigned char)tileId, nx, ny, nz, face)) return false;
    if (!placeTileResolved(world, nx, ny, nz, tileId, data, player)) return false;

    if (player) player->inventory->consumeSelected();
    return true;
}

bool SlabItem::useOn(ItemInstance* item, Player* player, World* w, int x, int y, int z, int face,
                     float clickX, float clickY, float clickZ) {
    if (!item || item->isNull()) return false;

    unsigned char currentTile = worldBlock(w, x, y, z);
    unsigned char currentData = worldData(w, x, y, z);
    int  slabType = currentData & DSLAB_MAT_MASK;
    bool isUpper  = (currentData & SLAB_TOP_SLOT_BIT) != 0;

    if (((face == F_TOP && !isUpper) || (face == F_DOWN && isUpper)) &&
        currentTile == tileId && slabType == (item->data & DSLAB_MAT_MASK)) {

        Tile* dbl = Tile::tiles[BLOCK_DOUBLE_SLAB];
        BlockAABB boxes[3];
        int n = dbl->getAABB(w, x, y, z, boxes);
        for (int i = 0; i < n; i++) {
            AABB box(boxes[i].x0, boxes[i].y0, boxes[i].z0, boxes[i].x1, boxes[i].y1, boxes[i].z1);
            if (!g_level.isUnobstructed(box)) return true;
        }
        if (worldSetBlockAndData(w, x, y, z, BLOCK_DOUBLE_SLAB, (unsigned char)slabType)) {
            const SoundType& snd = g_tileSounds[dbl->soundType];
            if (snd.stepSound)
                g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, snd.stepSound,
                                  (snd.volume + 1.0f) / 2.0f, snd.pitch * 0.8f);
            worldNotifyNeighborsChanged(w, x, y, z);
            worldUpdateLights(w);
            worldRebuildAroundNow(w, x, y, z);
            if (player) player->inventory->consumeSelected();
        }
        return true;
    }
    return TileItem::useOn(item, player, w, x, y, z, face, clickX, clickY, clickZ);
}

static int placementQuadrant(Player* p) {
    float yaw = p ? p->yRot : 0.0f;
    while (yaw < 0.0f)    yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;
    int q = ((int)floorf(yaw * 4.0f / 360.0f + 0.5f)) & 3;
    return (4 - q) & 3;
}

static void doorPlace(World* w, int x, int y, int z, int dir, short tileId) {
    int xra = 0, zra = 0;
    if (dir == 0) zra = +1;
    if (dir == 1) xra = -1;
    if (dir == 2) zra = -1;
    if (dir == 3) xra = +1;

    int solidLeft  = (isSolidPhys(worldBlock(w, x - xra, y, z - zra))     ? 1 : 0)
                   + (isSolidPhys(worldBlock(w, x - xra, y + 1, z - zra)) ? 1 : 0);
    int solidRight = (isSolidPhys(worldBlock(w, x + xra, y, z + zra))     ? 1 : 0)
                   + (isSolidPhys(worldBlock(w, x + xra, y + 1, z + zra)) ? 1 : 0);
    bool doorLeft  = (worldBlock(w, x - xra, y, z - zra) == tileId)
                  || (worldBlock(w, x - xra, y + 1, z - zra) == tileId);
    bool doorRight = (worldBlock(w, x + xra, y, z + zra) == tileId)
                  || (worldBlock(w, x + xra, y + 1, z + zra) == tileId);

    bool flip = false;
    if (doorLeft && !doorRight) flip = true;
    else if (solidRight > solidLeft) flip = true;

    worldSetBlockAndData(w, x, y, z, (unsigned char)tileId, (unsigned char)dir);
    worldSetBlockAndData(w, x, y + 1, z, (unsigned char)tileId, (unsigned char)(8 | (flip ? 1 : 0)));
    worldNotifyNeighborsChanged(w, x, y, z);
    worldNotifyNeighborsChanged(w, x, y + 1, z);
    worldUpdateLights(w);
    worldRebuildAroundNow(w, x, y, z);
    worldRebuildAroundNow(w, x, y + 1, z);
}

bool DoorItem::useOn(ItemInstance* item, Player* player, World* w, int x, int y, int z, int face,
                     float, float, float) {
    if (!item || item->isNull()) return false;
    if (face != F_TOP) return false;
    y++;

    if (!Tile::tiles[tileId & 0xFF]->mayPlace(w, x, y, z)) return false;

    int dir = (placementQuadrant(player) + 1) & 3;
    doorPlace(w, x, y, z, dir, tileId);
    if (player) player->inventory->consumeSelected();
    return true;
}

bool BedItem::useOn(ItemInstance* item, Player* player, World* w, int x, int y, int z, int face,
                    float, float, float) {
    if (!item || item->isNull()) return false;
    if (face != F_TOP) return false;
    y += 1;
    int dir = placementQuadrant(player);
    int xra = 0, zra = 0;
    if (dir == 0) zra = 1;
    if (dir == 1) xra = -1;
    if (dir == 2) zra = -1;
    if (dir == 3) xra = 1;

    if (worldBlock(w, x, y, z) != BLOCK_AIR) return false;
    if (worldBlock(w, x + xra, y, z + zra) != BLOCK_AIR) return false;
    if (!isSolidPhys(worldBlock(w, x, y - 1, z))) return false;
    if (!isSolidPhys(worldBlock(w, x + xra, y - 1, z + zra))) return false;

    worldSetBlockAndData(w, x, y, z, (unsigned char)tileId, (unsigned char)dir);

    if (worldBlock(w, x, y, z) == tileId)
        worldSetBlockAndData(w, x + xra, y, z + zra, (unsigned char)tileId,
                             (unsigned char)(dir + 8));
    worldNotifyNeighborsChanged(w, x, y, z);
    worldNotifyNeighborsChanged(w, x + xra, y, z + zra);
    worldUpdateLights(w);
    worldRebuildAroundNow(w, x, y, z);
    worldRebuildAroundNow(w, x + xra, y, z + zra);
    if (player) player->inventory->consumeSelected();
    return true;
}

bool FlintAndSteelItem::useOn(ItemInstance*, Player* player, World* w, int x, int y, int z, int face,
                              float, float, float) {
    if (worldBlock(w, x, y, z) == BLOCK_TNT) {
        worldPrimeTnt(w, x, y, z, 80);
        if (player) player->inventory->hurtSelected(1);
        return true;
    }
    int fx = x + kFaceNeighbor[face][0];
    int fy = y + kFaceNeighbor[face][1];
    int fz = z + kFaceNeighbor[face][2];
    if (worldBlock(w, fx, fy, fz) == BLOCK_AIR && fireMayPlace(w, fx, fy, fz)) {
        firePlace(w, fx, fy, fz);
        g_level.playSound(fx + 0.5f, fy + 0.5f, fz + 0.5f, "fire.ignite", 1.0f, 1.0f);

        if (player) player->inventory->hurtSelected(1);
        return true;
    }
    return false;
}
