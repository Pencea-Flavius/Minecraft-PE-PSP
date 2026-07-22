#pragma once

struct BlockHit;
struct ItemInstance;

class GameMode {
public:
    virtual ~GameMode() {}
    virtual bool isCreative() = 0;
    virtual void handleInput(unsigned int pressed, unsigned int held);

    virtual bool useItemOn(ItemInstance* item, const BlockHit& hit);
};

class CreativeMode : public GameMode {
public:
    bool isCreative() { return true; }
};

class SurvivalMode : public GameMode {
public:
    bool isCreative() { return false; }

};

struct MiningState { bool active; int x, y, z; float progress; };
extern MiningState g_mining;

extern GameMode* g_gameMode;
void gameModeInit(int gameType);
void gameModeShutdown();

void gameModeHandleInput(unsigned int pressed, unsigned int held);

void playerDropSelected(bool all);

struct CrosshairTarget { const char* useLabel; const char* breakLabel; };
CrosshairTarget gameModeCrosshairTarget();
