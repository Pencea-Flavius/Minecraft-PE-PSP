#ifndef MCPSP_WORLD_CONTAINER_H
#define MCPSP_WORLD_CONTAINER_H

#include "world/item/item_instance.h"

class Player;

struct ContainerType {
    static const int NONE          = -9;
    static const int INVENTORY     = -1;
    static const int CONTAINER     = 0;
    static const int WORKBENCH     = 1;
    static const int FURNACE       = 2;
};

class Container {
public:
    static const int LARGE_MAX_STACK_SIZE = 64;

    Container(int containerType) : containerId(-1), containerType(containerType) {}
    virtual ~Container() {}

    virtual ItemInstance* getItem(int slot) = 0;
    virtual void          setItem(int slot, ItemInstance* item) = 0;
    virtual ItemInstance  removeItem(int slot, int count) = 0;

    virtual int  getContainerSize() const = 0;
    virtual int  getMaxStackSize() const = 0;

    int containerId;
    int containerType;
};

#endif
