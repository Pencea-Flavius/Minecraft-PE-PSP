
#ifndef MCPSP_CLIENT_FALLING_TILE_RENDERER_H
#define MCPSP_CLIENT_FALLING_TILE_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class FallingTileRenderer : public EntityRenderer {
public:
    FallingTileRenderer() { shadowRadius = 0.4f; }
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
