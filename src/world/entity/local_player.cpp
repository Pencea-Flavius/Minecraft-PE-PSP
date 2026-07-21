
#include "world/entity/local_player.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/difficulty.h"
#include "client/player/player_state.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/particle.h"
#include "world/level/tile/redstone_ore.h"
#include <cmath>
#include <cstdlib>
#include <pspctrl.h>

extern World g_world;

int   g_autoJump = 1;
float g_sensitivity = 1.0f;

LocalPlayer::LocalPlayer(Level* level) : Player(level) {
    setSize(PLAYER_W, PLAYER_H);
    heightOffset = PLAYER_EYE;
    entityRendererId = ER_DEFAULT_RENDERER;
}

void LocalPlayer::aiStep(unsigned int btn, unsigned char lx, unsigned char ly) {
    const float LOOK = 7.5f * g_sensitivity;

    sleepTick();
    if (sleeping) {
        xo = x; yo = y; zo = z;
        yRotO = yRot; xRotO = xRot;
        walkDistO = walkDist;
        oBob = bob; oTilt = tilt;
        yBodyRotO = yBodyRot; walkAnimSpeedO = walkAnimSpeed; walkAnimPosO = walkAnimPos;
        xd = yd = zd = 0.0f;
        return;
    }

    xo = x; yo = y; zo = z;
    yRotO = yRot; xRotO = xRot;

    if (attackTime > 0)       attackTime--;
    if (hurtTime > 0)         hurtTime--;
    if (invulnerableTime > 0) invulnerableTime--;

    if (health <= 0 && deathTime < 20) {
        deathTime++;
        if (deathTime == 20)
            particlesMobDeath(x, y - heightOffset, z, bbWidth, bbHeight);
    }

    {
        int efx = (int)floorf(x), efz = (int)floorf(z);
        int efy = (int)floorf(y - PLAYER_EYE);
        unsigned char feet = worldBlock(&g_world, efx, efy, efz);
        if (isWaterId(feet)) { fallDistance = 0.0f; onFire = 0; }
        if (isLavaId(feet))  { fallDistance = 0.0f; lavaHurt(); }
        if (onFire > 0) { if (onFire % 20 == 0) hurt(0, 1); onFire--; }

        if (isAlive() && isInWall()) hurt(0, 1);

        if (isAlive() && y < -64.0f) { health = 0; die(0); }

        if (level->getDifficulty() == Difficulty::PEACEFUL && isAlive() && health < getMaxHealth()) {
            static int s_regenTick = 0;
            if (++s_regenTick >= 12 * 20) { s_regenTick = 0; heal(1); }
        }
    }

    if (btn & PSP_CTRL_SQUARE)   yRot += LOOK;
    if (btn & PSP_CTRL_CIRCLE)   yRot -= LOOK;
    if (btn & PSP_CTRL_TRIANGLE) xRot += LOOK;
    if (btn & PSP_CTRL_CROSS)    xRot -= LOOK;
    if (xRot >  89.0f) xRot =  89.0f;
    if (xRot < -89.0f) xRot = -89.0f;

    float xs = (128 - lx) / 127.0f;
    float yf = (128 - ly) / 127.0f;
    if (xs > -0.2f && xs < 0.2f) xs = 0.0f;
    if (yf > -0.2f && yf < 0.2f) yf = 0.0f;

    bool jumping = (btn & PSP_CTRL_START) != 0;

    unsigned char body = bodyBlock();
    if (flying) {
        if (jumping)              yd += 0.05f;
        if (btn & PSP_CTRL_DOWN)  yd -= 0.05f;
    } else if (jumping) {
        if (isLiquidId(body))  yd += 0.04f;
        else if (onGround)     yd = 0.42f;
    }

    xs *= 0.98f; yf *= 0.98f;

    if (bowPull > 0.0f) { xs *= 0.35f; yf *= 0.35f; }

    walkDistO = walkDist;
    float wx0 = x, wz0 = z;
    travel(xs, yf);

    extern int g_autoJump;
    if (g_autoJump && onGround && horizontalCollision && !flying && !isLiquidId(body)) {
        float sy = sinf(yRot * 3.14159265f / 180.0f), cy = cosf(yRot * 3.14159265f / 180.0f);
        float dirX = xs * cy + yf * sy, dirZ = yf * cy - xs * sy;
        float d = sqrtf(dirX * dirX + dirZ * dirZ);
        if (d > 0.01f) {
            dirX /= d; dirZ /= d;
            int ax = (int)floorf(x + dirX);
            int az = (int)floorf(z + dirZ);
            int stepY = (int)floorf(bb.y0 + 0.05f);
            unsigned char step = worldBlock(&g_world, ax, stepY, az);
            if (isSolidPhys(step) && !isFence(step) && !isFenceGate(step) && !isSlab(step)
                && !isSolidPhys(worldBlock(&g_world, ax, stepY + 1, az))
                && !isSolidPhys(worldBlock(&g_world, ax, stepY + 2, az)))
                yd = 0.42f;
        }
    }

    float wdx = x - wx0, wdz = z - wz0;
    float distSq = wdx * wdx + wdz * wdz;

    {
        std::vector<Entity*> nearby = level->getEntities(this, bb.grow(0.2f, 0.0f, 0.2f));
        for (unsigned int i = 0; i < nearby.size(); i++)
            if (nearby[i] && nearby[i]->isPushable()) nearby[i]->push(this);
    }

    yBodyRotO = yBodyRot; walkAnimSpeedO = walkAnimSpeed;

    float limbTgt = sqrtf(distSq) * 4.0f;
    if (limbTgt > 1.0f) limbTgt = 1.0f;
    walkAnimSpeed += (limbTgt - walkAnimSpeed) * 0.4f;
    walkAnimPosO = walkAnimPos;
    walkAnimPos += walkAnimSpeed;

    const float RADDEG = 180.0f / 3.14159265f;
    float bxd = x - xo, bzd = z - zo;
    float sideDist = sqrtf(bxd * bxd + bzd * bzd);
    float yBodyRotT = yBodyRot;
    if (sideDist > 0.05f) yBodyRotT = atan2f(bxd, bzd) * RADDEG;
    extern float g_attackAnim;
    if (g_attackAnim > 0.0f) yBodyRotT = yRot;
    float yBodyRotD = yBodyRotT - yBodyRot;
    while (yBodyRotD < -180.0f) yBodyRotD += 360.0f;
    while (yBodyRotD >= 180.0f) yBodyRotD -= 360.0f;
    yBodyRot += yBodyRotD * 0.3f;
    float headDiff = yRot - yBodyRot;
    while (headDiff < -180.0f) headDiff += 360.0f;
    while (headDiff >= 180.0f) headDiff -= 360.0f;
    if (headDiff < -75.0f) headDiff = -75.0f;
    if (headDiff >= 75.0f) headDiff = 75.0f;
    yBodyRot = yRot - headDiff;
    if (headDiff * headDiff > 50.0f * 50.0f) yBodyRot += headDiff * 0.2f;

    oBob = bob; oTilt = tilt;
    float tBob = xd * xd + zd * zd;
    if (tBob > 0.00001f) { tBob = sqrtf(tBob); if (tBob > 0.1f) tBob = 0.1f; }
    else                 { tBob = 0.0f; }
    if (!onGround) tBob = 0.0f;
    float tTilt = atanf(-yd * 0.2f) * 15.0f;
    if (onGround) tTilt = 0.0f;
    bob  += (tBob  - bob)  * 0.4f;
    tilt += (tTilt - tilt) * 0.8f;
    itemHandTick();

    if (onGround) {
        int fx = (int)floorf(x);
        int fy = (int)floorf(y - PLAYER_EYE - 0.2f);
        int fz = (int)floorf(z);
        if (worldBlock(&g_world, fx, fy, fz) == BLOCK_ORE_REDSTONE)
            redstoneOreInteract(&g_world, fx, fy, fz);
    }

    if (isWaterId(worldBlock(&g_world, (int)floorf(x), (int)floorf(y), (int)floorf(z)))) {
        if (--airSupply == -20) {
            airSupply = 0;
            for (int i = 0; i < 8; i++) {
                float ox = (float)rand() / RAND_MAX - (float)rand() / RAND_MAX;
                float oy = (float)rand() / RAND_MAX - (float)rand() / RAND_MAX;
                float oz = (float)rand() / RAND_MAX - (float)rand() / RAND_MAX;
                particlesBubble(x + ox, y + oy, z + oz, xd, yd, zd);
            }
            hurt(0, 2);
        }
    } else {
        airSupply = 300;
    }

    static bool s_wasInWater = false;
    int feetY = (int)floorf(y - PLAYER_EYE);
    bool inWater = isWaterId(worldBlock(&g_world, (int)floorf(x), feetY, (int)floorf(z)));
    if (inWater && !s_wasInWater) {

        float speed = sqrtf(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f) * 0.2f;
        if (speed > 1.0f) speed = 1.0f;
        level->playSound(this, "random.splash", speed,
                         1.0f + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.4f);

        float surf = feetY + 1.0f;
        int n = 1 + (int)(bbWidth * 20.0f);
        for (int i = 0; i < n; i++) {
            float rx = ((float)rand() / RAND_MAX * 2 - 1) * bbWidth;
            float rz = ((float)rand() / RAND_MAX * 2 - 1) * bbWidth;

            particlesBubble(x + rx, surf, z + rz, xd, yd - (float)rand() / RAND_MAX * 0.2f, zd);
        }
        for (int i = 0; i < n; i++) {
            float rx = ((float)rand() / RAND_MAX * 2 - 1) * bbWidth;
            float rz = ((float)rand() / RAND_MAX * 2 - 1) * bbWidth;
            particlesSplash(x + rx, surf, z + rz, xd, 0.0f, zd);
        }
    }
    s_wasInWater = inWater;
}

#include "world/entity/item_entity.h"
#include "world/inventory/inventory.h"
#include "util/mth.h"

void LocalPlayer::die(Entity* source) {

    stopSleepInBed(true, false);

    auto dropOnDeath = [this](const ItemInstance& it) {
        ItemEntity* e = new ItemEntity(&g_level, x, y - 0.3f, z, it);
        e->throwTime = 40;
        float pow = sharedRandom.nextFloat() * 0.5f;
        float dir = sharedRandom.nextFloat() * Mth::PI * 2.0f;
        e->xd = -sinf(dir) * pow;
        e->zd =  cosf(dir) * pow;
        e->yd = 0.2f;
        g_level.addEntity(e);
    };
    if (!g_inv.isCreative()) {

        for (int i = 0; i < g_inv.getContainerSize(); ++i) {
            ItemInstance* it = g_inv.getItem(i);
            if (it && !it->isNull()) dropOnDeath(*it);
            g_inv.clearSlot(i);
        }
        for (int i = 0; i < Inventory::HOTBAR; ++i) g_inv.linkSlot(i, -1);

        for (int i = 0; i < NUM_ARMOR; ++i) {
            ItemInstance* it = getArmor(i);
            if (!it) continue;
            dropOnDeath(*it);
            setArmor(i, nullptr);
        }
    }

    yd = 0.1f;
    if (source) {
        xd = -cosf((hurtDir + yRot) * Mth::PI / 180.0f) * 0.1f;
        zd = -sinf((hurtDir + yRot) * Mth::PI / 180.0f) * 0.1f;
    } else {
        xd = zd = 0.0f;
    }
    Mob::die(source);
}
