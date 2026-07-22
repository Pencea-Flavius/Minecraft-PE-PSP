
#include "world/entity/arrow.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "nbt/compound_tag.h"
#include "world/inventory/inventory.h"
#include "world/item/item_instance.h"
#include "world/item/item.h"
#include "util/mth.h"
#include "client/player/player_state.h"
#include "client/renderer/particle.h"
#include <cmath>
#include <cstdlib>

static const float RAD = 180.0f / Mth::PI;
static const float DEG = Mth::PI / 180.0f;

static float arrowPitch() {
    return 1.2f / ((rand() / (float)RAND_MAX) * 0.2f + 0.9f);
}

static inline bool solidAt(World* w, float x, float y, float z) {
    return isSolidPhys(worldBlock(w, Mth::floor(x), Mth::floor(y), Mth::floor(z)));
}

Arrow::Arrow(Level* level)
    : super(level), ownerId(0), inGround(false), life(0), shakeTime(0), critArrow(false),
      playerArrow(false), xTile(-1), yTile(-1), zTile(-1), lastTile(0) {
    setSize(0.25f, 0.25f);
    entityRendererId = ER_ARROW_RENDERER;
}

Arrow::Arrow(Level* level, float px, float py, float pz,
             float yaw, float pitch, float speed, bool crit, bool fromPlayer)
    : super(level), ownerId(0), inGround(false), life(0), shakeTime(0), critArrow(crit),
      playerArrow(fromPlayer), xTile(-1), yTile(-1), zTile(-1), lastTile(0) {
    setSize(0.25f, 0.25f);
    entityRendererId = ER_ARROW_RENDERER;

    float cy = cosf(yaw * DEG),   sy = sinf(yaw * DEG);
    float cp = cosf(pitch * DEG), sp = sinf(pitch * DEG);
    float dx = cp * sy, dy = sp, dz = cp * cy;

    setPos(px + dx * 0.2f, py + dy * 0.2f, pz + dz * 0.2f);
    xOld = x; yOld = y; zOld = z;

    speed *= 1.5f;

    dx += sharedRandom.nextGaussian() * 0.0075f;
    dy += sharedRandom.nextGaussian() * 0.0075f;
    dz += sharedRandom.nextGaussian() * 0.0075f;

    xd = dx * speed; yd = dy * speed; zd = dz * speed;
    yRot = yRotO = atan2f(xd, zd) * RAD;
    xRot = xRotO = atan2f(yd, Mth::sqrt(xd * xd + zd * zd)) * RAD;
}

void Arrow::tick() {

    xOld = x; yOld = y; zOld = z;

    baseTick();

    if (shakeTime > 0) shakeTime--;

    if (inGround) {

        if (shakeTime <= 0 && playerArrow) {
            float feet = g_level.player->y - PLAYER_EYE;
            float r    = PLAYER_W * 0.5f + 1.0f;
            if (x >= g_level.player->x - r && x <= g_level.player->x + r &&
                z >= g_level.player->z - r && z <= g_level.player->z + r &&
                y >= feet     && y <= feet + PLAYER_H) {

                if (!g_level.player->inventory->isCreative()) {
                    ItemInstance* stack = new ItemInstance(ITEM_ARROW, 1, 0);
                    if (!g_level.player->inventory->add(stack)) { delete stack; return; }
                    g_level.player->inventory->ensureHotbar(ITEM_ARROW, 0);
                }
                level->playSound(this, "random.pop", 0.2f,
                                 (((rand() / (float)RAND_MAX) - (rand() / (float)RAND_MAX)) * 0.7f + 1.0f) * 2.0f);
                remove();
                return;
            }
        }

        if (worldBlock(level->w, xTile, yTile, zTile) != lastTile) {
            inGround = false;
            xd *= 0.2f; yd *= 0.2f; zd *= 0.2f;
            life = 0;
            return;
        }
        if (++life >= 60 * TicksPerSecond) remove();
        return;
    }

    float nx = x + xd, ny = y + yd, nz = z + zd;
    float dist = Mth::sqrt(xd * xd + yd * yd + zd * zd);
    int steps = (int)(dist / 0.1f) + 1;
    float px = x, py = y, pz = z;
    bool hit = false;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float sx = x + xd * t, sy = y + yd * t, sz = z + zd * t;

        for (size_t ei = 0; ei < level->entities.size(); ei++) {
            Entity* e = level->entities[ei];
            if (!e || e->removed || e == this || !e->isPickable()) continue;
            if (e->entityId == ownerId) continue;
            if (sx > e->bb.x0 - 0.3f && sx < e->bb.x1 + 0.3f &&
                sy > e->bb.y0 - 0.3f && sy < e->bb.y1 + 0.3f &&
                sz > e->bb.z0 - 0.3f && sz < e->bb.z1 + 0.3f) {

                e->hurt(this, critArrow ? 6 : 4);
                level->playSound(this, "random.bowhit", 1.0f, arrowPitch());
                remove();
                return;
            }
        }

        if (ownerId != 0 && level->player && level->player->isAlive()) {
            LocalPlayer* p = level->player;
            float pf = p->y - PLAYER_EYE;
            if (sx > p->x - PLAYER_W * 0.5f - 0.3f && sx < p->x + PLAYER_W * 0.5f + 0.3f &&
                sy > pf - 0.3f                     && sy < pf + PLAYER_H + 0.3f &&
                sz > p->z - PLAYER_W * 0.5f - 0.3f && sz < p->z + PLAYER_W * 0.5f + 0.3f) {
                p->hurt(this, 4);
                level->playSound(this, "random.bowhit", 1.0f, arrowPitch());
                remove();
                return;
            }
        }
        if (solidAt(level->w, sx, sy, sz)) {
            hit = true;
            break;
        }
        px = sx; py = sy; pz = sz;
    }

    if (hit) {

        xTile = Mth::floor(px + xd / dist * 0.1f);
        yTile = Mth::floor(py + yd / dist * 0.1f);
        zTile = Mth::floor(pz + zd / dist * 0.1f);
        lastTile = worldBlock(level->w, xTile, yTile, zTile);
        x = px; y = py; z = pz;
        xd = yd = zd = 0.0f;
        inGround = true;
        shakeTime = 7;
        critArrow = false;
        level->playSound(this, "random.bowhit", 1.0f, arrowPitch());
        setPos(x, y, z);
        return;
    }

    if (critArrow) {
        for (int i = 0; i < 4; i++)
            particlesCrit(x + xd * i / 4.0f, y + yd * i / 4.0f, z + zd * i / 4.0f,
                          -xd, -yd + 0.2f, -zd);
    }

    x = nx; y = ny; z = nz;
    float sd = Mth::sqrt(xd * xd + zd * zd);
    yRot = atan2f(xd, zd) * RAD;
    xRot = atan2f(yd, sd) * RAD;

    float inertia = 0.99f;
    if (isInWater()) {
        inertia = 0.80f;
        for (int i = 0; i < 4; i++) {
            float s = 0.25f;
            particlesBubble(x - xd * s, y - yd * s, z - zd * s, xd, yd, zd);
        }
    }
    xd *= inertia; yd *= inertia; zd *= inertia;
    yd -= 0.05f;

    setPos(x, y, z);
}

int Arrow::getEntityTypeId() const { return EntityTypes::IdArrow; }

void Arrow::addAdditonalSaveData(CompoundTag* tag) {
    tag->putShort("xTile", (short)xTile);
    tag->putShort("yTile", (short)yTile);
    tag->putShort("zTile", (short)zTile);
    tag->putShort("inTile", (short)lastTile);
    tag->putBoolean("player", playerArrow);
    tag->putBoolean("inGround", inGround);
    tag->putShort("life", (short)life);
}

void Arrow::readAdditionalSaveData(CompoundTag* tag) {
    xTile = tag->getShort("xTile");
    yTile = tag->getShort("yTile");
    zTile = tag->getShort("zTile");
    lastTile = tag->getShort("inTile");
    playerArrow = tag->getBoolean("player");
    inGround = tag->getBoolean("inGround");
    life = tag->getShort("life");
}
