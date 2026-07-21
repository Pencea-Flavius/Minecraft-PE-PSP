
#ifndef MCPSP_CLIENT_ENTITY_PIG_RENDERER_H
#define MCPSP_CLIENT_ENTITY_PIG_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class PigRenderer : public EntityRenderer {
public:
    PigRenderer();
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
