
#ifndef MCPSP_WORLD_ENTITY_ENTITY_H
#define MCPSP_WORLD_ENTITY_ENTITY_H

#include "world/phys/aabb.h"
#include "world/entity/entity_renderer_id.h"
#include "world/level/levelgen/Random.h"

class Level;
class ItemEntity;
class ItemInstance;
class SynchedEntityData;
class CompoundTag;

class Entity {
public:
    static int entityCounter;
    static const int TicksPerSecond = 20;
    static const int TOTAL_AIR_SUPPLY = 15 * TicksPerSecond;

    Entity(Level* level);
    virtual ~Entity();

    static const int ENTITY_POOL = 96;
    static const unsigned ENTITY_SLOT = 2560;
    static bool  hasFreeSlot();
    static void* operator new(unsigned n);
    static void  operator delete(void* p);

    void _init();
    virtual void reset();

    int  hashCode();
    bool operator==(Entity& rhs);

    virtual void setLevel(Level* level);
    virtual void remove();

    virtual void setPos(float x, float y, float z);
    virtual void move(float xa, float ya, float za);
    virtual void moveTo(float x, float y, float z, float yRot, float xRot);
    virtual void moveRelative(float xa, float za, float speed);

    virtual void lerpTo(float x, float y, float z, float yRot, float xRot, int steps);
    virtual void lerpMotion(float xd, float yd, float zd);

    virtual void turn(float xo, float yo);
    virtual void interpolateTurn(float xo, float yo);

    virtual void tick();
    virtual void baseTick();

    virtual bool intersects(float x0, float y0, float z0, float x1, float y1, float z1);
    virtual bool isFree(float xa, float ya, float za, float grow);
    virtual bool isFree(float xa, float ya, float za);
    virtual bool isInWall();
    virtual bool isInWater();
    virtual bool isInLava();

    virtual void makeStuckInWeb();

    virtual float getHeadHeight();
    virtual float getShadowHeightOffs();
    virtual float getBrightness(float a);

    float distanceTo(Entity* e);
    float distanceTo(float x2, float y2, float z2);
    float distanceToSqr(float x2, float y2, float z2);
    float distanceToSqr(Entity* e);

    virtual bool interactPreventDefault();
    virtual bool interact();

    virtual void push(Entity* e);
    virtual void push(float xa, float ya, float za);

    virtual bool isPickable();
    virtual bool isPushable();
    virtual bool isShootable();
    virtual bool isSneaking();
    virtual bool isAlive();
    virtual bool isOnFire();
    virtual bool isPlayer();
    virtual bool isCreativeModeAllowed();

    virtual bool shouldRender(float cx, float cy, float cz);
    virtual bool shouldRenderAtSqrDistance(float distance);

    virtual bool hurt(Entity* source, int damage);
    virtual void animateHurt();
    virtual void handleEntityEvent(char eventId) {}
    virtual float getPickRadius();

    virtual ItemEntity* spawnAtLocation(int resource, int count);
    virtual ItemEntity* spawnAtLocation(int resource, int count, float yOffs);

    virtual void awardKillScore(Entity* victim, int score);
    virtual void setEquippedSlot(int slot, int item, int auxValue);

    virtual bool save(CompoundTag* entityTag);
    virtual void saveWithoutId(CompoundTag* entityTag);
    virtual bool load(CompoundTag* entityTag);
    virtual SynchedEntityData* getEntityData();

    inline bool isEntityType(int type) { return getEntityTypeId() == type; }
    virtual int getEntityTypeId() const = 0;
    virtual int getCreatureBaseType() const { return 0; }
    virtual EntityRendererId queryEntityRenderer() { return ER_DEFAULT_RENDERER; }

    virtual bool isMob() { return false; }
    virtual bool isItemEntity();
    virtual bool isHangingEntity();
    virtual int  getAuxData();
    virtual bool isBaby() { return false; }

protected:
    virtual void setRot(float yRot, float xRot);
    virtual void setSize(float w, float h);
    virtual void resetPos(bool clearMore);
    virtual void outOfWorld();

    virtual void checkFallDamage(float ya, bool onGround);
    virtual void causeFallDamage(float fallDamage2);
    virtual void markHurt();

    virtual void burn(int dmg);
    virtual void lavaHurt();

    virtual void readAdditionalSaveData(CompoundTag* tag) = 0;
    virtual void addAdditonalSaveData(CompoundTag* tag) = 0;

    virtual void playStepSound(int xt, int yt, int zt, int t);

public:
    float x, y, z;
    int xChunk, yChunk, zChunk;
    int entityId;
    float viewScale;

    Level* level;
    float xo, yo, zo;
    float xd, yd, zd;
    float yRot, xRot;
    float yRotO, xRotO;

    AABB bb;
    float heightOffset;
    float bbWidth, bbHeight;
    float walkDistO, walkDist;
    float xOld, yOld, zOld;
    float ySlideOffset;
    float footSize;
    float pushthrough;

    int tickCount;
    int invulnerableTime;
    int airSupply;
    int onFire;
    int flameTime;

    EntityRendererId entityRendererId;

    float fallDistance;
    bool blocksBuilding;
    bool inChunk;
    bool onGround;
    bool horizontalCollision, verticalCollision;
    bool collision;
    bool hurtMarked;
    bool slide;
    bool removed;
    bool noPhysics;
    bool canRemove;
    bool invisible;
    bool reallyRemoveIfPlayer;

protected:
    static Random sharedRandom;
    int airCapacity;
    bool makeStepSound;
    bool wasInWater;
    bool fireImmune;
    bool firstTick;
    int  nextStep;
    bool isStuckInWeb;
};

#endif
