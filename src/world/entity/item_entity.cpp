
#include "world/entity/item_entity.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "nbt/compound_tag.h"
#include "util/mth.h"
#include "client/player/player_state.h"
#include "world/inventory/inventory.h"

static const int LIFETIME = 5 * 60 * 20;

ItemEntity::ItemEntity(Level* level)
    : super(level), item(), age(0), throwTime(0),
      bobOffs(sharedRandom.nextFloat() * Mth::PI * 2.0f),
      tickCount(0), health(5), lifeTime(LIFETIME) {
    entityRendererId = ER_ITEM_RENDERER;
    setSize(0.25f, 0.25f);
    heightOffset = bbHeight / 2.0f;
    makeStepSound = false;
}

ItemEntity::ItemEntity(Level* level, float x, float y, float z, const ItemInstance& item)
    : super(level), item(item), age(0), throwTime(0),
      bobOffs(sharedRandom.nextFloat() * Mth::PI * 2.0f),
      tickCount(0), health(5), lifeTime(LIFETIME) {
    entityRendererId = ER_ITEM_RENDERER;
    setSize(0.25f, 0.25f);
    heightOffset = bbHeight / 2.0f;
    setPos(x, y, z);
    xOld = xo = this->x; yOld = yo = this->y; zOld = zo = this->z;

    yRot = sharedRandom.nextFloat() * 360.0f;
    xd = sharedRandom.nextFloat() * 0.2f - 0.1f;
    yd = 0.2f;
    zd = sharedRandom.nextFloat() * 0.2f - 0.1f;
    makeStepSound = false;
}

void ItemEntity::tick() {

    xOld = x; yOld = y; zOld = z;

    super::tick();

    if (throwTime > 0) throwTime--;
    xo = x; yo = y; zo = z;

    yd -= 0.04f;
    if (isLavaId(worldBlock(level->w, Mth::floor(x), Mth::floor(y), Mth::floor(z)))) {
        yd = 0.2f;
        xd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.2f;
        zd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.2f;
        level->playSound(this, "random.fizz", 0.4f, 2.0f + sharedRandom.nextFloat() * 0.4f);
    }
    checkInTile(x, y, z);
    move(xd, yd, zd);

    float friction = onGround ? 0.6f * 0.98f : 0.98f;
    xd *= friction;
    yd *= 0.98f;
    zd *= friction;
    if (onGround) yd *= -0.5f;

    if (throwTime == 0) tryPlayerPickup();

    if (throwTime == 0 && !removed && item.isStackable() && (age % 20) == 0)
        tryMergeNearby();

    tickCount++;
    age++;
    if (age >= lifeTime) remove();
}

void ItemEntity::tryMergeNearby() {
    for (unsigned int i = 0; i < g_level.entities.size(); ++i) {
        Entity* e = g_level.entities[i];
        if (e == this || e->removed || !e->isItemEntity()) continue;
        if (e->getEntityTypeId() != EntityTypes::IdItemEntity) continue;
        ItemEntity* o = (ItemEntity*)e;
        if (o->throwTime > 0) continue;
        if (o->item.id != item.id || o->item.data != item.data) continue;
        if (o->item.count + item.count > item.getMaxStackSize()) continue;
        float dx = o->x - x, dy = o->y - y, dz = o->z - z;
        if (dx * dx + dy * dy + dz * dz > 0.75f * 0.75f) continue;

        ItemEntity* big   = (o->item.count > item.count) ? o : this;
        ItemEntity* small = (big == this) ? o : this;
        big->item.count += small->item.count;
        if (small->age < big->age) big->age = small->age;
        small->remove();
        return;
    }
}

class TakeAnimationEntity : public ItemEntity {
    int life;
    float startX, startY, startZ;
public:
    TakeAnimationEntity(Level* level, ItemEntity* srcItem)
        : ItemEntity(level) {
        item = srcItem->item;
        x = xo = xOld = srcItem->x;
        y = yo = yOld = srcItem->y;
        z = zo = zOld = srcItem->z;
        startX = x;
        startY = y;
        startZ = z;
        bobOffs = srcItem->bobOffs;
        age = srcItem->age;
        noPhysics = true;
        life = 0;
    }

    virtual void tick() override {
        xOld = x; yOld = y; zOld = z;
        xo = x; yo = y; zo = z;

        life++;
        if (life >= 3) {
            remove();
            return;
        }

        float time = (float)life / 3.0f;
        time = time * time;

        x = startX + (g_level.player->x - startX) * time;
        y = startY + ((g_level.player->y - 0.5f) - startY) * time;
        z = startZ + (g_level.player->z - startZ) * time;
        age++;
    }

    virtual int getEntityTypeId() const override { return 0; }
    virtual void addAdditonalSaveData(CompoundTag* tag) override {}
};

void ItemEntity::tryPlayerPickup() {

    if (!g_level.player || g_level.player->health <= 0) return;
    float feet = g_level.player->y - PLAYER_EYE;
    float r    = PLAYER_W * 0.5f + 1.0f;
    if (x >= g_level.player->x - r && x <= g_level.player->x + r &&
        z >= g_level.player->z - r && z <= g_level.player->z + r &&
        y >= feet && y <= feet + PLAYER_H) {

        if (!g_inv.isCreative()) {
            ItemInstance* stack = new ItemInstance(item);
            if (!g_inv.add(stack)) { delete stack; return; }
            g_inv.ensureHotbar(item.id, item.data);
        }

        level->playSound(this, "random.pop", 0.3f,
                         ((sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.7f + 1.0f) * 2.0f);
        level->addEntity(new TakeAnimationEntity(level, this));
        remove();
    }
}

bool ItemEntity::hurt(Entity* , int damage) {
    markHurt();
    health -= damage;
    if (health <= 0) remove();
    return false;
}

void ItemEntity::burn(int dmg) { hurt(0, dmg); }

bool ItemEntity::checkInTile(float px, float py, float pz) {
    int xTile = Mth::floor(px);
    int yTile = Mth::floor(py);
    int zTile = Mth::floor(pz);

    float dx = px - xTile;
    float dy = py - yTile;
    float dz = pz - zTile;

    if (isOpaque(worldBlock(level->w, xTile, yTile, zTile))) {
        bool west  = !isOpaque(worldBlock(level->w, xTile - 1, yTile, zTile));
        bool east  = !isOpaque(worldBlock(level->w, xTile + 1, yTile, zTile));
        bool up    = !isOpaque(worldBlock(level->w, xTile, yTile - 1, zTile));
        bool down  = !isOpaque(worldBlock(level->w, xTile, yTile + 1, zTile));
        bool north = !isOpaque(worldBlock(level->w, xTile, yTile, zTile - 1));
        bool south = !isOpaque(worldBlock(level->w, xTile, yTile, zTile + 1));

        int dir = -1;
        float closest = 9999.0f;
        if (west  && dx < closest)     { closest = dx;     dir = 0; }
        if (east  && 1 - dx < closest) { closest = 1 - dx; dir = 1; }
        if (up    && dy < closest)     { closest = dy;     dir = 2; }
        if (down  && 1 - dy < closest) { closest = 1 - dy; dir = 3; }
        if (north && dz < closest)     { closest = dz;     dir = 4; }
        if (south && 1 - dz < closest) {                   dir = 5; }

        float speed = sharedRandom.nextFloat() * 0.2f + 0.1f;
        if (dir == 0) xd = -speed;
        if (dir == 1) xd = +speed;
        if (dir == 2) yd = -speed;
        if (dir == 3) yd = +speed;
        if (dir == 4) zd = -speed;
        if (dir == 5) zd = +speed;
    }
    return false;
}

void ItemEntity::addAdditonalSaveData(CompoundTag* tag) {
    tag->putShort("Health", (short)(health & 0xff));
    tag->putShort("Age", (short)age);

    tag->putShort("ItemId", (short)item.id);
    tag->putShort("ItemCount", (short)item.count);
    tag->putShort("ItemData", (short)item.data);
}

void ItemEntity::readAdditionalSaveData(CompoundTag* tag) {
    health = tag->getShort("Health") & 0xff;
    age    = tag->getShort("Age");
    item.id    = tag->getShort("ItemId");
    item.count = tag->getShort("ItemCount");
    item.data  = tag->getShort("ItemData");
}

bool ItemEntity::isItemEntity()        { return true; }
int  ItemEntity::getEntityTypeId() const { return EntityTypes::IdItemEntity; }
int  ItemEntity::getLifeTime() const   { return lifeTime; }
