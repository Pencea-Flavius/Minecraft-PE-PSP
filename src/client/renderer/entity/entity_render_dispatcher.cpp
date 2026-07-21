#include "client/renderer/entity/entity_render_dispatcher.h"
#include "client/renderer/entity/entity_renderer.h"
#include "client/renderer/entity/painting_renderer.h"
#include "client/renderer/entity/arrow_renderer.h"
#include "client/renderer/entity/falling_tile_renderer.h"
#include "client/renderer/entity/primed_tnt_renderer.h"
#include "client/renderer/entity/item_renderer.h"
#include "client/renderer/entity/throwable_renderer.h"
#include "client/renderer/entity/pig_renderer.h"
#include "client/renderer/entity/cow_renderer.h"
#include "client/renderer/entity/chicken_renderer.h"
#include "client/renderer/entity/sheep_renderer.h"
#include "client/renderer/entity/humanoid_renderer.h"
#include "world/item/item.h"
#include "client/renderer/entity/creeper_renderer.h"
#include "client/renderer/entity/spider_renderer.h"
#include "client/renderer/entity/tripod_camera_renderer.h"
#include "world/entity/entity.h"
#include "world/entity/local_player.h"
#include "world/entity/tripod_camera.h"
#include "world/level/level.h"
#include <cstring>

EntityRenderDispatcher::EntityRenderDispatcher() {
    memset(_renderers, 0, sizeof(_renderers));
    assign(ER_PAINTING_RENDERER, new PaintingRenderer());
    assign(ER_TRIPODCAMERA_RENDERER, new TripodCameraRenderer());
    assign(ER_ARROW_RENDERER, new ArrowRenderer());
    assign(ER_FALLINGTILE_RENDERER, new FallingTileRenderer());
    assign(ER_TNT_RENDERER, new PrimedTntRenderer());
    assign(ER_ITEM_RENDERER, new ItemRenderer());

    assign(ER_SNOWBALL_RENDERER, new ThrowableRenderer());
    assign(ER_THROWNEGG_RENDERER, new ThrowableRenderer());
    assign(ER_PIG_RENDERER, new PigRenderer());
    assign(ER_COW_RENDERER, new CowRenderer());
    assign(ER_CHICKEN_RENDERER, new ChickenRenderer());
    assign(ER_SHEEP_RENDERER, new SheepRenderer());

    assign(ER_ZOMBIE_RENDERER,    new HumanoidRenderer("data/images/mob/zombie.png"));
    assign(ER_SKELETON_RENDERER,  new HumanoidRenderer("data/images/mob/skeleton.png", true, true));
    assign(ER_PIGZOMBIE_RENDERER, new HumanoidRenderer("data/images/mob/pigzombie.png",
                                                       false, false, ITEM_SWORD_GOLD));
    assign(ER_CREEPER_RENDERER,   new CreeperRenderer());
    assign(ER_SPIDER_RENDERER,    new SpiderRenderer());
    for (int i = 0; i < MAX_RENDERERS; i++)
        if (_renderers[i]) _renderers[i]->init(this);
}

void EntityRenderDispatcher::assign(EntityRendererId id, EntityRenderer* r) {
    if ((int)id >= 0 && (int)id < MAX_RENDERERS) _renderers[id] = r;
}

EntityRenderDispatcher* EntityRenderDispatcher::getInstance() {
    static EntityRenderDispatcher instance;
    return &instance;
}

EntityRenderer* EntityRenderDispatcher::getRenderer(Entity* entity) {
    EntityRendererId id = entity->entityRendererId;
    if (id == ER_QUERY_RENDERER) id = entity->queryEntityRenderer();
    if ((int)id < 0 || (int)id >= MAX_RENDERERS) return 0;
    return _renderers[id];
}

void EntityRenderDispatcher::render(Entity* e, float a) {
    float x = e->xOld + (e->x - e->xOld) * a;
    float y = e->yOld + (e->y - e->yOld) * a;
    float z = e->zOld + (e->z - e->zOld) * a;
    float r = e->yRotO + (e->yRot - e->yRotO) * a;
    EntityRenderer* renderer = getRenderer(e);
    if (renderer) {
        renderer->render(e, x, y, z, r, a);
        renderer->postRender(e, x, y, z, a);
    }
}

void EntityRenderDispatcher::renderAll(Level* level, float a) {
    if (!level) return;

    extern float g_viewDist;
    const float entDist = (g_viewDist <= 16.0f) ? 16.0f : 32.0f;
    float entDist2 = entDist * entDist;
    for (size_t i = 0; i < level->entities.size(); i++) {
        Entity* e = level->entities[i];
        if (!e || e->removed || e->invisible) continue;

        if (g_photoPending && e == g_photoCamera) continue;

        if (level->player) {
            float dx = e->x - level->player->x, dy = e->y - level->player->y, dz = e->z - level->player->z;
            if (dx * dx + dy * dy + dz * dz > entDist2) continue;
            if (!e->shouldRender(level->player->x, level->player->y, level->player->z)) continue;
        }
        render(e, a);
    }
}
