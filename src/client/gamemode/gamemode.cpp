#include "client/gamemode/gamemode.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "client/player/player.h"
#include "client/gui/hud.h"
#include "world/entity/tripod_camera.h"
#include "client/player/player_state.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/entity/entity.h"
#include "world/entity/arrow.h"
#include "world/entity/throwable.h"
#include "world/entity/animal/pig.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"
#include "world/item/tile_item.h"
#include "world/item/food_item.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/entity/sign_tile_entity.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/item/crafting/recipe.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/level/tile/entity/reactor_tile_entity.h"
#include "world/entity/item_entity.h"

static Mob* nearbyTripodCamera() {
    if (!g_level.player) return 0;
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed || e->entityRendererId != ER_TRIPODCAMERA_RENDERER) continue;
        float dx = e->x - g_level.player->x;
        float dy = e->y - g_level.player->y;
        float dz = e->z - g_level.player->z;
        if (dx * dx + dy * dy + dz * dz < 2.5f * 2.5f) return (Mob*)e;
    }
    return 0;
}

static bool s_drawing = false;

static bool canDrawBow() {
    if (g_level.player->inventory->isCreative()) return true;
    for (int i = 0; i < g_level.player->inventory->getContainerSize(); i++) {
        ItemInstance* it = g_level.player->inventory->getItem(i);
        if (it && it->id == ITEM_ARROW && it->count > 0) return true;
    }
    return false;
}

static bool tileNeedsTool(unsigned char id, unsigned char data) {
    switch (id) {
        case BLOCK_STONE: case BLOCK_COBBLESTONE: case BLOCK_MOSSY_COBBLE:
        case BLOCK_STONE_BRICKS: case BLOCK_BRICKS: case BLOCK_SANDSTONE:
        case BLOCK_QUARTZ_BLOCK: case BLOCK_OBSIDIAN:

        case BLOCK_NETHERRACK: case BLOCK_NETHER_BRICK:
        case BLOCK_ORE_COAL: case BLOCK_ORE_IRON: case BLOCK_ORE_GOLD:
        case BLOCK_ORE_EMERALD: case BLOCK_ORE_LAPIS:
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT:
        case BLOCK_IRON_BLOCK: case BLOCK_GOLD_BLOCK: case BLOCK_DIAMOND_BLOCK:
        case BLOCK_LAPIS_BLOCK:
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT: case BLOCK_STONECUTTER:
        case BLOCK_TOPSNOW: case BLOCK_SNOW_BLOCK:
        case BLOCK_STAIRS_COBBLESTONE: case BLOCK_STAIRS_BRICK:
        case BLOCK_STAIRS_STONE_BRICK: case BLOCK_STAIRS_SANDSTONE:
        case BLOCK_STAIRS_NETHER_BRICK:
            return true;
        case BLOCK_DOUBLE_SLAB: case BLOCK_SLAB:
            return (data & DSLAB_MAT_MASK) != DSLAB_WOOD;
        default:
            return false;
    }
}

MiningState g_mining = { false, 0, 0, 0, 0.0f };

static float tileDestroyTime(int id) {
    switch (id) {
        case BLOCK_STONE: case BLOCK_STONE_BRICKS: case BLOCK_BOOKSHELF:
        case BLOCK_STAIRS_STONE_BRICK:
            return 1.5f;
        case BLOCK_DIRT: case BLOCK_SAND:
            return 0.5f;
        case BLOCK_GRASS: case BLOCK_GRAVEL: case BLOCK_CLAY: case BLOCK_FARMLAND:
            return 0.6f;
        case BLOCK_PLANKS: case BLOCK_LOG: case BLOCK_FENCE: case BLOCK_FENCE_GATE:
        case BLOCK_DOUBLE_SLAB: case BLOCK_SLAB: case BLOCK_COBBLESTONE:
        case BLOCK_BRICKS: case BLOCK_MOSSY_COBBLE: case BLOCK_NETHER_BRICK:
        case BLOCK_STAIRS_PLANKS: case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_STAIRS_BRICK: case BLOCK_STAIRS_NETHER_BRICK:
            return 2.0f;
        case BLOCK_LEAVES:
            return 0.2f;
        case BLOCK_GLASS: case BLOCK_GLASS_PANE: case BLOCK_GLOWSTONE:
            return 0.3f;
        case BLOCK_ORE_GOLD: case BLOCK_ORE_IRON: case BLOCK_ORE_COAL:
        case BLOCK_ORE_LAPIS: case BLOCK_ORE_EMERALD:
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT:
        case BLOCK_LAPIS_BLOCK: case BLOCK_GOLD_BLOCK:
        case BLOCK_DOOR_WOOD: case BLOCK_TRAPDOOR: case BLOCK_NETHER_REACTOR:
            return 3.0f;
        case BLOCK_IRON_BLOCK: case BLOCK_DIAMOND_BLOCK: case BLOCK_DOOR_IRON:
            return 5.0f;
        case BLOCK_OBSIDIAN: case BLOCK_GLOWING_OBSIDIAN:
            return 10.0f;
        case BLOCK_COBWEB:
            return 4.0f;
        case BLOCK_WOOL: case BLOCK_SANDSTONE: case BLOCK_QUARTZ_BLOCK:
        case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_QUARTZ:
            return 0.8f;
        case BLOCK_ICE:
            return 0.5f;
        case BLOCK_TOPSNOW:
            return 0.1f;
        case BLOCK_SNOW_BLOCK: case BLOCK_BED:
            return 0.2f;
        case BLOCK_CACTUS: case BLOCK_LADDER: case BLOCK_NETHERRACK:
            return 0.4f;
        case BLOCK_CHEST: case BLOCK_CRAFTING_TABLE: case BLOCK_STONECUTTER:
            return 2.5f;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT:
            return 3.5f;
        case BLOCK_SIGN: case BLOCK_WALL_SIGN: case BLOCK_MELON:
            return 1.0f;
        case BLOCK_BEDROCK: case BLOCK_INVISIBLE_BEDROCK:
            return -1.0f;

        case BLOCK_SAPLING: case BLOCK_TALLGRASS: case BLOCK_FLOWER: case BLOCK_ROSE:
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED: case BLOCK_TNT:
        case BLOCK_TORCH: case BLOCK_WHEAT: case BLOCK_REEDS: case BLOCK_MELON_STEM:
            return 0.0f;
        default:
            return 1.0f;
    }
}

void playerDropSelected(bool all) {
    if (g_level.player->inventory->isCreative() || !g_level.player) return;
    ItemInstance* held = g_level.player->inventory->getSelected();
    if (!held || held->isNull()) return;

    ItemInstance piece = g_level.player->inventory->removeSelected(all ? held->count : 1);
    if (piece.isNull()) return;

    LocalPlayer* p = g_level.player;
    const float D2R = 3.14159265f / 180.0f;
    float cy = cosf(p->yRot * D2R), sy = sinf(p->yRot * D2R);
    float cp = cosf(p->xRot * D2R), sp = sinf(p->xRot * D2R);

    const float pow = 0.3f;

    ItemEntity* e = new ItemEntity(&g_level, p->x, p->y - 0.3f, p->z, piece);
    e->xd = cp * sy * pow;
    e->yd = sp * pow + 0.1f;
    e->zd = cp * cy * pow;

    float dir = (rand() / (float)RAND_MAX) * 6.2831853f;
    float j = 0.02f * (rand() / (float)RAND_MAX);
    e->xd += cosf(dir) * j;
    e->zd += sinf(dir) * j;
    e->yd += ((rand() / (float)RAND_MAX) - (rand() / (float)RAND_MAX)) * 0.1f;
    e->throwTime = 20 * 2;
    g_level.addEntity(e);
}

static void spillContainerItem(int x, int y, int z, const ItemInstance& it) {
    const float s = 0.7f;
    float xo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    float yo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    float zo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    ItemEntity* e = new ItemEntity(&g_level, x + xo, y + yo, z + zo, it);
    e->throwTime = 10;
    g_level.addEntity(e);
}
#include "client/renderer/item_hand.h"
#include "client/renderer/particle.h"
#include <cmath>
#include <pspkernel.h>
#include <cstdio>
#include <cstdlib>
#include <pspctrl.h>

extern World g_world;
extern Level g_level;
extern bool  g_worldBuilt;

void signStartEdit(SignTileEntity* ste);

static void playTileSoundAt(const char* name, const SoundType& s, int x, int y, int z) {
    if (!name) return;
    g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, name,
                      (s.volume + 1.0f) / 2.0f, s.pitch * 0.8f);
}

static void playTileBreakSound(int tileId, int x, int y, int z) {
    const SoundType& s = g_tileSounds[Tile::tiles[tileId & 0xFF]->soundType];
    playTileSoundAt(s.breakSound, s, x, y, z);
}

static bool rayHitsAABB(float px, float py, float pz, float dx, float dy, float dz,
                        const AABB& box, float range, float& t) {
    float tmin = 0.0f, tmax = range;
    const float eps = 1e-6f;
    float lo, hi;

    if (dx > -eps && dx < eps) {
        if (px < box.x0 || px > box.x1) return false;
    } else {
        lo = (box.x0 - px)/dx; hi = (box.x1 - px)/dx;
        if (lo > hi) { float s = lo; lo = hi; hi = s; }
        if (lo > tmin) tmin = lo;
        if (hi < tmax) tmax = hi;
        if (tmin > tmax) return false;
    }

    if (dy > -eps && dy < eps) {
        if (py < box.y0 || py > box.y1) return false;
    } else {
        lo = (box.y0 - py)/dy; hi = (box.y1 - py)/dy;
        if (lo > hi) { float s = lo; lo = hi; hi = s; }
        if (lo > tmin) tmin = lo;
        if (hi < tmax) tmax = hi;
        if (tmin > tmax) return false;
    }

    if (dz > -eps && dz < eps) {
        if (pz < box.z0 || pz > box.z1) return false;
    } else {
        lo = (box.z0 - pz)/dz; hi = (box.z1 - pz)/dz;
        if (lo > hi) { float s = lo; lo = hi; hi = s; }
        if (lo > tmin) tmin = lo;
        if (hi < tmax) tmax = hi;
        if (tmin > tmax) return false;
    }
    t = tmin;
    return true;
}

static bool breakHangingEntityUnderCrosshair() {
    const float DEG2RAD = 3.14159265f / 180.0f;
    float cy = cosf(g_level.player->yRot * DEG2RAD), sy = sinf(g_level.player->yRot * DEG2RAD);
    float cp = cosf(g_level.player->xRot * DEG2RAD), sp = sinf(g_level.player->xRot * DEG2RAD);
    float dx = cp * sy, dy = sp, dz = cp * cy;
    const float range = 5.0f;

    float blockT = range;
    BlockHit bh = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z, g_level.player->yRot, g_level.player->xRot, range);
    if (bh.hit) {
        float hx = bh.x + bh.clickX, hy = bh.y + bh.clickY, hz = bh.z + bh.clickZ;
        float ddx = hx - g_level.player->x, ddy = hy - g_level.player->y, ddz = hz - g_level.player->z;
        blockT = sqrtf(ddx*ddx + ddy*ddy + ddz*ddz);
    }

    Entity* best = 0;
    float bestT = blockT;
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed || !e->isPickable()) continue;
        float t;
        if (rayHitsAABB(g_level.player->x, g_level.player->y, g_level.player->z, dx, dy, dz, e->bb, range, t) && t < bestT) {
            bestT = t; best = e;
        }
    }
    if (best) {

        int dmg = 1;
        ItemInstance* held = g_level.player->inventory->getSelected();
        if (held && !held->isNull() && Item::items[held->id])
            dmg = Item::items[held->id]->getAttackDamage();
        best->hurt(g_level.player, dmg);
        playerSwing();

        if (!g_gameMode->isCreative() && held && !held->isNull() &&
            Item::items[held->id] && Item::items[held->id]->maxDamage > 0)
            if (g_level.player->inventory->hurtSelected(Item::items[held->id]->getHurtEnemyDurabilityCost()))
                g_level.playSound(g_level.player, "random.break", 1.0f, 1.0f);
        return true;
    }
    return false;
}

static bool interactMobUnderCrosshair() {
    const float DEG2RAD = 3.14159265f / 180.0f;
    float cy = cosf(g_level.player->yRot * DEG2RAD), sy = sinf(g_level.player->yRot * DEG2RAD);
    float cp = cosf(g_level.player->xRot * DEG2RAD), sp = sinf(g_level.player->xRot * DEG2RAD);
    float dx = cp * sy, dy = sp, dz = cp * cy;
    const float range = 3.0f;
    Entity* best = 0; float bestT = range;
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed || !e->isMob()) continue;
        float t;
        if (rayHitsAABB(g_level.player->x, g_level.player->y, g_level.player->z, dx, dy, dz, e->bb, range, t) && t < bestT) {
            bestT = t; best = e;
        }
    }
    return best ? ((Mob*)best)->playerInteract() : false;
}

static void spawnEatParticles(int iconCell, int count) {
    particlesEat(g_level.player->x, g_level.player->y, g_level.player->z,
                 g_level.player->yRot, g_level.player->xRot, iconCell, count);

    float r1 = rand() / (float)RAND_MAX, r2 = rand() / (float)RAND_MAX;
    g_level.playSound(g_level.player, "random.eat",
                      0.5f + 0.5f * (rand() % 2), (r1 - r2) * 0.2f + 1.0f);
}

static void breakTargetedBlock(const BlockHit& hit) {

    unsigned char brokenId = worldBlock(&g_world, hit.x, hit.y, hit.z);
    unsigned char brokenData = worldData(&g_world, hit.x, hit.y, hit.z);

    if (brokenId == BLOCK_BEDROCK ||
        hit.x < 0 || hit.x >= WORLD_W ||
        hit.z < 0 || hit.z >= WORLD_D ||
        hit.y < 0 || hit.y >= WORLD_H) {
        return;
    }

    if (brokenId == BLOCK_CHEST || brokenId == BLOCK_FURNACE || brokenId == BLOCK_FURNACE_LIT) {
        TileEntity* te = g_level.getTileEntity(hit.x, hit.y, hit.z);
        if (te && te->type == TE_CHEST) {
            ChestTileEntity* c = (ChestTileEntity*)te;
            for (int i = 0; i < ChestTileEntity::CONTAINER_SIZE; i++)
                if (ItemInstance* it = c->container.getItem(i))
                    if (!it->isNull()) spillContainerItem(hit.x, hit.y, hit.z, *it);
        } else if (te && te->type == TE_FURNACE) {
            FurnaceTileEntity* fu = (FurnaceTileEntity*)te;
            for (int i = 0; i < FurnaceTileEntity::NUM_ITEMS; i++)
                if (!fu->items[i].isNull()) spillContainerItem(hit.x, hit.y, hit.z, fu->items[i]);
        }
    }
    if (isSign(brokenId) || brokenId == BLOCK_CHEST ||
        brokenId == BLOCK_FURNACE || brokenId == BLOCK_FURNACE_LIT)
        g_level.removeTileEntity(hit.x, hit.y, hit.z);
    {

        ItemInstance* sel = g_level.player->inventory->getSelected();
        bool shearedLeaf = (brokenId == BLOCK_LEAVES && sel && sel->id == ITEM_SHEARS);
        if (shearedLeaf) {
            ItemEntity* le = new ItemEntity(&g_level, hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f,
                                            ItemInstance(BLOCK_LEAVES, 1, (short)(brokenData & 3)));
            le->throwTime = 10;
            g_level.addEntity(le);
        }
        bool couldDestroy = true;
        if (!g_gameMode->isCreative() && tileNeedsTool(brokenId, brokenData)) {
            Item* it = (sel && sel->id > 0 && sel->id < 4096) ? Item::items[sel->id] : nullptr;
            couldDestroy = it && it->canDestroySpecial(brokenId);
        }
        if (couldDestroy && !shearedLeaf)
            worldSpawnResources(&g_world, hit.x, hit.y, hit.z, brokenId, brokenData);

        if (couldDestroy && brokenId == BLOCK_TOPSNOW && !g_gameMode->isCreative()) {
            ItemEntity* se = new ItemEntity(&g_level,
                hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f,
                ItemInstance(ITEM_SNOWBALL, 1, 0));
            se->throwTime = 10;
            g_level.addEntity(se);
        }

        if (!g_gameMode->isCreative() && sel && !sel->isNull()) {
            Item* tool = Item::items[sel->id];
            if (tool && tool->maxDamage > 0 &&
                g_level.player->inventory->hurtSelected(tool->getMineDurabilityCost()))
                g_level.playSound(g_level.player, "random.break", 1.0f, 1.0f);
        }

        particlesDestroyBlock(&g_world, hit.x, hit.y, hit.z, brokenId, brokenData);
        playTileBreakSound(brokenId, hit.x, hit.y, hit.z);
        worldSetBlockAndData(&g_world, hit.x, hit.y, hit.z, BLOCK_AIR, 0);
        worldNotifyNeighborsChanged(&g_world, hit.x, hit.y, hit.z);

        unsigned int editT0 = sceKernelGetSystemTimeLow();
        unsigned int lightT0 = editT0;

        worldUpdateLights(&g_world);
        unsigned int rebuildT0 = sceKernelGetSystemTimeLow();
        worldRecordLightUs(rebuildT0 - lightT0);
        worldRebuildAroundNow(&g_world, hit.x, hit.y, hit.z);
        unsigned int editT1 = sceKernelGetSystemTimeLow();
        worldRecordRebuildUs(editT1 - rebuildT0);
        worldRecordEditUs(editT1 - editT0);
    }
}

static bool continueMining(const BlockHit& hit) {
    static unsigned int s_lastUs = 0;
    static float s_digTicks = 0.0f;

    static unsigned int s_breakCooldownUs = 0;
    unsigned int now = sceKernelGetSystemTimeLow();

    unsigned char id   = worldBlock(&g_world, hit.x, hit.y, hit.z);
    unsigned char data = worldData(&g_world, hit.x, hit.y, hit.z);

    if (!g_mining.active || hit.x != g_mining.x || hit.y != g_mining.y || hit.z != g_mining.z) {
        g_mining.active = true;
        g_mining.x = hit.x; g_mining.y = hit.y; g_mining.z = hit.z;
        g_mining.progress = 0.0f;
        s_lastUs = now; s_digTicks = 0.0f;
        playerSwing();
    }

    float dt = tileDestroyTime(id);
    if (id == BLOCK_AIR || dt < 0.0f) { g_mining.progress = 0.0f; return false; }
    if (dt == 0.0f) return true;

    playerSwing();

    ItemInstance* sel = g_level.player->inventory->getSelected();
    Item* it = (sel && sel->id > 0 && sel->id < 4096) ? Item::items[sel->id] : nullptr;
    bool canDestroy = !tileNeedsTool(id, data) || (it && it->canDestroySpecial(id));
    float speed = canDestroy ? (it ? it->getDestroySpeed(id) : 1.0f) : 1.0f;

    LocalPlayer* p = g_level.player;
    if (p) {
        unsigned char eyeBlk = worldBlock(&g_world, Mth::floor(p->x), Mth::floor(p->y), Mth::floor(p->z));
        if (isWaterId(eyeBlk)) speed /= 5.0f;
        if (!p->onGround)      speed /= 5.0f;
    }
    float perTick = (speed / dt) / (canDestroy ? 30.0f : 100.0f);

    float ticks = (now - s_lastUs) / 50000.0f;
    s_lastUs = now;

    if (now < s_breakCooldownUs) return false;
    g_mining.progress += perTick * ticks;

    s_digTicks += ticks;
    if (s_digTicks >= 4.0f) {
        s_digTicks -= 4.0f;
        particlesCrackHit(&g_world, hit.x, hit.y, hit.z, id, data, hit.face);
        const SoundType& s = g_tileSounds[Tile::tiles[id]->soundType];
        if (s.stepSound)
            g_level.playSound(hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f, s.stepSound,
                              (s.volume + 1.0f) / 8.0f, s.pitch * 0.5f);
    }

    bool done = g_mining.progress >= 1.0f;
    if (done) s_breakCooldownUs = now + 250000;
    return done;
}

bool GameMode::useItemOn(ItemInstance* item, const BlockHit& hit) {
    unsigned char t = worldBlock(&g_world, hit.x, hit.y, hit.z);

    if (t == BLOCK_INVISIBLE_BEDROCK) return true;

    if (t > 0 && Tile::tiles[t]->use(&g_world, hit.x, hit.y, hit.z, g_level.player)) return true;
    if (!item || item->isNull()) return false;
    Item* used = item->getItem();
    if (!used) return false;

    if (isCreative()) {

        short aux = item->data, count = item->count;
        bool ok = used->useOn(item, g_level.player, &g_world, hit.x, hit.y, hit.z, hit.face,
                              hit.clickX, hit.clickY, hit.clickZ);
        item->data = aux; item->count = count;
        return ok;
    }
    return used->useOn(item, g_level.player, &g_world, hit.x, hit.y, hit.z, hit.face,
                       hit.clickX, hit.clickY, hit.clickZ);
}

void GameMode::handleInput(unsigned int pressed, unsigned int held) {

    extern int g_showFps;
    {
        static bool s_waterChord = false, s_lavaChord = false;
        bool down  = (held & PSP_CTRL_DOWN) != 0;
        bool water = down && (held & PSP_CTRL_LTRIGGER);
        bool lava  = down && (held & PSP_CTRL_RTRIGGER);
        if (g_worldBuilt && g_showFps && g_level.player->inventory->isCreative() && (water || lava) &&
            !(water ? s_waterChord : s_lavaChord)) {
            if (water) s_waterChord = true; else s_lavaChord = true;
            unsigned char id = water ? (unsigned char)BLOCK_WATER : (unsigned char)BLOCK_LAVA;
            BlockHit hit = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z, g_level.player->yRot, g_level.player->xRot, 5.0f);
            if (hit.hit) {
                int nx = hit.x + kFaceNeighbor[hit.face][0];
                int ny = hit.y + kFaceNeighbor[hit.face][1];
                int nz = hit.z + kFaceNeighbor[hit.face][2];
                worldSetBlockAndData(&g_world, nx, ny, nz, id, 0);
                worldScheduleTick(&g_world, nx, ny, nz, id, 30);
                worldNotifyNeighborsChanged(&g_world, nx, ny, nz);
                worldUpdateLights(&g_world);
                worldRebuildAroundNow(&g_world, nx, ny, nz);
            }
            return;
        }
        if (!water) s_waterChord = false;
        if (!lava)  s_lavaChord  = false;
    }

    if (g_worldBuilt) {
        static unsigned int s_startUs = 0;
        ItemInstance* sel = g_level.player->inventory->getSelected();
        if (sel && sel->id == ITEM_BOW) {
            bool lHeld = (held & PSP_CTRL_LTRIGGER) != 0;

            bool hasArrow = canDrawBow();
            if (lHeld && !s_drawing && (pressed & PSP_CTRL_LTRIGGER) && hasArrow) {
                s_drawing = true; s_startUs = sceKernelGetSystemTimeLow();
            }
            if (lHeld && s_drawing) {

                float ticks = (sceKernelGetSystemTimeLow() - s_startUs) / 50000.0f;
                g_level.player->bowTimeHeld = ticks;
                float p = ticks / 20.0f;
                p = ((p * p) + p * 2) / 3.0f;
                if (p > 1) p = 1;
                g_level.player->bowPull = p;
                pressed &= ~PSP_CTRL_RTRIGGER;
            } else {
                if (s_drawing) {
                    s_drawing = false;
                    float pow = g_level.player->bowPull;

                    if (pow >= 0.1f &&
                        g_level.player->inventory->removeResource(ItemInstance(ITEM_ARROW, 1, 0), true) == 0) {
                        g_level.addEntity(new Arrow(&g_level, g_level.player->x, g_level.player->y, g_level.player->z,
                                                    g_level.player->yRot, g_level.player->xRot, pow * 2.0f, pow >= 1.0f,
                                                     true));

                        g_level.playSound(g_level.player, "random.bow", 1.0f,
                                          1.0f / ((rand() / (float)RAND_MAX) * 0.4f + 1.2f) + pow * 0.5f);

                        if (g_level.player->inventory->hurtSelected(1))
                            g_level.playSound(g_level.player, "random.break", 1.0f, 1.0f);
                    }
                }
                g_level.player->bowPull = 0.0f;
                g_level.player->bowTimeHeld = 0.0f;
            }
            pressed &= ~PSP_CTRL_LTRIGGER;
        } else {

            s_drawing = false;
            g_level.player->bowPull = 0.0f;
            g_level.player->bowTimeHeld = 0.0f;
        }
    }

    if (g_worldBuilt) {
        ItemInstance* sel = g_level.player->inventory->getSelected();
        if (sel && (sel->id == ITEM_SNOWBALL || sel->id == ITEM_EGG)) {
            if (pressed & PSP_CTRL_LTRIGGER) {
                int type = (sel->id == ITEM_EGG) ? EntityTypes::IdThrownEgg
                                                 : EntityTypes::IdSnowball;
                g_level.addEntity(new Throwable(&g_level,
                    g_level.player->x, g_level.player->y, g_level.player->z,
                    g_level.player->yRot, g_level.player->xRot, type));
                playerSwing();

                g_level.playSound(g_level.player, "random.bow", 0.5f,
                                  0.4f / ((rand() / (float)RAND_MAX) * 0.4f + 0.8f));

                if (!isCreative()) g_level.player->inventory->consumeSelected();
            }
            pressed &= ~PSP_CTRL_LTRIGGER;
        }

        else if (sel && sel->id == ITEM_CAMERA) {
            if (pressed & PSP_CTRL_LTRIGGER) {

                bool handled = interactMobUnderCrosshair();
                if (!handled) {
                    Mob* near = nearbyTripodCamera();
                    if (near) handled = near->playerInteract();
                }
                if (!handled) {
                    g_level.addEntity(new TripodCamera(&g_level,
                        g_level.player->x, g_level.player->y, g_level.player->z,
                        g_level.player->yRot, g_level.player->xRot));
                }
                playerSwing();
            }
            pressed &= ~PSP_CTRL_LTRIGGER;
        }
    }

    if (g_worldBuilt) {
        static bool s_eating = false;
        static unsigned int s_eatStart = 0;
        static int s_lastEmit = 0;
        ItemInstance* sel = g_level.player->inventory->getSelected();
        bool isFoodSel = sel && sel->getItem() && sel->getItem()->isFood();
        if (isFoodSel) {
            bool lHeld = (held & PSP_CTRL_LTRIGGER) != 0;

            bool canEat = g_gameMode->isCreative() ||
                          g_level.player->health < g_level.player->getMaxHealth();
            if (lHeld && !s_eating && (pressed & PSP_CTRL_LTRIGGER) && canEat) {
                s_eating = true; s_eatStart = sceKernelGetSystemTimeLow(); s_lastEmit = 0;
            }
            if (lHeld && s_eating) {
                float ticks = (sceKernelGetSystemTimeLow() - s_eatStart) / 50000.0f;
                float progress = ticks / (float)FoodItem::EAT_TICKS;
                if (progress > 1.0f) progress = 1.0f;
                g_level.player->eatAnim = progress;
                int icon = itemFlatIcon(sel->id, (unsigned char)sel->data);

                int t4 = (int)ticks / 4;
                if ((int)ticks >= 4 && t4 != s_lastEmit) {
                    s_lastEmit = t4;
                    spawnEatParticles(icon, 5);
                }
                if (ticks >= (float)FoodItem::EAT_TICKS) {
                    spawnEatParticles(icon, 10);

                    if (!g_gameMode->isCreative()) {
                        int nutrition = ((FoodItem*)sel->getItem())->getNutrition();
                        g_level.player->heal(nutrition);
                        g_level.playSound(g_level.player, "random.burp", 0.5f,
                                          (rand() / (float)RAND_MAX) * 0.1f + 0.9f);

                        short remainder = ((FoodItem*)sel->getItem())->getFoodRemainder();
                        g_level.player->inventory->consumeSelected();
                        if (remainder) g_level.player->inventory->setSelectedIfEmpty(remainder, 0);

                        ItemInstance* next = g_level.player->inventory->getSelected();
                        bool moreFood = next && next->getItem() && next->getItem()->isFood();
                        bool stillHurt = g_level.player->health < g_level.player->getMaxHealth();
                        if (lHeld && moreFood && stillHurt) {
                            s_eatStart = sceKernelGetSystemTimeLow(); s_lastEmit = 0;
                            g_level.player->eatAnim = 0.0f;
                        } else {
                            s_eating = false;
                            g_level.player->eatAnim = 0.0f;
                        }
                    } else {
                        s_eatStart = sceKernelGetSystemTimeLow(); s_lastEmit = 0;
                        g_level.player->eatAnim = 0.0f;
                    }
                }
                pressed &= ~PSP_CTRL_LTRIGGER;
            } else {
                s_eating = false;
                g_level.player->eatAnim = 0.0f;
            }
        } else {
            s_eating = false;
            g_level.player->eatAnim = 0.0f;
        }
    }

    if (g_worldBuilt && g_gameMode && !g_gameMode->isCreative()) {
        ItemInstance* mineSel = g_level.player->inventory->getSelected();
        bool bow = mineSel && mineSel->id == ITEM_BOW;
        if ((held & PSP_CTRL_RTRIGGER) && !bow) {
            BlockHit hit = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z,
                                     g_level.player->yRot, g_level.player->xRot, 5.0f);
            if (hit.hit && continueMining(hit)) {
                breakTargetedBlock(hit);
                g_mining.active = false;
                g_mining.progress = 0.0f;
            } else if (!hit.hit) {
                g_mining.active = false;
                g_mining.progress = 0.0f;
            }
        } else {
            g_mining.active = false;
            g_mining.progress = 0.0f;
        }
    }

    if (g_worldBuilt && (pressed & (PSP_CTRL_RTRIGGER | PSP_CTRL_LTRIGGER))) {

        if ((pressed & PSP_CTRL_RTRIGGER) && breakHangingEntityUnderCrosshair()) {
            return;
        }

        if ((pressed & PSP_CTRL_LTRIGGER) && interactMobUnderCrosshair()) {
            playerSwing();
            return;
        }
        if (pressed & PSP_CTRL_RTRIGGER) {
            playerSwing();
        }
        BlockHit hit = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z, g_level.player->yRot, g_level.player->xRot, 5.0f);
        if (hit.hit) {
            if (pressed & PSP_CTRL_RTRIGGER) {

                if (g_gameMode->isCreative()) breakTargetedBlock(hit);
            } else if (g_gameMode->useItemOn(g_level.player->inventory->getSelected(), hit)) {

                playerSwing();
                return;
            }
        }
    }
}

GameMode* g_gameMode = 0;

void gameModeInit(int gameType) {
    delete g_gameMode;
    bool creative = (gameType == 1);
    g_gameMode = creative ? (GameMode*)new CreativeMode()
                          : (GameMode*)new SurvivalMode();

    g_level.player->inventory->reinit(creative);
}
void gameModeShutdown() { delete g_gameMode; g_gameMode = 0; }

void gameModeHandleInput(unsigned int pressed, unsigned int held) {
    if (g_gameMode) g_gameMode->handleInput(pressed, held);
}

static bool isUsableBlockId(unsigned char id) {
    if (isDoor(id) || isTrapdoor(id) || isFenceGate(id) || isSign(id)) return true;
    switch (id) {
        case BLOCK_CRAFTING_TABLE: case BLOCK_STONECUTTER:
        case BLOCK_FURNACE:        case BLOCK_FURNACE_LIT:
        case BLOCK_CHEST:
            return true;
        default:
            return false;
    }
}

CrosshairTarget gameModeCrosshairTarget() {
    CrosshairTarget t = { 0, 0 };
    if (!g_worldBuilt || !g_level.player) return t;

    ItemInstance* sel = g_level.player->inventory->getSelected();
    if (sel && sel->id == ITEM_BOW) {
        if (s_drawing)    { t.useLabel = "Release"; return t; }
        if (canDrawBow())   t.useLabel = "Draw";
    }

    if (sel && sel->getItem() && sel->getItem()->isFood() && g_gameMode &&
        (g_gameMode->isCreative() ||
         g_level.player->health < g_level.player->getMaxHealth())) {
        t.useLabel = "Eat";
        return t;
    }

    if (sel && sel->id == ITEM_CAMERA) {
        t.useLabel = nearbyTripodCamera() ? "Take Picture" : "Place";
        return t;
    }

    BlockHit hit = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z,
                             g_level.player->yRot, g_level.player->xRot, 5.0f);
    if (!hit.hit) return t;
    unsigned char id = worldBlock(&g_world, hit.x, hit.y, hit.z);

    if (id == BLOCK_INVISIBLE_BEDROCK) return t;

    t.breakLabel = (id == BLOCK_LEAVES && sel && sel->id == ITEM_SHEARS) ? "Shear" : "Mine";

    if (!t.useLabel) {

        if (sel && sel->id == ITEM_BONEMEAL && sel->data == DYE_WHITE &&
            (id == BLOCK_SAPLING || id == BLOCK_WHEAT || id == BLOCK_MELON_STEM ||
             id == BLOCK_GRASS   || id == BLOCK_REEDS))                t.useLabel = "Grow";
        else if (id == BLOCK_TNT && sel && sel->id == ITEM_FLINT_AND_STEEL) t.useLabel = "Ignite";
        else if (id == BLOCK_BED)                                      t.useLabel = "Sleep";
        else if (id == BLOCK_NETHER_REACTOR)                           t.useLabel = "Activate";
        else if (isUsableBlockId(id))                                  t.useLabel = "Use";

        else if (sel && sel->id == ITEM_PAINTING)                      t.useLabel = "Hang";
        else if (sel && (sel->id < 256 || sel->id == ITEM_SIGN ||
                         sel->id == ITEM_SPAWN_EGG))                   t.useLabel = "Place";
    }
    return t;
}
