
#include "world/entity/local_player.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/tile_shapes.h"
#include "world/difficulty.h"
#include "client/gamemode/gamemode.h"
#include "client/player/player_state.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/particle.h"
#include "world/level/tile/redstone_ore.h"
#include <cmath>
#include <cstdlib>
#include <pspctrl.h>

extern World g_world;

int   g_autoJump = 1;

int g_fineAim = 1;
float g_sensitivity = 1.0f;

float g_analogDeadzone = 0.20f;
int   g_southpaw = 0;
int   g_invertY = 0;

LocalPlayer::LocalPlayer(Level* level) : Player(level) {
    setSize(PLAYER_W, PLAYER_H);
    heightOffset = PLAYER_EYE;
    entityRendererId = ER_DEFAULT_RENDERER;
}

void LocalPlayer::aiStep(unsigned int btn, unsigned char lx, unsigned char ly,
                         unsigned char rx, unsigned char ry) {
    const float LOOK = 7.5f * g_sensitivity;

    const bool onMount = riding && !riding->removed;
    if (riding && riding->removed) riding = 0;

    sleepTick();
    if (sleeping) {
        xo = x; yo = y; zo = z;
        yRotO = yRot; xRotO = xRot;
        walkDistO = walkDist;
        oBob = bob; oTilt = tilt;
        xBobO = xBob; yBobO = yBob;
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
        if (isWaterId(feet) || isLavaId(feet)) fallDistance = 0.0f;
        if (isLavaId(feet)) lavaHurt();

        if (onFire > 0) { if (onFire % 20 == 0) hurt(0, 1); onFire--; }

        if (onFire > 1 && g_gameMode && g_gameMode->isCreative()) onFire = 1;

        if (isAlive() && isInWall()) hurt(0, 1);

        if (isAlive() && y < -64.0f) { health = 0; die(0); }

        if (level->getDifficulty() == Difficulty::PEACEFUL && isAlive() && health < getMaxHealth()) {
            static int s_regenTick = 0;
            if (++s_regenTick >= 12 * 20) { s_regenTick = 0; heal(1); }
        }
    }

    const float dz = g_analogDeadzone;
    float aH = ((int)lx - 128) / 127.0f, aV = (128 - (int)ly) / 127.0f;
    if (aH > -dz && aH < dz) aH = 0.0f;
    if (aV > -dz && aV < dz) aV = 0.0f;

    static const float LOOK_TAP_FRAC   = 0.30f;
    static const int   LOOK_RAMP_TICKS = 6;
    const unsigned int yawBtn   = btn & (PSP_CTRL_SQUARE | PSP_CTRL_CIRCLE);
    const unsigned int pitchBtn = btn & (PSP_CTRL_TRIANGLE | PSP_CTRL_CROSS);
    static unsigned int s_lastLookBtn[2] = {0, 0};
    static int          s_lookHeld[2]    = {0, 0};
    float lookRamp[2];
    const unsigned int axisBtn[2] = { yawBtn, pitchBtn };
    for (int a = 0; a < 2; a++) {
        if (axisBtn[a] != s_lastLookBtn[a]) { s_lastLookBtn[a] = axisBtn[a]; s_lookHeld[a] = 0; }
        else if (axisBtn[a] && s_lookHeld[a] < LOOK_RAMP_TICKS) s_lookHeld[a]++;
        lookRamp[a] = (axisBtn[a] && g_fineAim)
            ? LOOK_TAP_FRAC + (1.0f - LOOK_TAP_FRAC) * (float)s_lookHeld[a] / LOOK_RAMP_TICKS
            : 1.0f;
    }

    float bBtnH = 0.0f, bBtnV = 0.0f;
    if (btn & PSP_CTRL_CIRCLE)   bBtnH += 1.0f;
    if (btn & PSP_CTRL_SQUARE)   bBtnH -= 1.0f;
    if (btn & PSP_CTRL_TRIANGLE) bBtnV += 1.0f;
    if (btn & PSP_CTRL_CROSS)    bBtnV -= 1.0f;

    float bStkH = 0.0f, bStkV = 0.0f;
    if (rx || ry) {
        bStkH = ((int)rx - 128) / 127.0f;
        bStkV = (128 - (int)ry) / 127.0f;
        if (bStkH > -dz && bStkH < dz) bStkH = 0.0f;
        if (bStkV > -dz && bStkV < dz) bStkV = 0.0f;
    }

    float lookH, lookV, moveH, moveV;
    if (g_southpaw) {
        lookH = aH;
        lookV = aV;
        moveH = bBtnH + bStkH;
        moveV = bBtnV + bStkV;
    } else {
        lookH = bBtnH * lookRamp[0] + bStkH;
        lookV = bBtnV * lookRamp[1] + bStkV;
        moveH = aH;
        moveV = aV;
    }
    if (moveH >  1.0f) moveH =  1.0f; else if (moveH < -1.0f) moveH = -1.0f;
    if (moveV >  1.0f) moveV =  1.0f; else if (moveV < -1.0f) moveV = -1.0f;

    yRot += LOOK * lookH;
    xRot += (g_invertY ? -LOOK : LOOK) * lookV;
    if (xRot >  89.0f) xRot =  89.0f;
    if (xRot < -89.0f) xRot = -89.0f;

    float xs = -moveH;
    float yf =  moveV;

    bool jumping = (btn & PSP_CTRL_START) != 0 || autoJumpTime > 0;

    const bool inWater = isInWater();
    if (inWater && yf > 0.0f) jumping = true;

    const bool inLiquid = inWater || isInLava();

    flySlowdown = 1.0f;
    if (flying) {

        float ax = xs < 0.0f ? -xs : xs, af = yf < 0.0f ? -yf : yf;
        if (ax > af) af = ax;
        if (af < 0.01f) flySlowdown = 0.75f;

        if (btn & PSP_CTRL_START) yd += 0.15f;
        if (btn & PSP_CTRL_DOWN)  yd -= 0.15f;
    }

    static bool s_jumpEatenByDismount = false;
    if (!(btn & PSP_CTRL_START)) s_jumpEatenByDismount = false;

    if (jumping && onMount) {
        ride(0);
        s_jumpEatenByDismount = true;
    } else if (jumping) {
        if (inLiquid)                            yd += 0.04f;
        else if (onGround && !s_jumpEatenByDismount) yd = 0.42f;
    }

    xs *= 0.98f; yf *= 0.98f;

    bool downNow = (btn & PSP_CTRL_DOWN) != 0;

    if (flying) sneaking = false;
    else if (downNow && !prevSneakBtn) sneaking = !sneaking;
    prevSneakBtn = downNow;

    if (riding && sneaking) { ride(0); sneaking = false; }
    if (sneaking) { xs *= 0.3f; yf *= 0.3f; }

    if (bowPull > 0.0f) { xs *= 0.35f; yf *= 0.35f; }

    walkDistO = walkDist;

    xxa = xs; yya = yf;
    float wx0 = x, wz0 = z;
    if (onMount) {

        xd = yd = zd = 0.0f;
        onGround = true;
        fallDistance = 0.0f;
    } else {
        travel(xs, yf);
    }

    float wdx = x - wx0, wdz = z - wz0;
    float distSq = wdx * wdx + wdz * wdz;

    {
        static std::vector<Entity*> nearby;
        level->getEntities(this, bb.grow(0.2f, 0.0f, 0.2f), nearby);
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
    if (sideDist > 0.05f) yBodyRotT = atan2f(-bxd, bzd) * RADDEG;
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

    xBobO = xBob; yBobO = yBob;
    xBob += (xRot - xBob) * 0.5f;
    yBob += (yRot - yBob) * 0.5f;

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
    bool splashWet = false;
    {
        int bx0 = (int)floorf(bb.x0), bx1 = (int)floorf(bb.x1);
        int bz0 = (int)floorf(bb.z0), bz1 = (int)floorf(bb.z1);
        int by0 = (int)floorf(bb.y0 + 0.4f), by1 = (int)floorf(bb.y1 - 0.4f);
        for (int bx = bx0; bx <= bx1 && !splashWet; ++bx)
            for (int by = by0; by <= by1 && !splashWet; ++by)
                for (int bz = bz0; bz <= bz1 && !splashWet; ++bz)
                    if (isWaterId(worldBlock(&g_world, bx, by, bz))) splashWet = true;
    }
    if (splashWet && !s_wasInWater) doWaterSplashEffect();
    s_wasInWater = splashWet;
}

void LocalPlayer::move(float xa, float ya, float za) {

    if (autoJumpTime > 0) autoJumpTime--;

    const float prevX = x, prevZ = z;
    Entity::move(xa, ya, za);

    extern int g_autoJump;

    if (autoJumpTime > 0 || !g_autoJump || sneaking || flying || !onGround) return;

    if ((int)floorf(prevX * 2.0f) == (int)floorf(x * 2.0f) &&
        (int)floorf(prevZ * 2.0f) == (int)floorf(z * 2.0f)) return;

    const float dist = sqrtf(xa * xa + za * za);
    if (dist < 0.0001f) return;
    const float px = x + xa / dist, pz = z + za / dist;
    const int ax = (int)floorf(px);
    const int az = (int)floorf(pz);

    const int stepY = (int)floorf(y - 1.0f);
    const unsigned char step = worldBlock(&g_world, ax, stepY, az);
    const unsigned char stepData = worldData(&g_world, ax, stepY, az);

    if (!isSolidPhys(step)) return;
    if (isSolidPhys(worldBlock(&g_world, ax, (int)floorf(y), az)) ||
        isSolidPhys(worldBlock(&g_world, ax, (int)floorf(y + 1.0f), az))) return;
    if (!autoJumpable(step, stepData)) return;

    const float halfW = bbWidth * 0.5f;
    BlockAABB cb[3];
    const int nBox = Tile::tiles[step]->getAABB(&g_world, ax, stepY, az, cb);
    bool inTheWay = false;
    for (int i = 0; i < nBox && !inTheWay; i++)
        inTheWay = px >= cb[i].x0 - halfW && px <= cb[i].x1 + halfW &&
                   pz >= cb[i].z0 - halfW && pz <= cb[i].z1 + halfW;
    if (!inTheWay) return;

    autoJumpTime = 1;
}

void LocalPlayer::doWaterSplashEffect() {
    Entity::doWaterSplashEffect();

    float surf = floorf(bb.y0) + 1.0f;
    int n = 1 + (int)(bbWidth * 20.0f);
    for (int i = 0; i < n; i++) {
        float xo = (sharedRandom.nextFloat() * 2.0f - 1.0f) * bbWidth;
        float zo = (sharedRandom.nextFloat() * 2.0f - 1.0f) * bbWidth;
        particlesSplash(x + xo, surf, z + zo, xd, 0.0f, zd);
    }
}

#include "world/entity/item_entity.h"
#include "world/inventory/inventory.h"
#include "util/mth.h"

void LocalPlayer::die(Entity* source) {

    stopSleepInBed(true, false);

    auto dropOnDeath = [this](const ItemInstance& it) { drop(new ItemInstance(it), true); };
    if (!inventory->isCreative()) {

        for (int i = inventory->firstGridSlot(); i < inventory->getContainerSize(); ++i) {
            ItemInstance* it = inventory->getItem(i);
            if (it && !it->isNull()) dropOnDeath(*it);
            inventory->clearSlot(i);
        }
        for (int i = 0; i < Inventory::HOTBAR; ++i) inventory->linkSlot(i, -1);

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
