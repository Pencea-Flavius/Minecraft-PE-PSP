
#ifndef MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H
#define MCPSP_WORLD_ENTITY_LOCAL_PLAYER_H

#include "world/entity/player.h"

class LocalPlayer : public Player {
public:
    LocalPlayer(Level* level);

    void aiStep(unsigned int btn, unsigned char lx, unsigned char ly);

    bool prevSneakBtn = false;

    virtual void die(Entity* source);
};

#endif
