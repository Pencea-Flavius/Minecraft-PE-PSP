
#ifndef MCPSP_CLIENT_PAINTING_RENDERER_H
#define MCPSP_CLIENT_PAINTING_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class PaintingRenderer : public EntityRenderer {
public:
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
