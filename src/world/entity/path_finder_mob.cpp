#include "world/entity/path_finder_mob.h"
#include "world/level/level.h"
#include "world/level/pathfinder/vec3.h"
#include "util/mth.h"
#include <cmath>

static const float PF_PI = 3.14159265f;

PathfinderMob::PathfinderMob(Level* level)
:   Mob(level), attackTargetId(0), fleeTime(0), runSpeed(0.7f), holdGround(false) {}

bool PathfinderMob::isPathFinding() { return !path.isEmpty(); }

float PathfinderMob::getWalkingSpeedModifier() {
    float speed = Mob::getWalkingSpeedModifier();
    if (fleeTime > 0) speed *= 2.0f;
    return speed;
}

void PathfinderMob::updateAi() {
    if (fleeTime > 0) fleeTime--;
    holdGround = false;
    float maxDist = 16.0f;

    Entity* attackTarget = 0;
    if (attackTargetId == 0) {
        attackTarget = findAttackTarget();
        if (attackTarget) {
            level->findPath(&path, this, attackTarget, maxDist, false, false);
            attackTargetId = attackTarget->entityId;
        }
    } else {

        attackTarget = level->getEntity(attackTargetId);
        if (!attackTarget || !attackTarget->isAlive()) {
            attackTargetId = 0;
            attackTarget = 0;
        } else {
            float d = attackTarget->distanceTo(this);
            if (canSee(attackTarget)) checkHurtTarget(attackTarget, d);
            else                      checkCantSeeTarget(attackTarget, d);
        }
    }

    if (holdGround) {
        xxa = 0; yya = 0; jumping = false;
        if (attackTarget) {
            float dx = attackTarget->x - x, dz = attackTarget->z - z;
            yRot = atan2f(dx, dz) * 180.0f / PF_PI;
        }
        return;
    }

    if (attackTarget) {
        float dx = attackTarget->x - x, dz = attackTarget->z - z;
        if (dx * dx + dz * dz < 25.0f && canSee(attackTarget)) {
            float want = atan2f(dx, dz) * 180.0f / PF_PI;
            float diff = want - yRot;
            while (diff < -180.0f) diff += 360.0f;
            while (diff >= 180.0f) diff -= 360.0f;
            if (diff >  MAX_TURN) diff =  MAX_TURN;
            if (diff < -MAX_TURN) diff = -MAX_TURN;
            yRot += diff;
            xxa = 0; yya = runSpeed; xRot = 0; jumping = false;
            if (horizontalCollision) jumping = true;
            if (!path.isEmpty()) path.destroy();
            return;
        }
    }

    bool doStroll = false;
    if (attackTarget && (path.isEmpty() || sharedRandom.nextInt(20) == 0)) {
        level->findPath(&path, this, attackTarget, maxDist, false, false);
    } else {
        if (path.isEmpty() && sharedRandom.nextInt(180) == 0) {
            doStroll = true;
        } else {
            if (sharedRandom.nextInt(120) == 0) doStroll = true;
            else if (fleeTime > 0 && (fleeTime & 7) == 1) doStroll = true;
        }
    }
    if (doStroll && noActionTime < TicksPerSecond * 5) findRandomStrollLocation();

    int yFloor = Mth::floor(bb.y0 + 0.5f);
    bool inWater = isInWater(), inLava = isInLava();
    xRot = 0;
    if (path.isEmpty() || sharedRandom.nextInt(100) == 0) {
        Mob::updateAi();
        return;
    }

    Vec3 target = path.currentPos(this);
    float r = bbWidth * 2.0f;
    bool looping = true;
    while (looping && target.distanceToSqr(x, target.y, z) < r * r) {
        path.next();
        if (path.isDone()) { looping = false; path.destroy(); }
        else target = path.currentPos(this);
    }

    jumping = false;
    if (looping) {
        float dx = target.x - x, dz = target.z - z, dy = target.y - yFloor;

        float yRotD = atan2f(dx, dz) * 180.0f / PF_PI;
        float rotDiff = yRotD - yRot;
        yya = runSpeed;
        while (rotDiff < -180.0f) rotDiff += 360.0f;
        while (rotDiff >= 180.0f) rotDiff -= 360.0f;
        if (rotDiff >  MAX_TURN) rotDiff =  MAX_TURN;
        if (rotDiff < -MAX_TURN) rotDiff = -MAX_TURN;
        yRot += rotDiff;
        if (dy > 0) jumping = true;
    }

    if (horizontalCollision && (!isPathFinding() || attackTarget != 0)) jumping = true;
    if (sharedRandom.nextFloat() < 0.8f && (inWater || inLava)) jumping = true;
}

void PathfinderMob::checkCantSeeTarget(Entity* target, float d) {
    if (d > 32.0f) attackTargetId = 0;
}

void PathfinderMob::findRandomStrollLocation() {
    bool hasBest = false;
    int xBest = 0, yBest = 0, zBest = 0;
    float best = -99999.0f;
    for (int i = 0; i < 10; i++) {
        int xt = Mth::floor(x + sharedRandom.nextInt(13) - 6);
        int yt = Mth::floor(y + sharedRandom.nextInt(7) - 3);
        int zt = Mth::floor(z + sharedRandom.nextInt(13) - 6);
        float value = getWalkTargetValue(xt, yt, zt);
        if (value > best) { best = value; xBest = xt; yBest = yt; zBest = zt; hasBest = true; }
    }
    if (hasBest) level->findPath(&path, this, xBest, yBest, zBest, 10.0f, false, false);
}
