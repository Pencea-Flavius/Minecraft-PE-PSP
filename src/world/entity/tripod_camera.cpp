
#include "world/entity/tripod_camera.h"
#include "world/entity/player.h"
#include "world/level/level.h"
#include "world/inventory/inventory.h"
#include "world/item/item.h"
#include "world/item/item_instance.h"
#include "client/renderer/particle.h"
#include "nbt/compound_tag.h"

bool  g_photoPending = false;
float g_photoX, g_photoY, g_photoZ, g_photoYaw, g_photoPitch;
Entity* g_photoCamera = 0;
bool  g_photoIsIcon = false;
char  g_photoIconPath[320] = {0};

void TripodCamera::init() {
    entityRendererId = ER_TRIPODCAMERA_RENDERER;
    blocksBuilding = true;
    setSize(1.0f, 1.5f);
    heightOffset = bbHeight / 2.0f - 0.25f;
}

TripodCamera::TripodCamera(Level* level, float x, float y, float z,
                           float ownerYaw, float ownerPitch)
    : Mob(level), life(80), activated(false) {
    init();

    xRot = xRotO = ownerPitch;
    yRot = yRotO = ownerYaw;
    setPos(x, y, z);
    xo = xOld = x; yo = yOld = y; zo = zOld = z;
}

TripodCamera::TripodCamera(Level* level)
    : Mob(level), life(80), activated(false) {
    init();
}

void TripodCamera::addAdditonalSaveData(CompoundTag* tag) {
    Mob::addAdditonalSaveData(tag);

    tag->putShort("CameraLife", (short)life);
    tag->putBoolean("CameraActivated", activated);
}

void TripodCamera::readAdditionalSaveData(CompoundTag* tag) {
    Mob::readAdditionalSaveData(tag);
    life = tag->getShort("CameraLife");
    activated = tag->getBoolean("CameraActivated");

    if (life <= 0 || life > 80) life = 80;
}

void TripodCamera::breakAndDrop() {
    if (removed) return;

    Player* p = (Player*)level->player;
    bool creative = p && p->inventory && p->inventory->isCreative();
    if (!creative) spawnAtLocation(ITEM_CAMERA, 1);
    remove();
}

bool TripodCamera::hurt(Entity* source, int ) {
    if (removed || !source) return false;
    level->playSound(this, getHurtSound(), 1.0f, 1.0f);
    breakAndDrop();
    return true;
}

bool TripodCamera::playerInteract() {
    if (activated) return false;
    Player* p = (Player*)level->player;
    if (!p || !p->inventory) return false;
    if (!p->inventory->isCreative()) {
        ItemInstance* sel = p->inventory->getSelected();
        if (!sel || sel->isNull() || sel->id != ITEM_PAPER) return false;
        p->inventory->consumeSelected();
    }
    activated = true;
    return true;
}

void TripodCamera::tick() {

    xo = xOld = x; yo = yOld = y; zo = zOld = z;

    yd -= 0.04f;
    move(xd, yd, zd);
    xd *= 0.98f;
    yd *= 0.98f;
    zd *= 0.98f;
    if (onGround) {
        xd *= 0.7f;
        zd *= 0.7f;
        yd *= -0.5f;
    }

    if (activated) {
        --life;
        if (life == 0) {

            breakAndDrop();
        } else if (life == 8) {

            g_photoPending = true;
            g_photoCamera  = this;
            g_photoX = x;
            g_photoY = (y - heightOffset) + 18.0f / 16.0f;
            g_photoZ = z;
            g_photoYaw = yRot; g_photoPitch = xRot;

            particlesLargeSmoke(x, y + 0.9f, z);
            particlesLargeSmoke(x, y + 1.05f, z);
            particlesLargeSmoke(x, y + 1.2f, z);
        } else if (life > 8) {
            particlesSmoke(x, y + 0.95f, z);
        }
    }
}
