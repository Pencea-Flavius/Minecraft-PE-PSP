
#ifndef MCPSP_WORLD_ENTITY_TRIPOD_CAMERA_H
#define MCPSP_WORLD_ENTITY_TRIPOD_CAMERA_H

#include "world/entity/mob.h"
#include "world/entity/entity_types.h"

class TripodCamera : public Mob {
public:

    TripodCamera(Level* level, float x, float y, float z, float ownerYaw, float ownerPitch);

    explicit TripodCamera(Level* level);

    virtual void tick();
    virtual bool isPickable() { return !removed; }
    virtual bool isPushable() { return false; }
    virtual int  getEntityTypeId() const { return EntityTypes::IdTripodCamera; }
    virtual bool playerInteract();

    virtual bool hurt(Entity* source, int dmg);
    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    virtual const char* getHurtSound()  { return "random.pop"; }
    virtual const char* getDeathSound() { return "random.pop"; }

    void breakAndDrop();

    int  life;
    bool activated;

private:
    void init();
};

extern bool  g_photoPending;
extern float g_photoX, g_photoY, g_photoZ, g_photoYaw, g_photoPitch;
extern Entity* g_photoCamera;

extern bool g_photoIsIcon;
extern char g_photoIconPath[320];

#endif
