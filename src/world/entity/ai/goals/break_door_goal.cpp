#include "world/entity/ai/goals/break_door_goal.h"
#include "world/entity/mob.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/difficulty.h"
#include "client/renderer/particle.h"

extern World g_world;

bool BreakDoorGoal::canUse() {
    if (!DoorInteractGoal::canUse()) return false;
    return !isDoorOpen();
}

bool BreakDoorGoal::canContinueToUse() {
    if (ticksLeft < 0) return false;

    if (!isDoorAt(doorX, doorY, doorZ)) return false;
    if (isDoorOpen()) return false;
    return mob->distanceToSqr((float)doorX, (float)doorY, (float)doorZ) < 4.0f;
}

void BreakDoorGoal::start() {
    DoorInteractGoal::start();
    ticksLeft = 240;
    lastBreakProgress = -1;
}

void BreakDoorGoal::stop() {
    DoorInteractGoal::stop();
    mob->level->destroyTileProgress(mob->entityId, doorX, doorY, doorZ, -1);
}

static float doorSoundPitch() {
    return (Entity::sharedRandom.nextFloat() - Entity::sharedRandom.nextFloat()) * 0.2f + 1.0f;
}

void BreakDoorGoal::tick() {
    DoorInteractGoal::tick();
    float cx = doorX + 0.5f, cy = doorY + 0.5f, cz = doorZ + 0.5f;

    if (Entity::sharedRandom.nextInt(20) == 0) {
        mob->level->playSound(cx, cy, cz, "mob.zombie.wood", 2.0f, doorSoundPitch());
        mob->swing();
    }

    --ticksLeft;
    int progress = (240 - ticksLeft) * 10 / 240;
    if (progress != lastBreakProgress) {
        mob->level->destroyTileProgress(mob->entityId, doorX, doorY, doorZ, progress);
        lastBreakProgress = progress;
    }

    if (ticksLeft != 0) return;

    if (mob->level->getDifficulty() != Difficulty::HARD) return;

    unsigned char d  = (unsigned char)mob->level->getData(doorX, doorY, doorZ);
    unsigned char id = worldBlock(&g_world, doorX, doorY, doorZ);
    particlesDestroyBlock(&g_world, doorX, doorY, doorZ, id, d);
    worldSetTileUpdate(&g_world, doorX, doorY, doorZ, 0, 0);
    worldRebuildAroundNow(&g_world, doorX, doorY, doorZ);
    mob->level->playSound(cx, cy, cz, "mob.zombie.woodbreak", 2.0f, doorSoundPitch());
}
