#ifndef MCPSP_CLIENT_ENTITY_SHEEP_RENDERER_H
#define MCPSP_CLIENT_ENTITY_SHEEP_RENDERER_H
#include "client/renderer/entity/entity_renderer.h"
class SheepRenderer : public EntityRenderer {
public:
    SheepRenderer();
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};
#endif
