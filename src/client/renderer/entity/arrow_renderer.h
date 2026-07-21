
#ifndef MCPSP_CLIENT_ARROW_RENDERER_H
#define MCPSP_CLIENT_ARROW_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class ArrowRenderer : public EntityRenderer {
public:
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
