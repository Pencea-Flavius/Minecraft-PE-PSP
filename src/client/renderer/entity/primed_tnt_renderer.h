
#ifndef MCPSP_CLIENT_PRIMED_TNT_RENDERER_H
#define MCPSP_CLIENT_PRIMED_TNT_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class PrimedTntRenderer : public EntityRenderer {
public:
    PrimedTntRenderer() { shadowRadius = 0.4f; }
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
