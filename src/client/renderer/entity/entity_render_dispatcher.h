
#ifndef MCPSP_CLIENT_ENTITY_RENDER_DISPATCHER_H
#define MCPSP_CLIENT_ENTITY_RENDER_DISPATCHER_H

#include "world/entity/entity_renderer_id.h"

class Entity;
class EntityRenderer;
class Level;

class EntityRenderDispatcher {
public:
    static EntityRenderDispatcher* getInstance();

    void renderAll(Level* level, float a);

    void render(Entity* entity, float a);

private:
    EntityRenderDispatcher();
    void assign(EntityRendererId id, EntityRenderer* r);
    EntityRenderer* getRenderer(Entity* entity);

    static const int MAX_RENDERERS = ER_FALLINGTILE_RENDERER + 1;
    EntityRenderer* _renderers[MAX_RENDERERS];
};

#endif
