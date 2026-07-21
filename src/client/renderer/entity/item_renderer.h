
#ifndef MCPSP_CLIENT_ITEM_RENDERER_H
#define MCPSP_CLIENT_ITEM_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class ItemRenderer : public EntityRenderer {
public:
    ItemRenderer() { shadowRadius = 0.15f; }
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
