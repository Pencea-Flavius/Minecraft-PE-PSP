#ifndef MCPSP_CLIENT_ENTITY_COW_RENDERER_H
#define MCPSP_CLIENT_ENTITY_COW_RENDERER_H
#include "client/renderer/entity/entity_renderer.h"
class CowRenderer : public EntityRenderer {
public:
    CowRenderer();
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};
#endif
