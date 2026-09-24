
#include "world/entity/ai/move_control.h"
#include "world/entity/ai/jump_control.h"
#include "world/entity/mob.h"
#include "util/mth.h"
#include <cmath>

MoveControl::MoveControl(Mob* mob)
:   mob(mob), wantedX(mob->x), wantedY(mob->y), wantedZ(mob->z),
    speedMultiplier(0.0f), _hasWanted(false) {}

void MoveControl::setWantedPosition(float x, float y, float z, float speed) {
    wantedX = x; wantedY = y; wantedZ = z;
    speedMultiplier = speed;
    _hasWanted = true;
}

void MoveControl::tick() {

    mob->setYya(0.0f);
    if (!_hasWanted) return;
    _hasWanted = false;

    int   feetY = Mth::floor(mob->bb.y0 + 0.5f);
    float dx = wantedX - mob->x;
    float dy = wantedY - (float)feetY;
    float dz = wantedZ - mob->z;
    if (dx * dx + dy * dy + dz * dz < 0.00000025f) return;

    float want = atan2f(dz, dx) * 180.0f / Mth::PI - 90.0f;
    mob->yRot = Mth::clampRotate(mob->yRot, want, (float)Mob::STEER_TURN_RATE);

    mob->setSpeed(speedMultiplier);

    if (dy > 0.0f && (dx * dx + dz * dz) < 1.0f) mob->getJumpControl()->jump();
}
