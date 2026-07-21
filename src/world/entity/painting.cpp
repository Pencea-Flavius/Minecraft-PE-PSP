#include "world/entity/painting.h"
#include "world/entity/motive.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"
#include "client/gamemode/gamemode.h"
#include "nbt/compound_tag.h"

Painting::Painting(Level* level) : super(level), motive(MOTIVE_DEFAULT) {
    entityRendererId = ER_PAINTING_RENDERER;
}

Painting::Painting(Level* level, int xTile, int yTile, int zTile, int dir)
    : super(level, xTile, yTile, zTile, dir), motive(MOTIVE_DEFAULT) {
    setRandomMotive(dir);
    entityRendererId = ER_PAINTING_RENDERER;
}

Painting::Painting(Level* level, int x, int y, int z, int dir, const char* motiveName)
    : super(level, x, y, z, dir), motive(MOTIVE_DEFAULT) {
    int m = motiveByName(motiveName);
    motive = (m < 0) ? MOTIVE_DEFAULT : m;
    setDir(dir);
    entityRendererId = ER_PAINTING_RENDERER;
}

void Painting::setRandomMotive(int dir) {
    int survivable[MOTIVE_COUNT];
    int n = 0;
    for (int i = 0; i < MOTIVE_COUNT; i++) {
        if (!kMotives[i].isPublic) continue;
        motive = i;
        setDir(dir);
        if (survives()) survivable[n++] = i;
    }
    if (n > 0) {
        motive = survivable[sharedRandom.nextInt(n)];
        setDir(dir);
    } else {
        motive = MOTIVE_DEFAULT;
        setDir(dir);
    }
}

void Painting::addAdditonalSaveData(CompoundTag* tag) {
    tag->putString("Motive", kMotives[motive].name);
    super::addAdditonalSaveData(tag);
}

void Painting::readAdditionalSaveData(CompoundTag* tag) {
    int m = motiveByName(tag->getString("Motive").c_str());
    motive = (m < 0) ? MOTIVE_DEFAULT : m;
    super::readAdditionalSaveData(tag);
}

int Painting::getWidth()  { return kMotives[motive].w; }
int Painting::getHeight() { return kMotives[motive].h; }

void Painting::dropItem() {

    if (g_gameMode && g_gameMode->isCreative()) return;
    spawnAtLocation(ITEM_PAINTING, 1);
}

int  Painting::getEntityTypeId() const { return EntityTypes::IdPainting; }
bool Painting::isPickable() { return true; }
