
#ifndef MCPSP_WORLD_LEVEL_MOB_SPAWNER_H
#define MCPSP_WORLD_LEVEL_MOB_SPAWNER_H

class Level;

namespace MobSpawner {
    void tick(Level* level, bool spawnEnemies, bool spawnFriendlies);
    void populateInitial(Level* level);
}

#endif
