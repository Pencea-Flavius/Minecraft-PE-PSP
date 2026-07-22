
#include "client/renderer/entity/item_renderer.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/item_model.h"
#include "world/entity/item_entity.h"
#include "world/item/item.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "client/player/player_state.h"
#include "client/renderer/render.h"
#include "gpu/texture.h"
#include "util/mth.h"
#include "world/level/levelgen/Random.h"
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

extern bool    g_haveTerrain;
extern Texture g_terrain;
extern World   g_world;

static unsigned int dropLight(float x, float y, float z) {
    int br = lightRawAt(&g_world, Mth::floor(x), Mth::floor(y), Mth::floor(z));
    return g_brightColor[br];
}

void ItemRenderer::render(Entity* entity, float x, float y, float z, float , float a) {
    ItemEntity* ie = (ItemEntity*)entity;
    ItemInstance* it = &ie->item;
    if (it->isNull()) return;

    short id = it->id;
    unsigned char data = (unsigned char)it->data;
    float bob = sinf((ie->age + a) / 10.0f + ie->bobOffs) * 0.1f + 0.1f;

    int count = 1;
    if      (it->count > 20) count = 4;
    else if (it->count > 5)  count = 3;
    else if (it->count > 1)  count = 2;
    Random pileRnd(187);

    sceGuDisable(GU_CULL_FACE);

    if (!itemIsFlat2D(id)) {

        if (!g_haveTerrain) return;
        static ItemModelRenderer model;
        if (!model.build(id, data)) return;

        float spin = (ie->age + a) / 20.0f + ie->bobOffs;

        sceGumMatrixMode(GU_MODEL);
        sceGumPushMatrix();
        sceGumLoadIdentity();
        ScePspFVector3 tr = { x - g_relBaseX, y + bob - g_relBaseY, z - g_relBaseZ };
        sceGumTranslate(&tr);
        sceGumRotateY(spin);
        const float s = 0.25f;
        ScePspFVector3 sc = { s, s, s };
        sceGumScale(&sc);

        if (!isPlant(id)) { sceGuEnable(GU_CULL_FACE); sceGuFrontFace(GU_CCW); }
        unsigned int lc = dropLight(x, y, z);
        for (int i = 0; i < count; ++i) {
            sceGumPushMatrix();
            if (i > 0) {

                ScePspFVector3 off = { (pileRnd.nextFloat() * 2 - 1) * 0.2f / s,
                                       (pileRnd.nextFloat() * 2 - 1) * 0.2f / s,
                                       (pileRnd.nextFloat() * 2 - 1) * 0.2f / s };
                sceGumTranslate(&off);
            }
            ScePspFVector3 center = { -0.5f, -150.5f, -0.5f };
            sceGumTranslate(&center);

            model.draw(lc, true);
            sceGumPopMatrix();
        }
        sceGuDisable(GU_CULL_FACE);
        sceGumPopMatrix();
    } else {

        float u0, v0, u1, v1;
        const Texture* tex = itemFlatIconUV(id, data, &u0, &v0, &u1, &v1);
        if (!tex) return;

        const unsigned int c = dropLight(x, y, z);
        const float xo = 0.5f, yo = 0.25f, r = 1.0f;
        ChunkVertex q[6] = {
            { u0, v1, c, 0 - xo, 0 - yo, 0.0f },
            { u1, v1, c, r - xo, 0 - yo, 0.0f },
            { u1, v0, c, r - xo, 1 - yo, 0.0f },
            { u1, v0, c, r - xo, 1 - yo, 0.0f },
            { u0, v0, c, 0 - xo, 1 - yo, 0.0f },
            { u0, v1, c, 0 - xo, 0 - yo, 0.0f },
        };

        sceGumMatrixMode(GU_MODEL);
        sceGumPushMatrix();
        sceGumLoadIdentity();
        ScePspFVector3 tr = { x - g_relBaseX, y + bob - g_relBaseY, z - g_relBaseZ };
        sceGumTranslate(&tr);
        const float s = 0.5f;
        ScePspFVector3 sc = { s, s, s };
        sceGumScale(&sc);
        float bill = atan2f(g_camX - x, g_camZ - z);

        for (int i = 0; i < count; ++i) {
            sceGumPushMatrix();
            if (i > 0) {

                ScePspFVector3 off = { (pileRnd.nextFloat() * 2 - 1) * 0.3f,
                                       (pileRnd.nextFloat() * 2 - 1) * 0.3f,
                                       (pileRnd.nextFloat() * 2 - 1) * 0.3f };
                sceGumTranslate(&off);
            }

            sceGumRotateY(bill);
            ItemModelRenderer::drawMesh(q, 6, 0xFFFFFFFFu, tex, true);
            sceGumPopMatrix();
        }
        sceGumPopMatrix();
    }

    sceGuEnable(GU_CULL_FACE);
}
