
#ifndef MCPSP_CLIENT_ENTITY_CREEPER_RENDERER_H
#define MCPSP_CLIENT_ENTITY_CREEPER_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"
#include "gpu/texture.h"

class CreeperRenderer : public EntityRenderer {
public:
    CreeperRenderer();
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);

private:
    Texture tex;
    bool    haveTex;
};

#endif
