
#include "world/level/tile/tile.h"
#include "world/entity/entity.h"
#include <cstdlib>
#include "world/entity/item_entity.h"
#include "world/item/item_instance.h"
#include "world/level/level.h"
#include "world/level/world.h"
extern World g_world;
#include "world/level/world.h"
#include "nbt/compound_tag.h"
#include "util/mth.h"
#include <cmath>
#include <pspkernel.h>

int Entity::entityCounter = 0;
Random Entity::sharedRandom((long)sceKernelGetSystemTimeLow());

static unsigned char s_pool[Entity::ENTITY_POOL][Entity::ENTITY_SLOT];
static bool          s_slotUsed[Entity::ENTITY_POOL];

bool Entity::hasFreeSlot() {
    for (int i = 0; i < ENTITY_POOL; i++) if (!s_slotUsed[i]) return true;
    return false;
}

void* Entity::operator new(unsigned n) {
    if (n <= ENTITY_SLOT) {
        for (int i = 0; i < ENTITY_POOL; i++)
            if (!s_slotUsed[i]) { s_slotUsed[i] = true; return s_pool[i]; }
    }
    return malloc(n);
}

void Entity::operator delete(void* p) {
    if (!p) return;
    unsigned char* c = (unsigned char*)p;
    if (c >= s_pool[0] && c < s_pool[0] + sizeof(s_pool)) {
        s_slotUsed[(c - s_pool[0]) / ENTITY_SLOT] = false;
        return;
    }
    free(p);
}

Entity::Entity(Level* level)
:   x(0), y(0), z(0),
    xChunk(0), yChunk(0), zChunk(0),
    viewScale(1.0f),
    level(level),
    xo(0), yo(0), zo(0), xd(0), yd(0), zd(0),
    yRot(0), xRot(0), yRotO(0), xRotO(0),
    bb(0,0,0,0,0,0),
    heightOffset(0.0f),
    bbWidth(0.6f), bbHeight(1.8f),
    walkDistO(0), walkDist(0),
    xOld(0), yOld(0), zOld(0),
    ySlideOffset(0), footSize(0), pushthrough(0),
    tickCount(0), invulnerableTime(0),
    airSupply(TOTAL_AIR_SUPPLY), onFire(0), flameTime(1),
    entityRendererId(ER_DEFAULT_RENDERER),
    fallDistance(0), blocksBuilding(false), inChunk(false),
    onGround(false),
    horizontalCollision(false), verticalCollision(false),
    collision(false), hurtMarked(false),
    slide(true), removed(false), noPhysics(false),
    canRemove(true), invisible(false), reallyRemoveIfPlayer(false),
    airCapacity(TOTAL_AIR_SUPPLY),
    makeStepSound(true), wasInWater(false), fireImmune(false),
    firstTick(true), nextStep(1), isStuckInWeb(false)
{
    _init();
    entityId = ++entityCounter;
    setPos(0, 0, 0);
}

Entity::~Entity() {}

SynchedEntityData* Entity::getEntityData() { return 0; }

bool Entity::isInWall() {
    int xt = Mth::floor(x);
    int yt = Mth::floor(y + getHeadHeight());
    int zt = Mth::floor(z);
    return level->isSolidBlockingTile(xt, yt, zt);
}

void Entity::resetPos(bool ) {
    if (level == 0) return;
    while (y > 0) {
        setPos(x, y, z);
        if (level->getCubes(this, bb).size() == 0) break;
        y += 1;
    }
    xd = yd = zd = 0;
    xRot = 0;
}

bool Entity::isInWater() { return level->isInWater(this); }
bool Entity::isInLava()  { return level->isInLava(this); }

bool Entity::isFree(float xa, float ya, float za, float grow) {
    AABB box = bb.grow(grow, grow, grow).cloneMove(xa, ya, za);
    if (level->getCubes(this, box).size() > 0) return false;
    if (level->containsAnyLiquid(box)) return false;
    return true;
}
bool Entity::isFree(float xa, float ya, float za) {
    AABB box = bb.cloneMove(xa, ya, za);
    if (level->getCubes(this, box).size() > 0) return false;
    if (level->containsAnyLiquid(box)) return false;
    return true;
}

void Entity::move(float xa, float ya, float za) {
    if (noPhysics) {
        bb.move(xa, ya, za);
        x = (bb.x0 + bb.x1) / 2.0f;
        y = bb.y0 + heightOffset - ySlideOffset;
        z = (bb.z0 + bb.z1) / 2.0f;
        return;
    }

    float xOrigin = x;
    float zOrigin = z;

    if (isStuckInWeb) {
        isStuckInWeb = false;
        xa *= .25f; ya *= .05f; za *= .25f;
        xd = yd = zd = 0.0f;
    }

    float xaOrg = xa, yaOrg = ya, zaOrg = za;
    AABB bbOrg = bb;
    bool sneaking = onGround && isSneaking();

    if (sneaking) {
        float d = 0.05f;
        while (xa != 0 && level->getCubes(this, bb.cloneMove(xa, -1.0f, 0)).empty()) {
            if (xa < d && xa >= -d) xa = 0; else if (xa > 0) xa -= d; else xa += d;
            xaOrg = xa;
        }
        while (za != 0 && level->getCubes(this, bb.cloneMove(0, -1.0f, za)).empty()) {
            if (za < d && za >= -d) za = 0; else if (za > 0) za -= d; else za += d;
            zaOrg = za;
        }
        while (xa != 0 && za != 0 && level->getCubes(this, bb.cloneMove(xa, -1.0f, za)).empty()) {
            if (xa < d && xa >= -d) xa = 0; else if (xa > 0) xa -= d; else xa += d;
            if (za < d && za >= -d) za = 0; else if (za > 0) za -= d; else za += d;
            xaOrg = xa; zaOrg = za;
        }
    }

    std::vector<AABB> aABBs = level->getCubes(this, bb.expand(xa, ya, za));

    for (unsigned int i = 0; i < aABBs.size(); i++) ya = aABBs[i].clipYCollide(bb, ya);
    bb.move(0, ya, 0);
    if (!slide && yaOrg != ya) { xa = ya = za = 0; }
    bool og = onGround || (yaOrg != ya && yaOrg < 0);

    for (unsigned int i = 0; i < aABBs.size(); i++) xa = aABBs[i].clipXCollide(bb, xa);
    bb.move(xa, 0, 0);
    if (!slide && xaOrg != xa) { xa = ya = za = 0; }

    for (unsigned int i = 0; i < aABBs.size(); i++) za = aABBs[i].clipZCollide(bb, za);
    bb.move(0, 0, za);
    if (!slide && zaOrg != za) { xa = ya = za = 0; }

    if (footSize > 0 && og && ySlideOffset < 0.05f && (xaOrg != xa || zaOrg != za)) {
        float xaN = xa, yaN = ya, zaN = za;
        xa = xaOrg; ya = footSize; za = zaOrg;
        AABB normal = bb;
        bb.set(bbOrg);
        aABBs = level->getCubes(this, bb.expand(xa, ya, za));
        for (unsigned int i = 0; i < aABBs.size(); i++) ya = aABBs[i].clipYCollide(bb, ya);
        bb.move(0, ya, 0);
        if (!slide && yaOrg != ya) { xa = ya = za = 0; }
        for (unsigned int i = 0; i < aABBs.size(); i++) xa = aABBs[i].clipXCollide(bb, xa);
        bb.move(xa, 0, 0);
        if (!slide && xaOrg != xa) { xa = ya = za = 0; }
        for (unsigned int i = 0; i < aABBs.size(); i++) za = aABBs[i].clipZCollide(bb, za);
        bb.move(0, 0, za);
        if (!slide && zaOrg != za) { xa = ya = za = 0; }
        if (xaN * xaN + zaN * zaN >= xa * xa + za * za) {
            xa = xaN; ya = yaN; za = zaN; bb.set(normal);
        } else {
            ySlideOffset += 0.5f;
        }
    }

    x = (bb.x0 + bb.x1) / 2.0f;
    y = bb.y0 + heightOffset - ySlideOffset;
    z = (bb.z0 + bb.z1) / 2.0f;

    horizontalCollision = (xaOrg != xa) || (zaOrg != za);
    verticalCollision = (yaOrg != ya);
    onGround = yaOrg != ya && yaOrg < 0;
    collision = horizontalCollision || verticalCollision;
    checkFallDamage(ya, onGround);

    if (xaOrg != xa) xd = 0;
    if (yaOrg != ya) yd = 0;
    if (zaOrg != za) zd = 0;

    int bx0 = Mth::floor(bb.x0), by0 = Mth::floor(bb.y0), bz0 = Mth::floor(bb.z0);
    int bx1 = Mth::floor(bb.x1), by1 = Mth::floor(bb.y1), bz1 = Mth::floor(bb.z1);
    if (level->hasChunksAt(bx0, by0, bz0, bx1, by1, bz1)) {
        for (int ix = bx0; ix <= bx1; ix++)
            for (int iy = by0; iy <= by1; iy++)
                for (int iz = bz0; iz <= bz1; iz++) {
                    int t = level->getTile(ix, iy, iz);
                    if (t > 0) Tile::tiles[t]->entityInside(&g_world, ix, iy, iz, this);
                }
    }

    ySlideOffset *= 0.4f;

    bool water = isInWater();
    if (level->containsFireTile(bb)) {
        burn(1);
        if (!water) { onFire++; if (onFire == 0) onFire = 20 * 15; }
    } else {
        if (onFire <= 0) onFire = -flameTime;
    }
    if (water && onFire > 0) onFire = -flameTime;
}

void Entity::makeStuckInWeb() { isStuckInWeb = true; fallDistance = 0; }

void Entity::setPos(float nx, float ny, float nz) {
    x = nx; y = ny; z = nz;
    float w = bbWidth / 2;
    float h = bbHeight;
    bb.set(x - w, y - heightOffset + ySlideOffset, z - w,
           x + w, y - heightOffset + ySlideOffset + h, z + w);
}

float Entity::getBrightness(float ) {
    int xTile = Mth::floor(x);
    float hh = (bb.y1 - bb.y0) * 0.66f;
    int yTile = Mth::floor(y - heightOffset + hh);
    int zTile = Mth::floor(z);
    if (level->hasChunksAt(Mth::floor(bb.x0), Mth::floor(bb.y0), Mth::floor(bb.z0),
                           Mth::floor(bb.x1), Mth::floor(bb.y1), Mth::floor(bb.z1))) {
        return level->getBrightness(xTile, yTile, zTile);
    }
    return 0;
}

bool Entity::operator==(Entity& rhs) { return entityId == rhs.entityId; }
int  Entity::hashCode() { return entityId; }
void Entity::remove() { removed = true; }
void Entity::setSize(float w, float h) { bbWidth = w; bbHeight = h; }
void Entity::setRot(float yr, float xr) { yRot = yRotO = yr; xRot = xRotO = xr; }

void Entity::turn(float xo, float yo) {
    float xRotOld = xRot, yRotOld = yRot;
    yRot += xo * 0.15f;
    xRot -= yo * 0.15f;
    if (xRot < -90) xRot = -90;
    if (xRot > 90) xRot = 90;
    xRotO += xRot - xRotOld;
    yRotO += yRot - yRotOld;
}
void Entity::interpolateTurn(float xo, float yo) {
    yRot += xo * 0.15f;
    xRot -= yo * 0.15f;
    if (xRot < -90) xRot = -90;
    if (xRot > 90) xRot = 90;
}

void Entity::tick() { baseTick(); }

void Entity::baseTick() {
    tickCount++;
    walkDistO = walkDist;
    xo = x; yo = y; zo = z;
    xRotO = xRot; yRotO = yRot;

    if (isInWater()) {
        if (!wasInWater && !firstTick) {
            float speed = sqrtf(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f) * 0.2f;
            if (speed > 1) speed = 1;
            level->playSound(this, "random.splash", speed,
                             1 + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.4f);
        }
        fallDistance = 0;
        wasInWater = true;
        onFire = 0;
    } else {
        wasInWater = false;
    }

    if (level->isClientSide) {
        onFire = 0;
    } else {
        if (onFire > 0) {
            if (fireImmune) {
                onFire -= 4; if (onFire < 0) onFire = 0;
            } else {
                if (onFire % 20 == 0) hurt(0, 1);
                onFire--;
            }
        }
    }

    if (isInLava()) lavaHurt();
    if (y < -64) outOfWorld();
    firstTick = false;
}

void Entity::outOfWorld() { remove(); }

void Entity::checkFallDamage(float ya, bool onGround) {
    if (onGround) {
        if (fallDistance > 0) {
            if (isMob()) {
                int xt = Mth::floor(x);
                int yt = Mth::floor(y - 0.2f - heightOffset);
                int zt = Mth::floor(z);
                int t = level->getTile(xt, yt, zt);
                if (t > 0) level->handleFallOn(xt, yt, zt, this, fallDistance);
            }
            causeFallDamage(fallDistance);
            fallDistance = 0;
        }
    } else {
        if (ya < 0) fallDistance -= ya;
    }
}
void Entity::causeFallDamage(float) {}

float Entity::getHeadHeight() { return 0; }

void Entity::moveRelative(float xa, float za, float speed) {
    float dist = sqrtf(xa * xa + za * za);
    if (dist < 0.01f) return;
    if (dist < 1) dist = 1;
    dist = speed / dist;
    xa *= dist; za *= dist;
    float s = sinf(yRot * Mth::PI / 180);
    float c = cosf(yRot * Mth::PI / 180);
    xd += xa * c - za * s;
    zd += za * c + xa * s;
}

void Entity::setLevel(Level* l) { level = l; }

void Entity::moveTo(float nx, float ny, float nz, float yr, float xr) {
    xOld = xo = x = nx;
    yOld = yo = y = ny + heightOffset;
    zOld = zo = z = nz;
    yRot = yRotO = yr;
    xRot = xRotO = xr;
    setPos(x, y, z);
}

float Entity::distanceTo(Entity* e) { return distanceTo(e->x, e->y, e->z); }
float Entity::distanceTo(float x2, float y2, float z2) {
    float dx = x - x2, dy = y - y2, dz = z - z2;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}
float Entity::distanceToSqr(float x2, float y2, float z2) {
    float dx = x - x2, dy = y - y2, dz = z - z2;
    return dx * dx + dy * dy + dz * dz;
}
float Entity::distanceToSqr(Entity* e) { return distanceToSqr(e->x, e->y, e->z); }

void Entity::push(Entity* e) {
    float xa = e->x - x, za = e->z - z;
    float dd = Mth::absMax(xa, za);
    if (dd >= 0.01f) {
        dd = sqrtf(dd);
        xa /= dd; za /= dd;
        float p = 1 / dd; if (p > 1) p = 1;
        xa *= p * 0.05f * (1 - pushthrough);
        za *= p * 0.05f * (1 - pushthrough);
        this->push(-xa, 0, -za);
        e->push(xa, 0, za);
    }
}
void Entity::push(float xa, float ya, float za) { xd += xa; yd += ya; zd += za; }

void Entity::markHurt() { hurtMarked = true; }
bool Entity::hurt(Entity*, int) { markHurt(); return false; }

void Entity::reset() { _init(); }
void Entity::_init() {
    xo = xOld = x; yo = yOld = y; zo = zOld = z;
    xRotO = xRot; yRotO = yRot;
    onFire = 0; removed = false; fallDistance = 0;
}

bool Entity::intersects(float x0, float y0, float z0, float x1, float y1, float z1) {
    return bb.intersects(x0, y0, z0, x1, y1, z1);
}

bool Entity::isPickable()  { return false; }
bool Entity::isPushable()  { return false; }
bool Entity::isShootable() { return false; }
void Entity::awardKillScore(Entity*, int) {}

bool Entity::shouldRender(float cx, float cy, float cz) {
    if (invisible) return false;
    float dx = x - cx, dy = y - cy, dz = z - cz;
    return shouldRenderAtSqrDistance(dx * dx + dy * dy + dz * dz);
}
bool Entity::shouldRenderAtSqrDistance(float distance) {
    float size = bb.getSize() * 64.0f * viewScale;
    return distance < size * size;
}

bool  Entity::isCreativeModeAllowed() { return false; }
float Entity::getShadowHeightOffs() { return bbHeight / 2; }
bool  Entity::isAlive() { return !removed; }
bool  Entity::interact() { return false; }

void  Entity::lerpTo(float x, float y, float z, float yr, float xr, int) {
    setPos(x, y, z); setRot(yr, xr);
}
float Entity::getPickRadius() { return 0.1f; }
void  Entity::lerpMotion(float nxd, float nyd, float nzd) { xd = nxd; yd = nyd; zd = nzd; }
void  Entity::animateHurt() {}
void  Entity::setEquippedSlot(int, int, int) {}
bool  Entity::isSneaking() { return false; }
bool  Entity::isPlayer() { return false; }

void Entity::lavaHurt() {
    if (!fireImmune) { hurt(0, 4); onFire = 30 * TicksPerSecond; }
}
void Entity::burn(int dmg) { if (!fireImmune) hurt(0, dmg); }

bool Entity::save(CompoundTag* tag) {
    int id = getEntityTypeId();
    if (removed || id == 0) return false;
    tag->putInt("id", id);
    saveWithoutId(tag);
    return true;
}
void Entity::saveWithoutId(CompoundTag* tag) {
    tag->put("Pos", floatList(x, y, z));
    tag->put("Motion", floatList(xd, yd, zd));
    tag->put("Rotation", floatList(yRot, xRot));
    tag->putFloat("FallDistance", fallDistance);
    tag->putShort("Fire", (short)onFire);
    tag->putShort("Air", (short)airSupply);
    tag->putBoolean("OnGround", onGround);
    addAdditonalSaveData(tag);
}
bool Entity::load(CompoundTag* tag) {
    ListTag* pos = tag->getList("Pos");
    ListTag* motion = tag->getList("Motion");
    ListTag* rotation = tag->getList("Rotation");
    setPos(0, 0, 0);

    xd = motion->getFloat(0);
    yd = motion->getFloat(1);
    zd = motion->getFloat(2);
    if (Mth::abs(xd) > 10.0f) xd = 0;
    if (Mth::abs(yd) > 10.0f) yd = 0;
    if (Mth::abs(zd) > 10.0f) zd = 0;

    float xx = pos->getFloat(0), yy = pos->getFloat(1), zz = pos->getFloat(2);
    const float padding = bbWidth * 0.5f + 0.001f;
    xx = Mth::clamp(xx, padding, (float)WORLD_W - padding);
    zz = Mth::clamp(zz, padding, (float)WORLD_D - padding);

    xo = xOld = x = xx;
    yo = yOld = y = yy;
    zo = zOld = z = zz;
    yRotO = yRot = fmodf(rotation->getFloat(0), 360.0f);
    xRotO = xRot = fmodf(rotation->getFloat(1), 360.0f);

    fallDistance = tag->getFloat("FallDistance");
    onFire = tag->getShort("Fire");
    airSupply = tag->getShort("Air");
    onGround = tag->getBoolean("OnGround");

    setPos(x, y, z);
    readAdditionalSaveData(tag);
    return (tag->errorState == 0);
}

ItemEntity* Entity::spawnAtLocation(int resource, int count) {
    return spawnAtLocation(resource, count, 0);
}
ItemEntity* Entity::spawnAtLocation(int resource, int count, float yOffs) {
    if (resource <= 0 || count <= 0) return 0;
    ItemEntity* ie = new ItemEntity(level, x, y + yOffs, z, ItemInstance((short)resource, (short)count, 0));
    ie->throwTime = 10;
    level->addEntity(ie);
    return ie;
}

bool Entity::isOnFire() { return onFire > 0; }
bool Entity::interactPreventDefault() { return false; }
bool Entity::isItemEntity() { return false; }
bool Entity::isHangingEntity() { return false; }
int  Entity::getAuxData() { return 0; }

void Entity::playStepSound(int xt, int yt, int zt, int t) {
    level->playStepSound(this, xt, yt, zt, t);
}
