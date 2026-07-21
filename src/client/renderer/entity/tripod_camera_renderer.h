
#ifndef MCPSP_CLIENT_RENDERER_TRIPOD_CAMERA_RENDERER_H
#define MCPSP_CLIENT_RENDERER_TRIPOD_CAMERA_RENDERER_H

#include "client/renderer/entity/entity_renderer.h"

class TripodCameraRenderer : public EntityRenderer {
public:
    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
};

#endif
