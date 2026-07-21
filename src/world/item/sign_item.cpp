
#include "world/item/sign_item.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/item/item_instance.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "world/level/tile/entity/sign_tile_entity.h"
#include "client/player/player_state.h"
#include <cmath>

extern World g_world;
extern Level g_level;

void signStartEdit(SignTileEntity* ste);

static inline int facingFromYaw16(float yawDeg) {

    return ((int)floorf((180.0f - yawDeg) * 16.0f / 360.0f + 0.5f)) & 15;
}

SignItem::SignItem(short id, int icon) : Item(id), icon(icon) {
    maxStackSize = 16;
}

bool SignItem::useOn(ItemInstance* item, Player* , World* world,
                     int x, int y, int z, int face) {
    if (face == F_DOWN) return false;

    if (!isSolidMaterial(worldBlock(world, x, y, z))) return false;

    int nx = x + kFaceNeighbor[face][0];
    int ny = y + kFaceNeighbor[face][1];
    int nz = z + kFaceNeighbor[face][2];
    if (!isReplaceable(worldBlock(world, nx, ny, nz))) return false;

    unsigned char id, data;
    if (face == F_TOP) {
        id = BLOCK_SIGN;
        data = (unsigned char)facingFromYaw16(g_level.player->yRot);
    } else {
        id = BLOCK_WALL_SIGN;

        if (face == F_BACK)         data = 2;
        else if (face == F_FORWARD) data = 3;
        else if (face == F_LEFT)    data = 4;
        else                        data = 5;
    }

    worldSetBlockAndData(world, nx, ny, nz, id, data);
    worldNotifyNeighborsChanged(world, nx, ny, nz);
    worldUpdateLights(world);
    worldRebuildAroundNow(world, nx, ny, nz);

    SignTileEntity* ste = new SignTileEntity();
    g_level.setTileEntity(nx, ny, nz, ste);

    if (item) item->count--;
    signStartEdit(ste);
    return true;
}
