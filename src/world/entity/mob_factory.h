
#ifndef MCPSP_WORLD_ENTITY_MOB_FACTORY_H
#define MCPSP_WORLD_ENTITY_MOB_FACTORY_H

class Mob;
class Level;

namespace MobFactory {
    Mob* createMob(int mobType, Level* level);
}

#endif
