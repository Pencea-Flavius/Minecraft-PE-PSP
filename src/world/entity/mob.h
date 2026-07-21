
#ifndef MCPSP_WORLD_ENTITY_MOB_H
#define MCPSP_WORLD_ENTITY_MOB_H

#include "world/entity/entity.h"

float mobAiRange();

class Mob : public Entity {
public:
    Mob(Level* level);

    bool flying;

    float xxa, yya, yRotA;
    bool  jumping;
    float walkingSpeed, flyingSpeed;
    float defaultLookAngle;

    int   health, lastHealth, lastHurt;
    int   hurtTime, hurtDuration, deathTime, attackTime;
    int   invulnerableDuration;
    int   dmgSpill;
    float hurtDir;
    int   noActionTime;
    int   airSupply;

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
    virtual bool isImmobile() { return health <= 0; }

    virtual void tick();
    virtual void baseTick();
    virtual void aiStep();
    virtual void updateAi();
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

    virtual const char* getHurtSound()  { return "random.hurt"; }
    virtual const char* getDeathSound() { return "random.hurt"; }
    virtual const char* getAmbientSound() { return 0; }
    virtual float getSoundVolume() { return 1.0f; }

    virtual float getVoicePitch();
    virtual int   getAmbientSoundInterval() { return 8 * TicksPerSecond; }
    void playAmbientSound();
    int  ambientSoundTime;

    virtual void travel(float xs, float yf);

    virtual void move(float xa, float ya, float za);

    virtual void causeFallDamage(float dist);

protected:
    virtual void readAdditionalSaveData(CompoundTag* tag);
    virtual void addAdditonalSaveData(CompoundTag* tag);

    void mobMoveRelative(float xs, float yf, float speed);
    bool isFreeM(float dx, float dy, float dz);
    unsigned char bodyBlock();
    virtual bool onLadder();
};

#endif
