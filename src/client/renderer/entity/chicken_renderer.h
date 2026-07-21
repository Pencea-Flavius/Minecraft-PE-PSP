#ifndef MCPSP_CLIENT_ENTITY_CHICKEN_RENDERER_H
#define MCPSP_CLIENT_ENTITY_CHICKEN_RENDERER_H
#include "client/renderer/entity/entity_renderer.h"
class ChickenRenderer : public EntityRenderer {
public:
    ChickenRenderer();
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};
#endif
