
#include "world/level/tile/tile.h"
#include "world/entity/mob.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/entity/ai/look_control.h"
#include "world/entity/ai/move_control.h"
#include "world/entity/ai/jump_control.h"
#include "world/entity/ai/body_control.h"
#include "world/entity/ai/sensing.h"
#include "world/level/pathfinder/path_navigation.h"
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

    flyingSpeed(0.02f), flySlowdown(1.0f), defaultLookAngle(0.0f),
    health(10), lastHealth(0), lastHurt(0),
    hurtTime(0), hurtDuration(0), deathTime(0), attackTime(0),
    invulnerableDuration(20), dmgSpill(0), hurtDir(0), noActionTime(0),
    yHeadRot(0), yHeadRotO(0), yBodyRot(0), yBodyRotO(0),
    walkAnimSpeed(0), walkAnimSpeedO(0),
    walkAnimPos(0), walkAnimPosO(0),
    run(0), oRun(0), animStep(0), animStepO(0), lookTime(0),
    attackAnim(0), oAttackAnim(0), swingTime(-1), swinging(false),
    lastHurtByMobId(0), lastHurtByMobTime(0), attackTargetId(0),
    speed(0.7f), ambientSoundTime(0)
{
    blocksBuilding = true;
    footSize = 0.5f;
    health = getMaxHealth();

    lookControl = new LookControl(this);
    moveControl = new MoveControl(this);
    jumpControl = new JumpControl(this);
    bodyControl = new BodyControl(this);
    navigation  = new PathNavigation(this, level, 16.0f);
    sensing     = new Sensing(this);
}

Mob::~Mob() {
    delete lookControl; delete moveControl; delete jumpControl;
    delete bodyControl; delete navigation;  delete sensing;
}

void Mob::setLastHurtByMob(Mob* m) {
    lastHurtByMobId   = m ? m->entityId : 0;
    lastHurtByMobTime = m ? 60 : 0;
}

Entity* Mob::getTarget() {
    if (!attackTargetId) return 0;
    Entity* e = level->getEntity(attackTargetId);
    if (!e || !e->isAlive()) { attackTargetId = 0; return 0; }
    return e;
}

Mob* Mob::getLastHurtByMob() {
    if (!lastHurtByMobId) return 0;
    Entity* e = level->getEntity(lastHurtByMobId);
    if (!e || !e->isAlive() || !e->isMob()) { setLastHurtByMob(0); return 0; }
    return (Mob*)e;
}

void Mob::newServerAiStep() {
    noActionTime++;
    checkDespawn();
    if (removed) return;
    jumping = false;
    goalSelector2.tick();
    goalSelector.tick();
    navigation->tick();
    serverAiMobStep();
    moveControl->tick();
    lookControl->tick();
    jumpControl->tick();
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

    bool inWater = isInWater(), inLava = inWater ? false : isInLava();
    if (inWater) {
        float yo = y;
        moveRelative(xs, yf, useNewAi() ? 0.04f : 0.02f);
        move(xd, yd, zd);
        xd *= 0.80f; yd *= 0.80f; zd *= 0.80f;
        yd -= 0.02f;
        if (horizontalCollision && isFreeM(xd, yd + 0.6f - y + yo, zd))
            yd = 0.3f;
    } else if (inLava) {
        float yo = y;
        moveRelative(xs, yf, 0.02f);
        move(xd, yd, zd);
        xd *= 0.50f; yd *= 0.50f; zd *= 0.50f;
        yd -= 0.02f;
        if (horizontalCollision && isFreeM(xd, yd + 0.6f - y + yo, zd))
            yd = 0.3f;
    } else {

        float friction = 0.91f;
        if (onGround) {
            unsigned char under = worldBlock(&g_world, (int)floorf(x),
                                             (int)floorf(bb.y0 - 0.5f), (int)floorf(z));
            friction = (under == BLOCK_AIR) ? 0.546f
                                            : Tile::tiles[under]->slipperiness * 0.91f;
        }
        float f3 = friction * friction * friction;
        float friction2 = (0.6f * 0.6f * 0.91f * 0.91f * 0.6f * 0.91f) / f3;

        moveRelative(xs, yf, onGround ? (useNewAi() ? getSpeed() : 0.1f) * getWalkingSpeedModifier() * friction2
                                      : flyingSpeed);

        bool ladder = onLadder();
        if (ladder) {
            fallDistance = 0.0f;
            if (yd < -0.15f) yd = -0.15f;

            if (isSneaking() && yd < 0.0f) yd = 0.0f;
        }

        move(xd, yd, zd);

        if (horizontalCollision && ladder) {
            yd = 0.2f;
        }

        yd -= 0.08f;
        yd *= 0.98f;

        const float hdrag = friction * flySlowdown;
        xd *= hdrag;
        zd *= hdrag;
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

    if (isAlive() &&
        isWaterId(worldBlock(&g_world, (int)floorf(x),
                             (int)floorf(y + bbHeight * 0.85f), (int)floorf(z)))) {

        if (--airSupply == -20) {
            airSupply = 0;
            for (int i = 0; i < 8; i++) {
                float ox = sharedRandom.nextFloat() - sharedRandom.nextFloat();
                float oy = sharedRandom.nextFloat() - sharedRandom.nextFloat();
                float oz = sharedRandom.nextFloat() - sharedRandom.nextFloat();
                particlesBubble(x + ox, y + oy, z + oz, xd, yd, zd);
            }
            hurt(0, 2);
        }
        onFire = 0;
    } else {
        airSupply = TOTAL_AIR_SUPPLY;
    }

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
    yHeadRotO = yHeadRot;

    sensing->clear();

    if (lastHurtByMobId > 0) {
        if (lastHurtByMobTime <= 0) setLastHurtByMob(0);
        else                        lastHurtByMobTime--;
    }
}

void Mob::updateWalkAnim() {
    walkAnimSpeedO = walkAnimSpeed;
    float amp = (isOnFire() || hurtTime > 0) ? 1.5f : 1.0f;
    if (!onGround && yd > 0.0f) amp *= 0.35f;
    float xxd = x - xo, zzd = z - zo;
    float wst = sqrtf(xxd * xxd + zzd * zzd) * 4.0f;
    if (wst > 1.0f) wst = 1.0f;
    if (wst <= 0.0f) {
        float turn = fabsf(yBodyRot - yBodyRotO) * 0.15f;
        wst = (turn > 0.5f) ? 0.5f : turn;
    }
    walkAnimSpeed += (wst * amp - walkAnimSpeed) * 0.4f;
    walkAnimPosO = walkAnimPos;
    walkAnimPos += walkAnimSpeed;
}

void Mob::outOfWorld() {
    int before = health;
    hurt(0, 4);
    if (health >= before) actuallyHurt(4);
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
    float ay = (y - heightOffset) + bbHeight * 0.85f;
    float by = (e->y - e->heightOffset) + e->bbHeight * 0.85f;
    return !worldClip(level->w, x, ay, z, e->x, by, e->z, false, false).hit;
}

void Mob::checkDespawn() {
    if (!level->player) return;
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

void Mob::updateAi() {
    noActionTime++;
    checkDespawn();
    if (removed) return;
    xxa = 0; yya = 0;

    if (lookTime <= 0 && level->player && sharedRandom.nextFloat() < 0.02f) {
        float dx = level->player->x - x, dz = level->player->z - z;
        if (dx * dx + dz * dz < 64.0f) lookTime = 10 + sharedRandom.nextInt(20);
    }
    if (lookTime > 0 && level->player) {
        lookTime--;
        float dx = level->player->x - x, dz = level->player->z - z;
        float want = atan2f(dz, dx) * MOB_RADDEG - 90.0f;
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

    applySwimUrge();
}

void Mob::applySwimUrge() {
    if (sharedRandom.nextFloat() < 0.8f && (isInWater() || isInLava())) jumping = true;
}

void Mob::aiStep() {

    bool farAway = false;
    if (level->player) {
        float ddx = x - level->player->x, ddy = y - level->player->y, ddz = z - level->player->z;
        farAway = (ddx * ddx + ddy * ddy + ddz * ddz) > (mobAiRange() * mobAiRange());
    }

    if (isImmobile() || farAway) {

        if (farAway) { checkDespawn(); if (removed) return; }
        jumping = false; xxa = 0; yya = 0; yRotA = 0;

        if (farAway) applySwimUrge();
    } else if (!level->isClientSide) {

        if (useNewAi()) newServerAiStep();
        else            updateAi();
    }

    if (!useNewAi()) yHeadRot = yRot;

    bool inWater = isInWater(), inLava = inWater ? false : isInLava();
    if (jumping) {
        if (inWater || inLava) yd += 0.04f;
        else if (onGround)     jumpFromGround();
    }

    xxa *= 0.98f; yya *= 0.98f; yRotA *= 0.9f;

    float nf = flyingSpeed;
    flyingSpeed *= getWalkingSpeedModifier();
    travel(xxa, yya);
    flyingSpeed = nf;

    if (!farAway) {
        AABB region = bb.grow(0.2f, 0.0f, 0.2f);

        static std::vector<Entity*> nearby;
        level->getEntities(this, region, nearby);
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

    float mdx = x - xo, mdz = z - zo;
    float sideDist = sqrtf(mdx * mdx + mdz * mdz);
    float yBodyRotT = yBodyRot;
    float walkSpeed = 0.0f;
    oRun = run;
    float tRun = 0.0f;
    if (sideDist > 0.05f) {
        tRun = 1.0f;
        walkSpeed = sideDist * 3.0f;
        yBodyRotT = atan2f(mdz, mdx) * MOB_RADDEG - 90.0f;
    }
    if (!onGround) tRun = 0.0f;
    run += (tRun - run) * 0.3f;

    if (useNewAi()) {
        bodyControl->clientTick();
    } else {
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
    }

    while (yHeadRot - yHeadRotO < -180.0f) yHeadRotO -= 360.0f;
    while (yHeadRot - yHeadRotO >= 180.0f) yHeadRotO += 360.0f;
    while (yRot - yRotO < -180.0f) yRotO -= 360.0f;
    while (yRot - yRotO >= 180.0f) yRotO += 360.0f;
    while (yBodyRot - yBodyRotO < -180.0f) yBodyRotO -= 360.0f;
    while (yBodyRot - yBodyRotO >= 180.0f) yBodyRotO += 360.0f;
    while (xRot - xRotO < -180.0f) xRotO -= 360.0f;
    while (xRot - xRotO >= 180.0f) xRotO += 360.0f;

    updateWalkAnim();
    animStep += walkSpeed;
}

bool Mob::hurt(Entity* source, int dmg) {
    if (level->isClientSide) return false;
    noActionTime = 0;
    if (health <= 0) return false;
    walkAnimSpeed = 1.5f;

    if (source && source != this && source->isMob()) setLastHurtByMob((Mob*)source);

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

bool Mob::canSpawn() {
    return level->isUnobstructed(bb)
        && level->getCubes(this, bb).empty()
        && !level->containsAnyLiquid(bb);
}
