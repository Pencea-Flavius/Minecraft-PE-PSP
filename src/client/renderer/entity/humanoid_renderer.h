
#ifndef MCPSP_CLIENT_ENTITY_HUMANOID_RENDERER_H
#define MCPSP_CLIENT_ENTITY_HUMANOID_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"
#include "gpu/texture.h"

class HumanoidRenderer : public EntityRenderer {
public:

    explicit HumanoidRenderer(const char* texPath, bool thin = false, bool bow = false, short holdItem = 0);
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);

private:
    const char* texPath;
    Texture tex;
    bool    haveTex;
    bool    thin;
    bool    bow;
    short   holdItem;
};

#endif
