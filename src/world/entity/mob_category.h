
#ifndef MCPSP_WORLD_ENTITY_MOB_CATEGORY_H
#define MCPSP_WORLD_ENTITY_MOB_CATEGORY_H

#include "world/entity/entity_types.h"

namespace MobCategory {
    struct Category { int baseType; int maxPerLevel; bool friendly; bool water; };

    static const Category monster  = { EntityTypes::BaseEnemy,         20, false, false };
    static const Category creature = { EntityTypes::BaseCreature,      15, true,  false };
    static const Category water    = { EntityTypes::BaseWaterCreature, 10, true,  true  };
}

#endif
