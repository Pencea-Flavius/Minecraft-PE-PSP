
#ifndef MCPSP_WORLD_ENTITY_TRIPOD_CAMERA_H
#define MCPSP_WORLD_ENTITY_TRIPOD_CAMERA_H

#include "world/entity/mob.h"

class TripodCamera : public Mob {
public:
    TripodCamera(Level* level, float x, float y, float z, float ownerYaw, float ownerPitch);

    virtual void tick();
    virtual bool isPickable() { return !removed; }
    virtual bool isPushable() { return false; }
    virtual int  getEntityTypeId() const { return 0; }
    virtual bool playerInteract();

    virtual const char* getHurtSound()  { return "random.pop"; }
    virtual const char* getDeathSound() { return "random.pop"; }

    int  life;
    bool activated;
};

extern bool  g_photoPending;
extern float g_photoX, g_photoY, g_photoZ, g_photoYaw, g_photoPitch;
extern Entity* g_photoCamera;

#endif
