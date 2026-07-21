#ifndef MCPSP_WORLD_ITEM_INSTANCE_H
#define MCPSP_WORLD_ITEM_INSTANCE_H

class Item;

struct ItemInstance {
    short id;
    short count;
    short data;

    ItemInstance() : id(0), count(0), data(0) {}
    ItemInstance(short id, short count, short data) : id(id), count(count), data(data) {}

    Item* getItem() const;

    bool isNull() const { return id == 0 || count <= 0; }
    void setNull() { id = 0; count = 0; data = 0; }

    bool matches(const ItemInstance* o) const {
        return o && id == o->id && data == o->data;
    }

    int  getMaxStackSize() const;
    bool isStackable() const { return getMaxStackSize() > 1; }

    void hurt(int amount);

    ItemInstance remove(int n);
};

#endif
