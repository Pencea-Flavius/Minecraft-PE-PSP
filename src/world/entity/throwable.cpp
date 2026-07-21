#include "world/entity/throwable.h"
#include "world/entity/entity_types.h"
#include "world/entity/entity_renderer_id.h"
#include "world/entity/animal/chicken.h"
#include "world/item/item.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "client/renderer/particle.h"
#include "util/mth.h"
#include <cmath>
#include "client/gui/hud.h"

static const float RAD = 180.0f / Mth::PI;
static const float DEG = Mth::PI / 180.0f;

static inline bool solidAt(World* w, float x, float y, float z) {
    return isSolidPhys(worldBlock(w, Mth::floor(x), Mth::floor(y), Mth::floor(z)));
}

void Throwable::configure(int t) {
    type = t;
    setSize(0.25f, 0.25f);
    if (t == EntityTypes::IdThrownEgg) {
        itemId = ITEM_EGG;      entityRendererId = ER_THROWNEGG_RENDERER;
    } else {
        itemId = ITEM_SNOWBALL; entityRendererId = ER_SNOWBALL_RENDERER;
    }
}

Throwable::Throwable(Level* level, int t) : super(level), life(0) {
    configure(t);
}

Throwable::Throwable(Level* level, float px, float py, float pz,
                     float yaw, float pitch, int t) : super(level), life(0) {
    configure(t);

    float cy = cosf(yaw * DEG),   sy = sinf(yaw * DEG);
    float cp = cosf(pitch * DEG), sp = sinf(pitch * DEG);

    px -= cy * 0.16f;
    py -= 0.1f;
    pz -= sy * 0.16f;
    setPos(px, py, pz);
    xOld = x; yOld = y; zOld = z;

    shoot(cp * sy, sp, cp * cy, 1.5f);
}

void Throwable::shoot(float dx, float dy, float dz, float power) {
    float dist = Mth::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist >= 0.001f) { dx /= dist; dy /= dist; dz /= dist; }
    else { dx = dy = dz = 0.0f; }

    dx += sharedRandom.nextGaussian() * 0.0075f;
    dy += sharedRandom.nextGaussian() * 0.0075f;
    dz += sharedRandom.nextGaussian() * 0.0075f;

    xd = dx * power; yd = dy * power; zd = dz * power;
    float sd = Mth::sqrt(xd * xd + zd * zd);
    yRot = yRotO = atan2f(xd, zd) * RAD;
    xRot = xRotO = atan2f(yd, sd) * RAD;
}

void Throwable::onHit(float hx, float hy, float hz) {
    if (type == EntityTypes::IdThrownEgg && sharedRandom.nextInt(8) == 0) {
        int count = sharedRandom.nextInt(32) == 0 ? 4 : 1;
        for (int i = 0; i < count; i++) {
            Chicken* c = new Chicken(level);
            c->moveTo(hx, hy, hz, yRot, 0.0f);
            level->addEntity(c);
        }
    }
    Item* it = Item::items[itemId];
    particlesThrowPoof(hx, hy, hz, itemFlatIcon(itemId, 0));
    remove();
}

void Throwable::tick() {
    xOld = x; yOld = y; zOld = z;
    baseTick();

    float dist = Mth::sqrt(xd * xd + yd * yd + zd * zd);
    int steps = (int)(dist / 0.1f) + 1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float sx = x + xd * t, sy = y + yd * t, sz = z + zd * t;

        for (size_t ei = 0; ei < level->entities.size(); ei++) {
            Entity* e = level->entities[ei];
            if (!e || e->removed || e == this || e == (Entity*)level->player || !e->isPickable()) continue;
            if (sx > e->bb.x0 - 0.3f && sx < e->bb.x1 + 0.3f &&
                sy > e->bb.y0 - 0.3f && sy < e->bb.y1 + 0.3f &&
                sz > e->bb.z0 - 0.3f && sz < e->bb.z1 + 0.3f) {
                e->hurt(this, 0);
                onHit(sx, sy, sz);
                return;
            }
        }

        if (solidAt(level->w, sx, sy, sz)) {

            float tp = (float)(i - 1) / steps;
            onHit(x + xd * tp, y + yd * tp, z + zd * tp);
            return;
        }
    }

    x += xd; y += yd; z += zd;
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
    yd -= 0.03f;
    setPos(x, y, z);

    if (++life >= 60 * TicksPerSecond) remove();
}
