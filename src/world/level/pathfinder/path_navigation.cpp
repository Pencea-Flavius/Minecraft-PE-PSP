
#include "world/level/pathfinder/path_navigation.h"
#include "world/level/pathfinder/path.h"
#include "world/level/pathfinder/node.h"
#include "world/level/pathfinder/path_finder.h"
#include "world/entity/mob.h"
#include "world/entity/ai/move_control.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "util/mth.h"
#include <cmath>

static Path s_candidate;

PathNavigation::PathNavigation(Mob* mob, Level* level, float range)
:   mob(mob), level(level), range(range), speedMultiplier(0.0f), avoidSun(false),
    tickCounter(0), lastProgressTick(0), lastProgressPos(0, 0, 0),
    _canPassDoors(true), _canOpenDoors(false), avoidWater(false), canFloat(false) {}

Path& PathNavigation::path() const { return mob->path; }

bool PathNavigation::isDone() const { return path().isEmpty() || path().isDone(); }

void PathNavigation::stop() { path().destroy(); }

bool PathNavigation::isInLiquid() const { return mob->isInWater() || mob->isInLava(); }

bool PathNavigation::canUpdatePath() const {
    if (mob->onGround) return true;
    if (canFloat) return isInLiquid();
    return false;
}

int PathNavigation::getSurfaceY() const {
    if (!mob->isInWater() || !canFloat) return (int)(mob->bb.y0 + 0.5f);

    int y    = (int)mob->bb.y0;
    int stop = y + 17;
    int xt = Mth::floor(mob->x), zt = Mth::floor(mob->z);
    while (isWaterId((unsigned char)level->getTile(xt, y, zt))) {
        if (++y == stop) return (int)mob->bb.y0;
    }
    return y;
}

Vec3 PathNavigation::getTempMobPos() const {
    return Vec3(mob->x, (float)getSurfaceY(), mob->z);
}

bool PathNavigation::adoptPath(float speed) {

    if (!s_candidate.sameAs(path())) path() = s_candidate;
    if (avoidSun) trimPathFromSun();
    if (path().getSize() == 0) return false;
    speedMultiplier  = speed;
    lastProgressTick = tickCounter;
    lastProgressPos  = getTempMobPos();
    return true;
}

bool PathNavigation::moveTo(Mob* target, float speed) {
    if (!canUpdatePath()) return false;
    if (!level->findPath(&s_candidate, mob, target, range, _canOpenDoors, avoidWater)) {
        stop();
        return false;
    }
    return adoptPath(speed);
}

bool PathNavigation::moveTo(float x, float y, float z, float speed) {
    if (!canUpdatePath()) return false;
    if (!level->findPath(&s_candidate, mob, Mth::floor(x), (int)y, Mth::floor(z),
                         range, _canOpenDoors, avoidWater)) {
        stop();
        return false;
    }
    return adoptPath(speed);
}

void PathNavigation::tick() {
    ++tickCounter;
    if (isDone()) return;
    if (canUpdatePath()) updatePath();
    if (isDone()) return;
    Vec3 next = path().currentPos(mob);
    mob->getMoveControl()->setWantedPosition(next.x, next.y, next.z, speedMultiplier);
}

void PathNavigation::trimPathFromSun() {
    int xt = Mth::floor(mob->x), zt = Mth::floor(mob->z);
    if (worldCanSeeSky(level->w, xt, (int)(mob->bb.y0 + 0.5f), zt)) return;
    for (int i = 0; i < path().getSize(); i++) {
        Node* n = path().get(i);
        if (worldCanSeeSky(level->w, n->x, n->y, n->z)) { path().setSize(i - 1); return; }
    }
}

void PathNavigation::updatePath() {
    Vec3 here = getTempMobPos();

    int segEnd = path().getSize();
    for (int i = path().getIndex(); i < path().getSize(); i++) {
        if (path().get(i)->y != (int)here.y) { segEnd = i; break; }
    }

    float reach = mob->bbWidth * mob->bbWidth;
    for (int i = path().getIndex(); i < segEnd; i++) {
        Vec3 p = path().getPos(mob, i);
        if (p.distanceToSqr(here.x, here.y, here.z) < reach) path().setIndex(i + 1);
    }

    int xs = (int)ceilf(mob->bbWidth);
    int ys = (int)mob->bbHeight;
    for (int i = segEnd - 1; i >= path().getIndex(); i--) {
        Vec3 p = path().getPos(mob, i);
        if (canMoveDirectly(here, p, xs, ys + 1, xs)) { path().setIndex(i); break; }
    }

    if (tickCounter - lastProgressTick > 100) {
        if (lastProgressPos.distanceToSqr(here.x, here.y, here.z) < 2.25f) stop();
        lastProgressTick = tickCounter;
        lastProgressPos  = here;
    }
}

bool PathNavigation::canMoveDirectly(const Vec3& from, const Vec3& to,
                                     int xs, int ys, int zs) const {
    float dx = to.x - from.x, dz = to.z - from.z;
    if (dx * dx + dz * dz < 0.000001f) return false;

    float inv = 1.0f / sqrtf(dx * dx + dz * dz);
    float dirX = dx * inv, dirZ = dz * inv;

    int cx = Mth::floor(from.x), cz = Mth::floor(from.z);

    if (!canWalkOn(cx, (int)from.y, cz, xs + 2, ys, zs + 2, from, dirX, dirZ))
        return false;

    float tStepX = 1.0f / fabsf(dirX);
    float tStepZ = 1.0f / fabsf(dirZ);
    float tx = (float)cx - from.x + (dirX >= 0.0f ? 1.0f : 0.0f);
    float tz = (float)cz - from.z + (dirZ >= 0.0f ? 1.0f : 0.0f);
    tx /= dirX;
    tz /= dirZ;
    int stepX = dirX >= 0.0f ? 1 : -1;
    int stepZ = dirZ >= 0.0f ? 1 : -1;
    int endX = Mth::floor(to.x), endZ = Mth::floor(to.z);

    while (stepX * (endX - cx) > 0 || stepZ * (endZ - cz) > 0) {
        if (tx < tz) { cx += stepX; tx += tStepX; }
        else         { cz += stepZ; tz += tStepZ; }
        if (!canWalkOn(cx, (int)from.y, cz, xs, ys, zs, from, dirX, dirZ))
            return false;
    }
    return true;
}

bool PathNavigation::canWalkOn(int x, int y, int z, int xs, int ys, int zs,
                               const Vec3& from, float dirX, float dirZ) const {
    int x0 = x - xs / 2;
    int z0 = z - zs / 2;
    if (!canWalkAbove(x0, y, z0, xs, ys, zs, from, dirX, dirZ)) return false;

    for (int xx = x0; xx < x0 + xs; xx++) {
        for (int zz = z0; zz < z0 + zs; zz++) {

            if (((float)zz + 0.5f - from.z) * dirZ +
                ((float)xx + 0.5f - from.x) * dirX < 0.0f) continue;
            int id = level->getTile(xx, y - 1, zz);
            if (id <= 0) return false;
            unsigned char b = (unsigned char)id;
            if (isWaterId(b) && !mob->isInWater()) return false;
            if (isLavaId(b)) return false;
            if (!isSolidPhys(b) && !isWaterId(b)) return false;
        }
    }
    return true;
}

bool PathNavigation::canWalkAbove(int x, int y, int z, int xs, int ys, int zs,
                                  const Vec3& from, float dirX, float dirZ) const {
    for (int xx = x; xx < x + xs; xx++)
        for (int yy = y; yy < y + ys; yy++)
            for (int zz = z; zz < z + zs; zz++) {
                if (((float)zz + 0.5f - from.z) * dirZ +
                    ((float)xx + 0.5f - from.x) * dirX < 0.0f) continue;
                int id = level->getTile(xx, yy, zz);

                if (id > 0 && isSolidPhys((unsigned char)id) &&
                    !pathIsHole(level, xx, yy, zz, (unsigned char)id))
                    return false;
            }
    return true;
}
