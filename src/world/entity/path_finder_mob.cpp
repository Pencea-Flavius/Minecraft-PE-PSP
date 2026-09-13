#include "world/entity/path_finder_mob.h"
#include "world/level/level.h"
#include "world/level/pathfinder/vec3.h"
#include "util/mth.h"
#include "util/random_pos.h"
#include <cmath>

static const float PF_PI = 3.14159265f;

PathfinderMob::PathfinderMob(Level* level)
:   Mob(level), fleeTime(0), runSpeed(0.7f), holdGround(false) {}

bool PathfinderMob::isPathFinding() { return !path.isEmpty(); }

float PathfinderMob::getWalkingSpeedModifier() {
    float speed = Mob::getWalkingSpeedModifier();

    if (fleeTime > 0 && !useNewAi()) speed *= 2.0f;
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

    if (attackTarget && !holdGround) {
        float dx = attackTarget->x - x, dz = attackTarget->z - z;
        if (dx * dx + dz * dz < 25.0f && canSee(attackTarget)) {
            float want = atan2f(dz, dx) * 180.0f / PF_PI - 90.0f;
            float diff = want - yRot;
            while (diff < -180.0f) diff += 360.0f;
            while (diff >= 180.0f) diff -= 360.0f;
            if (diff >  MAX_TURN) diff =  MAX_TURN;
            if (diff < -MAX_TURN) diff = -MAX_TURN;
            yRot += diff;
            xxa = 0; yya = runSpeed; xRot = 0; jumping = false;
            if (horizontalCollision) jumping = true;
            if (!path.isEmpty()) path.destroy();
            applySwimUrge();
            return;
        }
    }

    bool doStroll = false;
    if (attackTarget && !holdGround && (path.isEmpty() || sharedRandom.nextInt(20) == 0)) {
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
        float yRotD = atan2f(dz, dx) * 180.0f / PF_PI - 90.0f;
        float rotDiff = yRotD - yRot;
        yya = runSpeed;
        while (rotDiff < -180.0f) rotDiff += 360.0f;
        while (rotDiff >= 180.0f) rotDiff -= 360.0f;
        if (rotDiff >  MAX_TURN) rotDiff =  MAX_TURN;
        if (rotDiff < -MAX_TURN) rotDiff = -MAX_TURN;
        yRot += rotDiff;
        if (dy > 0) jumping = true;
    }

    if (holdGround && attackTarget) {
        float dx = attackTarget->x - x, dz = attackTarget->z - z;
        float want = atan2f(dz, dx) * 180.0f / PF_PI - 90.0f;
        float turned = yRot - want;
        yRot = want;
        float rad = (turned + 90.0f) * PF_PI / 180.0f;
        float fwd = yya;
        xxa = -sinf(rad) * fwd;
        yya =  cosf(rad) * fwd;
    }

    if (horizontalCollision && (!isPathFinding() || attackTarget != 0)) jumping = true;
    applySwimUrge();
}

void PathfinderMob::checkCantSeeTarget(Entity* target, float d) {
    if (d > 32.0f) attackTargetId = 0;
}

void PathfinderMob::findRandomStrollLocation() {

    Vec3 target;
    if (RandomPos::generateRandomPos(target, this, 6, 3, 0))
        level->findPath(&path, this, (int)target.x, (int)target.y, (int)target.z,
                        10.0f, false, false);
}
