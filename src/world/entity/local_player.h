
#ifndef MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H
#define MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H

#include "world/entity/player.h"

class LocalPlayer : public Player {
public:
    LocalPlayer(Level* level);

    void aiStep(unsigned int btn, unsigned char lx, unsigned char ly);

    bool prevSneakBtn = false;
    bool prevForward = false;
    int  aiTickCount = 0;
    int  lastForwardTapTick = -1000;

    virtual void die(Entity* source);

    virtual void doWaterSplashEffect();
};

#endif
