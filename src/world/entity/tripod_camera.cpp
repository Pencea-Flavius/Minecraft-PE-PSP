
#include "world/entity/tripod_camera.h"
#include "world/level/level.h"
#include "client/renderer/particle.h"

bool  g_photoPending = false;
float g_photoX, g_photoY, g_photoZ, g_photoYaw, g_photoPitch;
Entity* g_photoCamera = 0;

TripodCamera::TripodCamera(Level* level, float x, float y, float z,
                           float ownerYaw, float ownerPitch)
    : Mob(level), life(80), activated(false) {
    entityRendererId = ER_TRIPODCAMERA_RENDERER;

    xRot = xRotO = ownerPitch;
    yRot = yRotO = ownerYaw;
    blocksBuilding = true;
    setSize(1.0f, 1.5f);
    heightOffset = bbHeight / 2.0f - 0.25f;
    setPos(x, y, z);
    xo = xOld = x; yo = yOld = y; zo = zOld = z;
}

bool TripodCamera::playerInteract() {
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
            remove();
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
