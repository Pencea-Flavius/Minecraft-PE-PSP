#include "world/entity/player.h"
#include "world/inventory/inventory.h"
#include "world/entity/entity_types.h"
#include "world/entity/item_entity.h"
#include "world/level/level.h"
#include "world/difficulty.h"
#include "client/gamemode/gamemode.h"
#include "client/renderer/particle.h"
#include "client/gui/hud.h"
#include "world/item/item.h"
#include "util/mth.h"
#include <cmath>
#include <stdlib.h>

Player::Player(Level* level)
    : Mob(level), inventory(new Inventory(true)),
      bob(0), oBob(0), tilt(0), oTilt(0),
      xCloak(0), yCloak(0), zCloak(0), xCloakO(0), yCloakO(0), zCloakO(0),
      xBob(0), yBob(0), xBobO(0), yBobO(0),
      useItemDuration(0),
      score(0), sleeping(false), sleepCounter(0), bedX(0), bedY(0), bedZ(0),
      respawnX(0), respawnY(-1), respawnZ(0) {}

Player::~Player() { delete inventory; }

int  Player::getEntityTypeId() const { return EntityTypes::IdLocalPlayer; }

bool Player::hurt(Entity* source, int dmg) {
    if (g_gameMode && g_gameMode->isCreative()) return false;

    if (isSleeping()) stopSleepInBed(true, true);

    if (source && (source->getCreatureBaseType() == EntityTypes::BaseEnemy ||
                   source->isEntityType(EntityTypes::IdArrow))) {
        int diff = level->getDifficulty();
        if      (diff == Difficulty::PEACEFUL) dmg = 0;
        else if (diff == Difficulty::EASY)     dmg = dmg / 3 + 1;
        else if (diff == Difficulty::HARD)     dmg = dmg * 3 / 2;

    }
    if (dmg == 0) return false;
    return Mob::hurt(source, dmg);
}

void Player::travel(float xs, float yf) {
    if (!flying) { Mob::travel(xs, yf); return; }

    const float yd0 = yd, fly0 = flyingSpeed;
    flyingSpeed = 0.05f;
    Mob::travel(xs, yf);
    yd = yd0 * 0.6f;
    flyingSpeed = fly0;
}

void Player::causeFallDamage(float dist) {
    int dmg = (int)ceilf(dist - 3.0f);
    if (dmg <= 0) return;
    level->playSound(this, dmg > 4 ? "damage.fallbig" : "damage.fallsmall", 0.75f, 1.0f);
    int xt = Mth::floor(x), yt = Mth::floor(y - 0.2f - heightOffset), zt = Mth::floor(z);
    int t = level->getTile(xt, yt, zt);
    if (t > 0) level->playLandSound(this, xt, yt, zt, t);
    hurt(0, dmg);
}

void Player::drop(ItemInstance* item) { drop(item, false); }

void Player::drop(ItemInstance* item, bool randomly) {
    if (item == 0) return;
    if (item->isNull()) { delete item; return; }

    ItemEntity* thrown = new ItemEntity(level, x, y - 0.3f + getHeadHeight(), z, *item);
    delete item;
    thrown->throwTime = 20 * 2;

    if (randomly) {
        float pow = sharedRandom.nextFloat() * 0.5f;
        float dir = sharedRandom.nextFloat() * Mth::PI * 2.0f;
        thrown->xd = -sinf(dir) * pow;
        thrown->zd =  cosf(dir) * pow;
        thrown->yd = 0.2f;
    }
    level->addEntity(thrown);
}

void Player::addAdditonalSaveData(CompoundTag* ) {}
void Player::readAdditionalSaveData(CompoundTag* ) {}

#include "world/level/world.h"
#include "world/level/chunk/chunk.h"

const int BED_HEAD_OFF[4][2] = { { 0, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 } };

bool bedFindStandUpPosition(World* w, int x, int y, int z, int dir, int* ox, int* oy, int* oz) {
    for (int step = 0; step <= 1; step++) {
        int bx = x - BED_HEAD_OFF[dir][0] * step;
        int bz = z - BED_HEAD_OFF[dir][1] * step;
        for (int xi = bx - 1; xi <= bx + 1; xi++)
            for (int zi = bz - 1; zi <= bz + 1; zi++)
                if (isSolidPhys(worldBlock(w, xi, y - 1, zi)) &&
                    worldBlock(w, xi, y, zi) == BLOCK_AIR &&
                    worldBlock(w, xi, y + 1, zi) == BLOCK_AIR) {
                    *ox = xi; *oy = y; *oz = zi;
                    return true;
                }
    }
    return false;
}

bool Player::checkBed() {
    return level->getTile(bedX, bedY, bedZ) == BLOCK_BED;
}

int Player::startSleepInBed(int bx, int by, int bz) {
    if (isSleeping() || !isAlive()) return BED_OTHER_PROBLEM;
    if (Mth::abs(x - (bx + 0.5f)) > 3.0f ||
        Mth::abs((y - heightOffset) - by) > 4.0f ||
        Mth::abs(z - (bz + 0.5f)) > 3.0f) return BED_TOO_FAR_AWAY;
    if (worldIsDay()) return BED_NOT_POSSIBLE_NOW;

    {
        static EntityList monsters;
        AABB region(bx + 0.5f - 8.0f, by + 0.5f - 5.0f, bz + 0.5f - 8.0f,
                    bx + 0.5f + 8.0f, by + 0.5f + 5.0f, bz + 0.5f + 8.0f);
        if (level->getEntitiesOfClass(EntityTypes::BaseEnemy, region, monsters))
            return BED_NOT_SAFE;
    }
    int data = level->getData(bx, by, bz);
    int dir = data & 3;
    float xo2 = 0.5f, zo2 = 0.5f;
    switch (dir) {
        case 0: zo2 = 0.9f; break;
        case 1: xo2 = 0.1f; break;
        case 2: zo2 = 0.1f; break;
        case 3: xo2 = 0.9f; break;
    }

    setPos(bx + xo2, by + 0.5625f + heightOffset, bz + zo2);

    sleeping = true;
    sleepCounter = 0;
    bedX = bx; bedY = by; bedZ = bz;
    xd = yd = zd = 0.0f;
    return BED_OK;
}

void Player::stopSleepInBed(bool forcefulWakeUp, bool ) {
    if (!isSleeping()) return;
    World* w = level->w;
    if (level->getTile(bedX, bedY, bedZ) == BLOCK_BED) {

        worldSetData(w, bedX, bedY, bedZ, (unsigned char)(level->getData(bedX, bedY, bedZ) & ~4));
        int dir = level->getData(bedX, bedY, bedZ) & 3;
        int sx, sy, sz;
        if (!bedFindStandUpPosition(w, bedX, bedY, bedZ, dir, &sx, &sy, &sz)) {
            sx = bedX; sy = bedY + 1; sz = bedZ;
        }
        setPos(sx + 0.5f, sy + 0.1f + heightOffset, sz + 0.5f);

        setRespawnPosition(sx, sy, sz);
    }
    sleeping = false;
    sleepCounter = forcefulWakeUp ? 0 : SLEEP_DURATION;

    if (!forcefulWakeUp) health = getMaxHealth();
}

void Player::sleepTick() {
    if (sleeping) {
        sleepCounter++;
        if (sleepCounter > SLEEP_DURATION) sleepCounter = SLEEP_DURATION;
        if (!checkBed()) { stopSleepInBed(true, false); return; }
        if (isSleepingLongEnough()) {
            World* w = level->w;
            long nt = w->dayTime + TICKS_PER_DAY;
            w->dayTime = nt - (nt % TICKS_PER_DAY);
            stopSleepInBed(false, true);
            return;
        }
        if (worldIsDay()) stopSleepInBed(false, true);
    } else if (sleepCounter > 0) {
        sleepCounter++;
        if (sleepCounter >= SLEEP_DURATION + 10) sleepCounter = 0;
    }
}

#include "world/item/armor_item.h"

ItemInstance* Player::getArmor(int slot) {
    if (slot < 0 || slot >= NUM_ARMOR) return nullptr;
    if (armor[slot].isNull()) return nullptr;
    return &armor[slot];
}

void Player::attack(Entity* target) {
    int dmg = inventory->getAttackDamage(target);
    if (dmg <= 0) return;
    target->hurt(this, dmg);
    ItemInstance* sel = inventory->getSelected();

    if (sel && !sel->isNull() && sel->getItem() && target->isMob() &&
        !(g_gameMode && g_gameMode->isCreative()))
        sel->getItem()->hurtEnemy(sel, (Mob*)target, this);
}

int Player::getTicksUsingItem() const {
    if (!isUsingItem()) return 0;
    Item* used = item.getItem();
    return used ? used->getMaxUseDuration() - useItemDuration : 0;
}

void Player::startUsingItem(const ItemInstance& used, int duration) {

    if (item.matches(&used) && item.count == used.count) return;
    item = used;
    useItemDuration = duration;
}

void Player::stopUsingItem() {
    item.setNull();
    useItemDuration = 0;

}

void Player::releaseUsingItem() {
    if (!item.isNull()) {
        Item* used = item.getItem();
        if (used) used->releaseUsing(&item, this, useItemDuration);
    }
    stopUsingItem();
}

void Player::completeUsingItem() {
    if (item.isNull()) return;
    spawnEatParticles(&item, 10);

    ItemInstance* selected = inventory->getSelected();
    bool sameStack = selected && item.matches(selected);
    Item* used = item.getItem();
    if (used) used->useTimeDepleted(&item, this);
    if (sameStack) {

        if (item.isNull())                        inventory->consumeSelected();
        else if (item.id != selected->id)         inventory->replaceSelected(item.id, (unsigned char)item.data);
        else                                      *selected = item;
    }
    stopUsingItem();
}

void Player::spawnEatParticles(const ItemInstance* used, int count) {
    if (!used || used->isNull()) return;
    Item* usedItem = used->getItem();
    int anim = usedItem ? usedItem->getUseAnimation() : 0;
    if (anim == 2) {
        level->playSound(this, "random.drink", 0.5f, (rand() / (float)RAND_MAX) * 0.1f + 0.9f);
        return;
    }
    if (anim != 1) return;
    int iconCell = itemFlatIcon(used->id, (unsigned char)used->data);

    particlesEat(x, y + getHeadHeight(), z, yRot, xRot, iconCell, count);

    float r1 = rand() / (float)RAND_MAX, r2 = rand() / (float)RAND_MAX;
    level->playSound(this, "random.eat", 0.5f + 0.5f * (rand() % 2), (r1 - r2) * 0.2f + 1.0f);
}

void Player::setArmor(int slot, const ItemInstance* item) {
    if (slot < 0 || slot >= NUM_ARMOR) return;
    if (!item) armor[slot].setNull();
    else       armor[slot] = *item;
}

static bool isArmorItem(const ItemInstance& it) {
    if (it.isNull()) return false;
    Item* item = it.getItem();
    return item && item->isArmor();
}

int Player::getArmorValue() {
    int val = 0;
    for (int i = 0; i < NUM_ARMOR; i++)
        if (isArmorItem(armor[i]))
            val += ((ArmorItem*)armor[i].getItem())->getDefense();
    return val;
}

void Player::hurtArmor(int dmg) {
    dmg = dmg / 4; if (dmg < 1) dmg = 1;
    for (int i = 0; i < NUM_ARMOR; i++) {
        if (!isArmorItem(armor[i])) continue;
        armor[i].hurt(dmg);
        if (armor[i].count == 0) armor[i].setNull();
    }
}
