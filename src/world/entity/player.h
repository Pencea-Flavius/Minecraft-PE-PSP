
#ifndef MCPSP_WORLD_ENTITY_PLAYER_H
#define MCPSP_WORLD_ENTITY_PLAYER_H

#include "world/entity/mob.h"
#include "world/item/item_instance.h"

class Inventory;

class Player : public Mob {
public:
    Player(Level* level);
    virtual ~Player();

    Inventory* inventory;

    static const int NUM_ARMOR = 4;
    ItemInstance armor[NUM_ARMOR];

    ItemInstance* getArmor(int slot);
    void setArmor(int slot, const ItemInstance* item);
    virtual int getArmorValue();
    void hurtArmor(int dmg);

    float bob, oBob, tilt, oTilt;
    float bowPull, bowTimeHeld;
    float eatAnim;

    virtual bool isPlayer() { return true; }
    virtual int  getEntityTypeId() const;
    virtual int  getMaxHealth() { return 20; }

    int score;
    int getScore() const { return score; }
    virtual void awardKillScore(Entity* victim, int amount) { score += amount; }

    enum { BED_OK = 0, BED_NOT_POSSIBLE_HERE = 1, BED_NOT_POSSIBLE_NOW = 2,
           BED_TOO_FAR_AWAY = 3, BED_OTHER_PROBLEM = 4, BED_NOT_SAFE = 5 };
    static const int SLEEP_DURATION = 100;
    bool  sleeping;
    short sleepCounter;
    int   bedX, bedY, bedZ;

    bool isSleeping() const { return sleeping; }
    bool isSleepingLongEnough() const { return sleeping && sleepCounter >= SLEEP_DURATION; }
    int  startSleepInBed(int x, int y, int z);
    void stopSleepInBed(bool forcefulWakeUp, bool saveRespawnPoint);
    void sleepTick();
    bool checkBed();

    virtual bool hurt(Entity* source, int dmg);

    virtual void causeFallDamage(float dist);

protected:
    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);
};

struct World;
extern const int BED_HEAD_OFF[4][2];
bool bedFindStandUpPosition(World* w, int x, int y, int z, int dir, int* ox, int* oy, int* oz);

extern int g_bedSpawnX, g_bedSpawnY, g_bedSpawnZ;

#endif
