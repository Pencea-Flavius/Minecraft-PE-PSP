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

    if (worldBlock(w, nx, ny, nz) == (unsigned char)tileId &&
        worldData(w, nx, ny, nz) == (unsigned char)data) return false;
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
    } else {
        face = F_TOP;
    }

    Tile* tile = Tile::tiles[tileId & 0xFF];

    int data = tile->getPlacedOnFaceDataValue(world, nx, ny, nz, face, clickX, clickY, clickZ, item->data);

    if (!tileMayPlace(world, (unsigned char)tileId, nx, ny, nz, face, data)) return false;
    if (!placeTileResolved(world, nx, ny, nz, tileId, data, player)) return false;

    if (player) player->inventory->consumeSelected();
    return true;
}

bool SlabItem::mergeAt(ItemInstance* item, Player* player, World* w, int x, int y, int z,
                       int slabType) {

    Tile* dbl = Tile::tiles[doubleId];
    if (!tileUnobstructedAt(w, (unsigned char)doubleId, x, y, z))
        return true;
    if (worldSetBlockAndData(w, x, y, z, (unsigned char)doubleId, (unsigned char)slabType)) {
        const SoundType& snd = g_tileSounds[dbl->soundType];
        if (snd.stepSound)
            g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, snd.stepSound,
                              (snd.volume + 1.0f) / 2.0f, snd.pitch * 0.8f);
        worldNotifyNeighborsChanged(w, x, y, z);
        worldUpdateLights(w);
        worldRebuildAroundNow(w, x, y, z);
        if (player) player->inventory->consumeSelected();
    }
    (void)item;
    return true;
}

bool SlabItem::useOn(ItemInstance* item, Player* player, World* w, int x, int y, int z, int face,
                     float clickX, float clickY, float clickZ) {
    if (!item || item->isNull()) return false;

    unsigned char currentTile = worldBlock(w, x, y, z);
    unsigned char currentData = worldData(w, x, y, z);
    int  slabType = currentData & DSLAB_MAT_MASK;
    bool isUpper  = (currentData & SLAB_TOP_SLOT_BIT) != 0;
    const int myType = item->data & DSLAB_MAT_MASK;

    if (((face == F_TOP && !isUpper) || (face == F_DOWN && isUpper)) &&
        currentTile == tileId && slabType == myType)
        return mergeAt(item, player, w, x, y, z, slabType);

    {
        const int tx = x + kFaceNeighbor[face][0];
        const int ty = y + kFaceNeighbor[face][1];
        const int tz = z + kFaceNeighbor[face][2];
        if (worldBlock(w, tx, ty, tz) == (unsigned char)tileId &&
            (worldData(w, tx, ty, tz) & DSLAB_MAT_MASK) == myType)
            return mergeAt(item, player, w, tx, ty, tz,
                           worldData(w, tx, ty, tz) & DSLAB_MAT_MASK);
    }
    return TileItem::useOn(item, player, w, x, y, z, face, clickX, clickY, clickZ);
}

static int placementQuadrant(Player* p) {
    float yaw = p ? p->yRot : 0.0f;
    while (yaw < 0.0f)    yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;
    return ((int)floorf(yaw * 4.0f / 360.0f + 0.5f)) & 3;
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

    const SoundType& snd = g_tileSounds[Tile::tiles[tileId & 0xFF]->soundType];
    if (snd.stepSound)
        g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, snd.stepSound,
                          (snd.volume + 1.0f) / 2.0f, snd.pitch * 0.8f);

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

    if (!tileUnobstructedAt(w, (unsigned char)tileId, x, y, z)) return false;
    if (!tileUnobstructedAt(w, (unsigned char)tileId, x, y + 1, z)) return false;

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

    if (!tileUnobstructedAt(w, (unsigned char)tileId, x, y, z)) return false;
    if (!tileUnobstructedAt(w, (unsigned char)tileId, x + xra, y, z + zra)) return false;
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

        g_level.playSound(fx + 0.5f, fy + 0.5f, fz + 0.5f, "fire.ignite", 1.0f,
                          (rand() / (float)RAND_MAX) * 0.4f + 0.8f);
    }

    if (player) player->inventory->hurtSelected(1);
    return true;
}
