
#include "world/item/hanging_entity_item.h"
#include "world/item/item_instance.h"
#include "world/entity/hanging_entity.h"
#include "world/entity/painting.h"
#include "world/entity/entity_types.h"
#include "world/direction.h"
#include "world/level/level.h"

extern Level g_level;

static const int kPortFaceToFacing[6] = {
    Facing::WEST, Facing::EAST, Facing::DOWN, Facing::UP, Facing::NORTH, Facing::SOUTH
};

HangingEntityItem::HangingEntityItem(short id, int entityType, int icon)
    : Item(id), entityType(entityType), icon(icon) {
    maxStackSize = 64;
}

HangingEntity* HangingEntityItem::createEntity(Level* level, int x, int y, int z, int dir) {
    switch (entityType) {
        case EntityTypes::IdPainting: return new Painting(level, x, y, z, dir);
    }
    return 0;
}

bool HangingEntityItem::useOn(ItemInstance* item, Player* , World* ,
                              int x, int y, int z, int face) {
    int facing = (face >= 0 && face < 6) ? kPortFaceToFacing[face] : Facing::DOWN;
    if (facing == Facing::DOWN) return false;
    if (facing == Facing::UP)   return false;

    int dir = Direction::FACING_DIRECTION[facing];

    HangingEntity* entity = createEntity(&g_level, x, y, z, dir);
    if (entity && entity->survives()) {
        g_level.addEntity(entity);

        return true;
    }
    if (entity) delete entity;
    return false;
}
