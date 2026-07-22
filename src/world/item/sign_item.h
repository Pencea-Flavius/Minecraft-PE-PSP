
#ifndef MCPSP_WORLD_ITEM_SIGN_ITEM_H
#define MCPSP_WORLD_ITEM_SIGN_ITEM_H

#include "world/item/item.h"

class SignItem : public Item {
public:
    SignItem(short id, int icon);
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float);
    virtual int  getIcon(short data) const { return icon; }
private:
    int icon;
};

#endif
