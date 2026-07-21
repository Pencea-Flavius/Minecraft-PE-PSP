
#ifndef MCPSP_WORLD_ENTITY_FACTORY_H
#define MCPSP_WORLD_ENTITY_FACTORY_H

class Entity;
class Level;
class CompoundTag;

namespace EntityFactory {
    Entity* createEntity(int typeId, Level* level);
    Entity* loadEntity(CompoundTag* tag, Level* level);
}

#endif
