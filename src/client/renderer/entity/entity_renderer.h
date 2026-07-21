
#ifndef MCPSP_CLIENT_ENTITY_RENDERER_H
#define MCPSP_CLIENT_ENTITY_RENDERER_H

extern float g_relBaseX, g_relBaseY, g_relBaseZ;

class Entity;
class EntityRenderDispatcher;

void renderEntityShadow(float x, float y, float z, float off, float radius, float pow);

class EntityRenderer {
public:
    virtual ~EntityRenderer() {}

    virtual void render(Entity* entity, float x, float y, float z, float rot, float a) = 0;
    virtual void init(EntityRenderDispatcher* ) {}

    void postRender(Entity* entity, float x, float y, float z, float a);

protected:

    float shadowRadius;
    float shadowStrength;
    EntityRenderer() : shadowRadius(0.0f), shadowStrength(1.0f) {}
};

#endif
