
#ifndef MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H
#define MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H

#include "world/entity/player.h"

class LocalPlayer : public Player {
public:
    LocalPlayer(Level* level);

    using Mob::aiStep;

    void aiStep(unsigned int btn, unsigned char lx, unsigned char ly,
                unsigned char rx = 128, unsigned char ry = 128);

    bool prevSneakBtn = false;

    virtual void move(float xa, float ya, float za);

    void checkInTile(float px, float py, float pz);

    int autoJumpTime = 0;

    virtual void die(Entity* source);

    virtual void doWaterSplashEffect();
};

#endif
