
#ifndef MCPSP_WORLD_ENTITY_MOB_H
#define MCPSP_WORLD_ENTITY_MOB_H

#include "world/entity/entity.h"
#include "world/entity/ai/goal_selector.h"
#include "world/level/pathfinder/path.h"

class LookControl;
class MoveControl;
class JumpControl;
class BodyControl;
class PathNavigation;
class Sensing;

float mobAiRange();

class Mob : public Entity {
public:
    Mob(Level* level);
    virtual ~Mob();

    bool flying;

    float xxa, yya, yRotA;
    bool  jumping;
    float flyingSpeed;

    float flySlowdown;
    float defaultLookAngle;

    int   health, lastHealth, lastHurt;
    int   hurtTime, hurtDuration, deathTime, attackTime;
    int   invulnerableDuration;
    int   dmgSpill;
    float hurtDir;
    int   noActionTime;

    float yHeadRot, yHeadRotO;
    float yBodyRot, yBodyRotO;
    float walkAnimSpeed, walkAnimSpeedO;
    float walkAnimPos,   walkAnimPosO;
    float run, oRun, animStep, animStepO;
    int   lookTime;

    float attackAnim, oAttackAnim;
    int   swingTime;
    bool  swinging;
    void  swing();
    void  updateAttackAnim();

    float getAttackAnim(float a) {
        float aa = attackAnim;
        if (aa < oAttackAnim) aa += 1.0f;
        return oAttackAnim + (aa - oAttackAnim) * a;
    }

    virtual bool isMob() { return true; }

    virtual bool removeWhenFarAway() { return true; }
    virtual bool isPickable() { return true; }
    virtual bool isPushable() { return true; }
    virtual bool playerInteract() { return false; }
    virtual int  getMaxHealth() { return 10; }
    virtual bool isBaby() { return false; }
    virtual void outOfWorld();
    virtual bool isImmobile() { return health <= 0; }

    virtual bool useNewAi() { return false; }
    void newServerAiStep();
    virtual void serverAiMobStep() {}

    virtual void ate() {}

    LookControl*    getLookControl() { return lookControl; }
    MoveControl*    getMoveControl() { return moveControl; }
    JumpControl*    getJumpControl() { return jumpControl; }
    BodyControl*    getBodyControl() { return bodyControl; }
    PathNavigation* getNavigation()  { return navigation; }
    Sensing*        getSensing()     { return sensing; }

    GoalSelector goalSelector;
    GoalSelector goalSelector2;

    virtual float getBaseSpeed() { return 0.1f; }
    float getSpeed() { return useNewAi() ? speed : getBaseSpeed(); }
    void  setSpeed(float s) { speed = s; yya = s; }
    void  setYya(float v) { yya = v; }
    void  setJumping(bool v) { jumping = v; }
    virtual float getMaxHeadXRot() { return 40.0f; }

    static const int STEER_TURN_RATE = 30;
    static const int BODY_TURN_RATE  = 30;

    Mob* getLastHurtByMob();
    void setLastHurtByMob(Mob* m);
    int  lastHurtByMobId, lastHurtByMobTime;

    int  attackTargetId;
    Entity* getTarget();
    void setTarget(Entity* e) { attackTargetId = e ? e->entityId : 0; }

    virtual bool doHurtTarget(Entity* ) { return false; }

    virtual void performRangedAttack(Entity* , float ) {}
    float speed;

    Path path;

    virtual void tick();
    virtual void baseTick();
    virtual void aiStep();
    virtual void updateAi();

    void applySwimUrge();

    void checkDespawn();
    void updateWalkAnim();
    virtual void jumpFromGround();
    virtual float getWalkingSpeedModifier() { return 1.0f; }

    virtual bool canSee(Entity* e);

    virtual bool hurt(Entity* source, int damage);
    virtual void actuallyHurt(int damage);
    virtual int  getArmorValue() { return 0; }
    virtual void hurtArmor(int ) {}
    int getDamageAfterArmorAbsorb(int damage);
    virtual void knockback(Entity* source, int damage, float xdir, float zdir);
    virtual void die(Entity* source);
    virtual void dropDeathLoot();
    virtual int  getDeathLoot() { return 0; }
    void heal(int amount);
    virtual void animateHurt();
    virtual bool isAlive();

    virtual bool canSpawn();

    virtual float getHeadHeight() { return bbHeight * 0.85f; }

    virtual const char* getHurtSound()  { return "random.hurt"; }
    virtual const char* getDeathSound() { return "random.hurt"; }
    virtual const char* getAmbientSound() { return 0; }
    virtual float getSoundVolume() { return 1.0f; }

    virtual float getVoicePitch();
    virtual int   getAmbientSoundInterval() { return 8 * TicksPerSecond; }
    void playAmbientSound();
    int  ambientSoundTime;

    virtual void travel(float xs, float yf);

    virtual void causeFallDamage(float dist);

protected:
    virtual void readAdditionalSaveData(CompoundTag* tag);
    virtual void addAdditonalSaveData(CompoundTag* tag);

    bool isFreeM(float dx, float dy, float dz);
    unsigned char bodyBlock();
    virtual bool onLadder();

    LookControl*    lookControl;
    MoveControl*    moveControl;
    JumpControl*    jumpControl;
    BodyControl*    bodyControl;
    PathNavigation* navigation;
    Sensing*        sensing;
};

#endif
