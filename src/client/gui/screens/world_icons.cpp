
#include <pspgu.h>
#include <cstdio>
#include <cstring>

#include "gpu/texture.h"
#include "client/gui/screens/world_icons.h"
#include "platform/path.h"

static const int ICON_SLOTS = 4;
static struct IconSlot {
    char name[64];
    Texture tex;
    bool have;
    unsigned int used;
} s_icons[ICON_SLOTS];
static unsigned int s_iconClock = 0;

Texture* worldIcon(const char* name) {
    IconSlot* victim = &s_icons[0];
    for (int i = 0; i < ICON_SLOTS; i++) {
        IconSlot* sl = &s_icons[i];
        if (sl->name[0] && std::strcmp(sl->name, name) == 0) {
            sl->used = ++s_iconClock;
            return sl->have ? &sl->tex : 0;
        }

        if (!sl->name[0] || sl->used < victim->used) victim = sl;
    }

    if (victim->have) textureFree(&victim->tex);
    std::strncpy(victim->name, name, sizeof(victim->name) - 1);
    victim->name[sizeof(victim->name) - 1] = '\0';
    victim->used = ++s_iconClock;
    char rel[320];
    std::snprintf(rel, sizeof(rel), "saves/%s/icon.png", name);

    victim->have = textureLoad16Optional(savePath(rel), &victim->tex, GU_PSM_5650);
    return victim->have ? &victim->tex : 0;
}

void worldIconsSetLoaded(bool want) {
    if (want) return;
    for (int i = 0; i < ICON_SLOTS; i++) {
        if (s_icons[i].have) textureFree(&s_icons[i].tex);
        s_icons[i].have = false;
        s_icons[i].name[0] = '\0';
        s_icons[i].used = 0;
    }
    s_iconClock = 0;
}
