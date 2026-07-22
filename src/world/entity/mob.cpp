
#include "world/level/tile/tile.h"
#include "world/entity/mob.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "client/player/physics.h"

float mobAiRange() {
    extern float g_viewDist;
    return (g_viewDist <= 16.0f) ? 32.0f : 48.0f;
}
#include "client/player/player_state.h"
#include "nbt/compound_tag.h"
#include "client/renderer/particle.h"
#include <cmath>
#include <vector>

extern World g_world;

static inline int ifloor(float v) { int i = (int)v; return (v < 0 && v != i) ? i - 1 : i; }

Mob::Mob(Level* level)
:   Entity(level), flying(false),
    xxa(0), yya(0), yRotA(0), jumping(false),
    walkingSpeed(0.1f),

    flyingSpeed(0.02f), defaultLookAngle(0.0f),
    health(10), lastHealth(0), lastHurt(0),
    hurtTime(0), hurtDuration(0), deathTime(0), attackTime(0),
    invulnerableDuration(20), dmgSpill(0), hurtDir(0), noActionTime(0),
    yBodyRot(0), yBodyRotO(0), walkAnimSpeed(0), walkAnimSpeedO(0),
    walkAnimPos(0), walkAnimPosO(0),
    run(0), oRun(0), animStep(0), animStepO(0), lookTime(0),
    attackAnim(0), oAttackAnim(0), swingTime(-1), swinging(false),
    ambientSoundTime(0), airSupply(300)
{
    blocksBuilding = true;
    health = getMaxHealth();
}

void Mob::move(float xa, float ya, float za) {
    if (isStuckInWeb) {
        isStuckInWeb = false;
        xa *= .25f; ya *= .05f; za *= .25f;
        xd = yd = zd = 0.0f;
    }

    float c[6] = { x - bbWidth * 0.5f, y - heightOffset,            z - bbWidth * 0.5f,
                   x + bbWidth * 0.5f, y - heightOffset + bbHeight, z + bbWidth * 0.5f };
    float cPre[6]; for (int i = 0; i < 6; i++) cPre[i] = c[i];
    float xaOrg = xa, yaOrg = ya, zaOrg = za;
    bool wasOnGround = onGround;
    float xOrigin = x, zOrigin = z;

    auto moveAABB = [](float& dx, float& dy, float& dz, float box[6]) {
        float ex0 = box[0] + (dx < 0 ? dx : 0), ex1 = box[3] + (dx > 0 ? dx : 0);
        float ey0 = box[1] + (dy < 0 ? dy : 0), ey1 = box[4] + (dy > 0 ? dy : 0);
        float ez0 = box[2] + (dz < 0 ? dz : 0), ez1 = box[5] + (dz > 0 ? dz : 0);
        int bx0 = ifloor(ex0), bx1 = ifloor(ex1 + 1.0f);
        int by0 = ifloor(ey0) - 1, by1 = ifloor(ey1 + 1.0f);
        int bz0 = ifloor(ez0), bz1 = ifloor(ez1 + 1.0f);

        for (int X = bx0; X < bx1; X++) for (int Z = bz0; Z < bz1; Z++) for (int Y = by0; Y < by1; Y++) {
            BlockAABB boxes[3];
            int num = getBlockAABBs(&g_world, X, Y, Z, boxes);
            for (int i = 0; i < num; i++)
                dy = clipYCollide(boxes[i].x0, boxes[i].y0, boxes[i].z0, boxes[i].x1, boxes[i].y1, boxes[i].z1, box, dy);
        }
        box[1] += dy; box[4] += dy;

        for (int X = bx0; X < bx1; X++) for (int Z = bz0; Z < bz1; Z++) for (int Y = by0; Y < by1; Y++) {
            BlockAABB boxes[3];
            int num = getBlockAABBs(&g_world, X, Y, Z, boxes);
            for (int i = 0; i < num; i++)
                dx = clipXCollide(boxes[i].x0, boxes[i].y0, boxes[i].z0, boxes[i].x1, boxes[i].y1, boxes[i].z1, box, dx);
        }
        box[0] += dx; box[3] += dx;

        for (int X = bx0; X < bx1; X++) for (int Z = bz0; Z < bz1; Z++) for (int Y = by0; Y < by1; Y++) {
            BlockAABB boxes[3];
            int num = getBlockAABBs(&g_world, X, Y, Z, boxes);
            for (int i = 0; i < num; i++)
                dz = clipZCollide(boxes[i].x0, boxes[i].y0, boxes[i].z0, boxes[i].x1, boxes[i].y1, boxes[i].z1, box, dz);
        }
        box[2] += dz; box[5] += dz;
    };

    float xaClip = xa, yaClip = ya, zaClip = za;
    float cClip[6];
    for (int i = 0; i < 6; i++) cClip[i] = c[i];
    moveAABB(xaClip, yaClip, zaClip, cClip);

    float stepHeight = 0.5f;
    if (stepHeight > 0.0f && wasOnGround && (xaOrg != xaClip || zaOrg != zaClip)) {
        float stepXa = xaOrg, stepYa = stepHeight, stepZa = zaOrg;
        float cStep[6];
        for (int i = 0; i < 6; i++) cStep[i] = c[i];

        moveAABB(stepXa, stepYa, stepZa, cStep);

        float stepYaDown = -stepYa;
        float noXa = 0.0f, noZa = 0.0f;
        moveAABB(noXa, stepYaDown, noZa, cStep);

        if (stepXa * stepXa + stepZa * stepZa > xaClip * xaClip + zaClip * zaClip) {
            xa = stepXa;
            ya = stepYa + stepYaDown;
            za = stepZa;
            for (int i = 0; i < 6; i++) c[i] = cStep[i];
        } else {
            xa = xaClip;
            ya = yaClip;
            za = zaClip;
            for (int i = 0; i < 6; i++) c[i] = cClip[i];
        }
    } else {
        xa = xaClip;
        ya = yaClip;
        za = zaClip;
        for (int i = 0; i < 6; i++) c[i] = cClip[i];
    }

    x = (c[0] + c[3]) * 0.5f;
    y = c[1] + heightOffset;
    z = (c[2] + c[5]) * 0.5f;
    bb.set(c[0], c[1], c[2], c[3], c[4], c[5]);

    horizontalCollision = (xaOrg != xa) || (zaOrg != za);
    onGround = (yaOrg != ya) && (yaOrg < 0);
    if (xaOrg != xa) xd = 0;
    if (yaOrg != ya) yd = 0;
    if (zaOrg != za) zd = 0;

    if (onGround) {
        if (fallDistance > 0.0f && !isInWater()) {
            if (Entity::sharedRandom.nextFloat() < (fallDistance - 0.5f)) {
                int fx = ifloor((c[0] + c[3]) * 0.5f), fz = ifloor((c[2] + c[5]) * 0.5f);
                int fy = ifloor(c[1] - 0.01f);
                if (worldBlock(&g_world, fx, fy, fz) == BLOCK_FARMLAND) {
                    worldSetBlockAndData(&g_world, fx, fy, fz, BLOCK_DIRT, 0);
                    worldNotifyNeighborsChanged(&g_world, fx, fy, fz);
                    worldRebuildAroundNow(&g_world, fx, fy, fz);
                }
            }
            causeFallDamage(fallDistance);
        }
        fallDistance = 0.0f;
    } else if (yaOrg < 0.0f) {
        fallDistance -= yaOrg;
    }

    if (makeStepSound && !isSneaking()) {
        float xm = x - xOrigin, zm = z - zOrigin;
        walkDist += sqrtf(xm * xm + zm * zm) * 0.6f;
        int xt = ifloor(x), yt = ifloor(y - 0.2f - heightOffset), zt = ifloor(z);
        int t = level->getTile(xt, yt, zt);

        if (t == 0) {
            int under = level->getTile(xt, yt - 1, zt);
            if (isFence((unsigned char)under) || isFenceGate((unsigned char)under)) t = under;
        }
        if (walkDist > nextStep && t > 0) {
            nextStep = ((int)walkDist) + 1;
            playStepSound(xt, yt, zt, t);
        }
    }

    isInWater();

    int bx0 = ifloor(bb.x0), by0 = ifloor(bb.y0), bz0 = ifloor(bb.z0);
    int bx1 = ifloor(bb.x1), by1 = ifloor(bb.y1), bz1 = ifloor(bb.z1);
    if (level->hasChunksAt(bx0, by0, bz0, bx1, by1, bz1)) {
        for (int ix = bx0; ix <= bx1; ix++)
            for (int iy = by0; iy <= by1; iy++)
                for (int iz = bz0; iz <= bz1; iz++) {
                    int t = level->getTile(ix, iy, iz);
                    if (t > 0) Tile::tiles[t]->entityInside(&g_world, ix, iy, iz, this);
                }
    }
}

bool Mob::isFreeM(float dx, float dy, float dz) {
    float x0 = x - bbWidth * 0.5f + dx, x1 = x + bbWidth * 0.5f + dx;
    float y0 = y - heightOffset + dy,      y1 = y0 + bbHeight;
    float z0 = z - bbWidth * 0.5f + dz, z1 = z + bbWidth * 0.5f + dz;
    for (int X = ifloor(x0); X < ifloor(x1 + 1.0f); X++)
    for (int Z = ifloor(z0); Z < ifloor(z1 + 1.0f); Z++)
    for (int Y = ifloor(y0); Y < ifloor(y1 + 1.0f); Y++) {
        unsigned char blk = worldBlock(&g_world, X, Y, Z);
        if (isLiquidId(blk)) return false;
        BlockAABB boxes[3];
        int num = getBlockAABBs(&g_world, X, Y, Z, boxes);
        for (int i = 0; i < num; i++) {
            if (x0 < boxes[i].x1 && x1 > boxes[i].x0 &&
                y0 < boxes[i].y1 && y1 > boxes[i].y0 &&
                z0 < boxes[i].z1 && z1 > boxes[i].z0) {
                return false;
            }
        }
    }
    return true;
}

void Mob::mobMoveRelative(float xs, float yf, float speed) {
    float dist = sqrtf(xs * xs + yf * yf);
    if (dist < 0.01f) return;
    if (dist < 1.0f) dist = 1.0f;
    dist = speed / dist;
    xs *= dist; yf *= dist;
    float sy = sinf(yRot * 3.14159265f / 180.0f), cy = cosf(yRot * 3.14159265f / 180.0f);
    xd += xs * cy + yf * sy;
    zd += yf * cy - xs * sy;
}

unsigned char Mob::bodyBlock() {
    return worldBlock(&g_world, ifloor(x), ifloor(y - heightOffset + 0.4f), ifloor(z));
}

bool Mob::onLadder() {
    int px = (int)floorf(x);
    int pz = (int)floorf(z);
    unsigned char b0 = worldBlock(&g_world, px, (int)floorf(y - heightOffset), pz);
    unsigned char b1 = worldBlock(&g_world, px, (int)floorf(y - heightOffset + 1.0f), pz);
    return isLadder(b0) || isLadder(b1);
}

void Mob::travel(float xs, float yf) {
    unsigned char body = bodyBlock();
    if (flying) {
        mobMoveRelative(xs, yf, 0.05f);
        move(xd, yd, zd);
        xd *= 0.91f; yd *= 0.91f; zd *= 0.91f;
    } else if (isWaterId(body)) {
        float yo = y;
        mobMoveRelative(xs, yf, 0.02f);
        move(xd, yd, zd);
        xd *= 0.80f; yd *= 0.80f; zd *= 0.80f;
        yd -= 0.02f;
        if (horizontalCollision && isFreeM(xd, yd + 0.6f - y + yo, zd))
            yd = 0.3f;
    } else if (isLavaId(body)) {
        float yo = y;
        mobMoveRelative(xs, yf, 0.02f);
        move(xd, yd, zd);
        xd *= 0.50f; yd *= 0.50f; zd *= 0.50f;
        yd -= 0.02f;
        if (horizontalCollision && isFreeM(xd, yd + 0.6f - y + yo, zd))
            yd = 0.3f;
    } else {
        float groundFriction = 0.6f;
        if (onGround) {
            int blockX = (int)floorf(x);
            int blockY = (int)floorf(y - heightOffset - 0.5f);
            int blockZ = (int)floorf(z);
            unsigned char idUnder = worldBlock(&g_world, blockX, blockY, blockZ);
            if (idUnder == BLOCK_ICE) groundFriction = 0.98f;
        }

        float friction = onGround ? groundFriction * 0.91f : 0.91f;
        float f3 = friction * friction * friction;
        float friction2 = (0.6f * 0.6f * 0.91f * 0.91f * 0.6f * 0.91f) / f3;
        mobMoveRelative(xs, yf, onGround ? walkingSpeed * friction2 : flyingSpeed);

        bool ladder = onLadder();
        if (ladder) {
            fallDistance = 0.0f;
            if (yd < -0.15f) yd = -0.15f;
        }

        move(xd, yd, zd);

        if (horizontalCollision && ladder) {
            yd = 0.2f;
        }

        yd -= 0.08f;
        yd *= 0.98f;
        xd *= friction;
        zd *= friction;
    }
}

static const float MOB_RADDEG = 180.0f / 3.14159265f;

float Mob::getVoicePitch() {
    float base = isBaby() ? 1.5f : 1.0f;
    return (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.2f + base;
}

void Mob::playAmbientSound() {
    const char* ambient = getAmbientSound();
    if (ambient) level->playSound(this, ambient, getSoundVolume(), getVoicePitch());
}

void Mob::baseTick() {
    Entity::baseTick();

    if (((ambientSoundTime++ & 15) == 0) && (rand() % 2000) < ambientSoundTime) {
        ambientSoundTime = -getAmbientSoundInterval();
        playAmbientSound();
    }

    if (isAlive() && isInWall()) hurt(0, 1);
    if (attackTime > 0) attackTime--;
    if (hurtTime > 0) hurtTime--;
    if (invulnerableTime > 0) invulnerableTime--;
    if (health <= 0) {
        deathTime++;
        if (deathTime > TicksPerSecond) {

            particlesMobDeath(x, y - heightOffset, z, bbWidth, bbHeight);
            remove();
        }
    }
    animStepO = animStep;
    yBodyRotO = yBodyRot;
}

void Mob::updateWalkAnim() {
    walkAnimSpeedO = walkAnimSpeed;
    float xxd = x - xo, zzd = z - zo;
    float wst = sqrtf(xxd * xxd + zzd * zzd) * 4.0f;
    if (wst > 1.0f) wst = 1.0f;
    walkAnimSpeed += (wst - walkAnimSpeed) * 0.4f;
    walkAnimPosO = walkAnimPos;
    walkAnimPos += walkAnimSpeed;
}

void Mob::jumpFromGround() { yd = 0.42f; }

void Mob::causeFallDamage(float dist) {
    int d = (int)ceilf(dist - 3.0f);
    if (d <= 0) return;
    level->playSound(this, d > 4 ? "damage.fallbig" : "damage.fallsmall", 0.75f, 1.0f);
    hurt(0, d);

    int xt = ifloor(x), yt = ifloor(y - 0.2f - heightOffset), zt = ifloor(z);
    int t = level->getTile(xt, yt, zt);
    if (t > 0) level->playLandSound(this, xt, yt, zt, t);
}

bool Mob::canSee(Entity* e) {
    if (!e) return false;
    float ax = x, ay = (y - heightOffset) + bbHeight * 0.85f, az = z;
    float bx = e->x, by = (e->y - e->heightOffset) + e->bbHeight * 0.85f, bz = e->z;
    float dx = bx - ax, dy = by - ay, dz = bz - az;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (dist < 1e-4f) return true;
    int steps = (int)(dist * 2.0f) + 1;
    float inv = 1.0f / (float)steps;
    for (int i = 1; i < steps; i++) {
        float t = i * inv;
        int cx = (int)floorf(ax + dx * t);
        int cy = (int)floorf(ay + dy * t);
        int cz = (int)floorf(az + dz * t);
        if (level->isSolidBlockingTile(cx, cy, cz)) return false;
    }
    return true;
}

void Mob::updateAi() {
    noActionTime++;

    if (level->player) {
        float pdx = level->player->x - x;
        float pdy = level->player->y - y;
        float pdz = level->player->z - z;
        float sd = pdx * pdx + pdy * pdy + pdz * pdz;
        bool removeIfFar = removeWhenFarAway();
        if (removeIfFar && sd > 96.0f * 96.0f) { remove(); return; }
        if (noActionTime > 30 * 20 && sharedRandom.nextInt(800) == 0 &&
            removeIfFar && sd > 32.0f * 32.0f)
            remove();
        else
            noActionTime = 0;
    }
    xxa = 0; yya = 0;

    if (lookTime <= 0 && level->player && sharedRandom.nextFloat() < 0.02f) {
        float dx = level->player->x - x, dz = level->player->z - z;
        if (dx * dx + dz * dz < 64.0f) lookTime = 10 + sharedRandom.nextInt(20);
    }
    if (lookTime > 0 && level->player) {
        lookTime--;
        float dx = level->player->x - x, dz = level->player->z - z;
        float want = atan2f(dx, dz) * MOB_RADDEG;
        float diff = want - yRot;
        while (diff < -180.0f) diff += 360.0f;
        while (diff >= 180.0f) diff -= 360.0f;
        if (diff >  10.0f) diff =  10.0f;
        if (diff < -10.0f) diff = -10.0f;
        yRot += diff;
    } else {
        if (sharedRandom.nextFloat() < 0.05f)
            yRotA = (sharedRandom.nextFloat() - 0.5f) * 20.0f;
        yRot += yRotA;
    }
    xRot = defaultLookAngle;

    unsigned char body = bodyBlock();
    if (isWaterId(body) || isLavaId(body))
        jumping = sharedRandom.nextFloat() < 0.8f;
}

void Mob::aiStep() {

    bool farAway = false;
    if (level->player) {
        float ddx = x - level->player->x, ddy = y - level->player->y, ddz = z - level->player->z;
        farAway = (ddx * ddx + ddy * ddy + ddz * ddz) > (mobAiRange() * mobAiRange());
    }

    if (isImmobile() || farAway) {
        jumping = false; xxa = 0; yya = 0; yRotA = 0;
    } else if (!level->isClientSide) {
        updateAi();
    }

    unsigned char body = bodyBlock();
    bool inWater = isWaterId(body), inLava = isLavaId(body);
    if (jumping) {
        if (inWater || inLava) yd += 0.04f;
        else if (onGround)     jumpFromGround();
    }

    xxa *= 0.98f; yya *= 0.98f; yRotA *= 0.9f;

    float ns = walkingSpeed, nf = flyingSpeed;
    walkingSpeed *= getWalkingSpeedModifier();
    flyingSpeed  *= getWalkingSpeedModifier();
    travel(xxa, yya);
    walkingSpeed = ns; flyingSpeed = nf;

    if (!farAway) {
        AABB region = bb.grow(0.2f, 0.0f, 0.2f);
        std::vector<Entity*> nearby = level->getEntities(this, region);
        for (unsigned int i = 0; i < nearby.size(); i++)
            if (nearby[i] && nearby[i]->isPushable()) nearby[i]->push(this);

        if (level->player) {
            Entity* p = level->player;
            if (region.intersects(p->bb.x0, p->bb.y0, p->bb.z0, p->bb.x1, p->bb.y1, p->bb.z1))
                p->push(this);
        }
    }
}

static const int MOB_SWING_DUR = 6;
void Mob::updateAttackAnim() {
    oAttackAnim = attackAnim;
    if (swinging) {
        swingTime++;
        if (swingTime >= MOB_SWING_DUR) { swingTime = 0; swinging = false; }
    } else {
        swingTime = 0;
    }
    attackAnim = (float)swingTime / (float)MOB_SWING_DUR;
}

void Mob::swing() {
    if (!swinging || swingTime >= MOB_SWING_DUR / 2 || swingTime < 0) {
        swingTime = -1;
        swinging = true;
    }
}

void Mob::tick() {
    xOld = x; yOld = y; zOld = z;
    updateAttackAnim();
    Entity::tick();
    aiStep();
    updateWalkAnim();

    float mdx = x - xo, mdz = z - zo;
    float sideDist = sqrtf(mdx * mdx + mdz * mdz);
    float yBodyRotT = yBodyRot;
    float walkSpeed = 0.0f;
    oRun = run;
    float tRun = 0.0f;
    if (sideDist > 0.05f) {
        tRun = 1.0f;
        walkSpeed = sideDist * 3.0f;

        yBodyRotT = atan2f(mdx, mdz) * MOB_RADDEG;
    }
    if (!onGround) tRun = 0.0f;
    run += (tRun - run) * 0.3f;

    float yBodyRotD = yBodyRotT - yBodyRot;
    while (yBodyRotD < -180.0f) yBodyRotD += 360.0f;
    while (yBodyRotD >= 180.0f) yBodyRotD -= 360.0f;
    yBodyRot += yBodyRotD * 0.3f;

    float headDiff = yRot - yBodyRot;
    while (headDiff < -180.0f) headDiff += 360.0f;
    while (headDiff >= 180.0f) headDiff -= 360.0f;
    bool behind = headDiff < -90.0f || headDiff >= 90.0f;
    if (headDiff < -75.0f) headDiff = -75.0f;
    if (headDiff >= 75.0f) headDiff = 75.0f;
    yBodyRot = yRot - headDiff;
    if (headDiff * headDiff > 50.0f * 50.0f) yBodyRot += headDiff * 0.2f;
    if (behind) walkSpeed *= -1.0f;

    while (yRot - yRotO < -180.0f) yRotO -= 360.0f;
    while (yRot - yRotO >= 180.0f) yRotO += 360.0f;
    while (yBodyRot - yBodyRotO < -180.0f) yBodyRotO -= 360.0f;
    while (yBodyRot - yBodyRotO >= 180.0f) yBodyRotO += 360.0f;
    while (xRot - xRotO < -180.0f) xRotO -= 360.0f;
    while (xRot - xRotO >= 180.0f) xRotO += 360.0f;
    animStep += walkSpeed;
}

bool Mob::hurt(Entity* source, int dmg) {
    if (level->isClientSide) return false;
    noActionTime = 0;
    if (health <= 0) return false;
    walkAnimSpeed = 1.5f;

    bool sound = true;
    if (invulnerableTime > invulnerableDuration / 2) {
        if (dmg <= lastHurt) return false;
        actuallyHurt(dmg - lastHurt);
        lastHurt = dmg;
        sound = false;
    } else {
        lastHurt = dmg;
        lastHealth = health;
        invulnerableTime = invulnerableDuration;
        actuallyHurt(dmg);
        hurtTime = hurtDuration = 10;
    }

    hurtDir = 0;
    if (sound) {
        markHurt();
        if (source != 0) {
            float sxd = source->x - x, szd = source->z - z;
            while (sxd * sxd + szd * szd < 0.0001f) {
                sxd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.01f;
                szd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.01f;
            }
            hurtDir = atan2f(szd, sxd) * MOB_RADDEG - yRot;
            knockback(source, dmg, sxd, szd);
        } else {
            hurtDir = (float)((int)(sharedRandom.nextFloat() * 2.0f) * 180);
        }
    }

    if (health <= 0) {
        if (sound) level->playSound(this, getDeathSound(), getSoundVolume(), getVoicePitch());
        die(source);
    } else {
        if (sound) level->playSound(this, getHurtSound(), getSoundVolume(), getVoicePitch());
    }
    return true;
}

void Mob::actuallyHurt(int dmg) {
    dmg = getDamageAfterArmorAbsorb(dmg);
    health -= dmg;
}

int Mob::getDamageAfterArmorAbsorb(int damage) {
    int absorb = 25 - getArmorValue();
    int v = damage * absorb + dmgSpill;
    hurtArmor(damage);
    damage = v / 25;
    dmgSpill = v % 25;
    return damage;
}

void Mob::knockback(Entity* , int , float kxd, float kzd) {
    float len = sqrtf(kxd * kxd + kzd * kzd);
    float dd = (len > 0.0001f) ? 1.0f / len : 0.0f;
    const float pow = 0.4f;
    xd *= 0.5f; yd *= 0.5f; zd *= 0.5f;
    xd -= kxd * dd * pow;
    yd += 0.4f;
    zd -= kzd * dd * pow;
    if (yd > 0.4f) yd = 0.4f;
}

void Mob::die(Entity* source) {
    if (source) source->awardKillScore(this, 0);
    if (!level->isClientSide && !isBaby()) dropDeathLoot();
}

void Mob::dropDeathLoot() {
    int loot = getDeathLoot();
    if (loot > 0) {
        int count = sharedRandom.nextInt(3);
        for (int i = 0; i < count; i++) spawnAtLocation(loot, 1);
    }
}

void Mob::heal(int amount) {
    if (health <= 0) return;
    health += amount;
    if (health > getMaxHealth()) health = getMaxHealth();
}

void Mob::animateHurt() { hurtTime = hurtDuration = 10; hurtDir = 0; }
bool Mob::isAlive() { return !removed && health > 0; }

void Mob::addAdditonalSaveData(CompoundTag* tag) {
    tag->putShort("Health", (short)health);
    tag->putShort("HurtTime", (short)hurtTime);
    tag->putShort("DeathTime", (short)deathTime);
    tag->putShort("AttackTime", (short)attackTime);
}

void Mob::readAdditionalSaveData(CompoundTag* tag) {
    health = tag->getShort("Health");
    hurtTime = tag->getShort("HurtTime");
    deathTime = tag->getShort("DeathTime");
    attackTime = tag->getShort("AttackTime");
}
