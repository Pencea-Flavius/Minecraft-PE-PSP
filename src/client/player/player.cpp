
#include "client/player/player.h"
#include "client/gui/screens/screen.h"
#include "client/gui/screens/control_scheme.h"
#include "client/gui/screens/panorama.h"
#include "client/renderer/render.h"

#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/storage/level_storage.h"
#include "world/level/storage/worldlist.h"
#include "platform/audio/sound.h"

#include "platform/path.h"
#include "platform/time.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/level/near_patch.h"
#include "client/renderer/particle.h"
#include "world/level/tile/redstone_ore.h"
#include "world/item/crafting/recipe.h"
#include "client/gui/hud.h"
#include "client/gui/inventory_ui.h"
#include "util/prof.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pspkernel.h>

extern World g_world;
extern Level g_level;
extern bool  g_worldBuilt;

#include "client/player/player_state.h"
#include "client/gamemode/gamemode.h"
#include "world/entity/local_player.h"

extern int g_classicPick;

int g_viewBobbing = 1;
int g_fancyGraphics = 0;
int g_fancyLeaves = 0;
int g_cloudTicks = 0;
extern int g_autosave;
int g_autosaveTick = 0;

bool  g_invOpen   = false;

float g_dropCharge = -1.0f;
int   g_invCursor = 0;
int   g_invHeaderSel = -1;
float g_flashSlotStartTime = -1.0f;

int   g_flashSlotIndex = 0;
int   g_invFlashCursor = -1;
int   g_invFlashTicks = 0;

#define TICKS_PER_SECOND      20.0f
#define MAX_TICKS_PER_UPDATE  10
float g_timerPassed = 0.0f;
float g_timerLast   = 0.0f;
float g_timerAlpha  = 0.0f;

static const float DEG2RAD = 3.14159265f / 180.0f;

bool g_saveRequested = false;
bool g_isoMapRequested = false;

bool g_quitAfterSave = false;

void playerRespawn() {
    LocalPlayer* p = g_level.player;
    if (!p) return;
    p->health = p->getMaxHealth();
    p->deathTime = 0; p->hurtTime = 0; p->invulnerableTime = 0;
    p->onFire = 0;

    g_level.validateSpawn();
    int sx = p->hasRespawnPosition() ? p->respawnX : g_level.spawnX;
    int sy = p->hasRespawnPosition() ? p->respawnY : g_level.spawnY;
    int sz = p->hasRespawnPosition() ? p->respawnZ : g_level.spawnZ;
    p->x = sx + 0.5f; p->z = sz + 0.5f;
    playerSpawnAt(sy + 1.0f);
    p->resetPos(true);
}

void releaseWorldAndPlayer() {
    extern void skyFreeStars(void);
    extern void cloudFreeMesh(void);
    skyFreeStars();
    cloudFreeMesh();
    nearPatchFree();
    worldFree(&g_world);
    g_level.removeAllEntities();
    g_level.removeAllTileEntities();
    hudChatClear();
    delete g_level.player; g_level.player = 0;
    gameModeShutdown();
    g_worldBuilt = false;
}

void quitToMenuNoSave(MenuState& s) {
    g_saveRequested = false;
    g_isoMapRequested = false;
    g_quitAfterSave = false;
    g_invOpen = false;
    g_craftOpen = false;
    g_armorOpen = false;
    g_uiParentIsInventory = false;
    g_furnaceOpen = false;
    chestClose();
    signStopEdit();
    g_deadScreen = false;
    g_paused = false;
    g_terrainProgress = 0;
    extern MiningState g_mining;
    g_mining.active = false; g_mining.progress = 0.0f;

    soundStopAll();

    releaseWorldAndPlayer();

    worldListScan(&s.worlds);
    s.worldSelected = 0;

    panoramaLoadAll();
    s.screen = SCREEN_WORLDS;
}

void gameTimerReset() {
    g_timerLast = nowSeconds();
    g_timerPassed = 0.0f;
    g_timerAlpha = 0.0f;
}

static void runTicks(MenuState& s, unsigned int btn, unsigned char lx, unsigned char ly,
                     unsigned char rx = 128, unsigned char ry = 128) {

    float now = nowSeconds();

    if (!g_worldBuilt) { g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f; return; }
    float passed = now - g_timerLast;
    g_timerLast = now;
    if (passed < 0.0f) passed = 0.0f;
    if (passed > 1.0f) passed = 1.0f;

    g_gameSeconds += passed;
    g_gameFrozen = false;
    g_timerPassed += passed * TICKS_PER_SECOND;
    int ticks = (int)g_timerPassed;
    g_timerPassed -= ticks;
    if (ticks > MAX_TICKS_PER_UPDATE) ticks = MAX_TICKS_PER_UPDATE;
    g_timerAlpha = g_timerPassed;

    profBegin(PROF_TICK);
    for (int i = 0; i < ticks; i++) {
        profBegin(PROF_TPLAYER);
        if (g_level.player) g_level.player->aiStep(btn, lx, ly, rx, ry);
        profEnd(PROF_TPLAYER);

        g_world.simTick = true;
        profBegin(PROF_TWORLD);
        if (g_worldBuilt)
            worldTick(&g_world);
        profEnd(PROF_TWORLD);
        profBegin(PROF_TENT);
        if (g_worldBuilt) g_level.tickEntities();
        profEnd(PROF_TENT);
        profBegin(PROF_TTE);
        if (g_worldBuilt) g_level.tickTileEntities();
        profEnd(PROF_TTE);
        g_world.simTick = false;

        if (g_worldBuilt) g_level.tickCaveMood();
        profBegin(PROF_TPART);
        if (g_worldBuilt && g_level.player) particlesTick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z);
        profEnd(PROF_TPART);

        if (g_worldBuilt && g_autosave > 0) {
            ++g_autosaveTick;

            int remaining = g_autosave - g_autosaveTick;
            if (remaining > 0 && remaining <= 100 && remaining % 20 == 0) {
                char buf[24];
                snprintf(buf, sizeof(buf), "Autosave in: %d", remaining / 20);
                hudChatMessage(buf);
            }
            if (g_autosaveTick >= g_autosave) { g_autosaveTick = 0; g_saveRequested = true; }
        }
        if (g_invFlashTicks > 0) g_invFlashTicks--;
        if (g_useItemDelay > 0) g_useItemDelay--;
        g_cloudTicks++;
    }
    profEnd(PROF_TICK);
}

void gameUpdate(MenuState& s, unsigned int pressed, const SceCtrlData& padIn) {

    static unsigned int s_swallow = 0;
    static bool s_uiOwned = false;
    s_swallow &= padIn.Buttons;
    const bool uiOwnedLastFrame = s_uiOwned;
    s_uiOwned = true;
    SceCtrlData pad = padIn;

    const unsigned int uiPressed = menuFaceSwap(controlSchemeMenuAlias(pressed));
    const unsigned int uiHeld    = menuFaceSwap(controlSchemeMenuAlias(pad.Buttons));

    g_gameFrozen = true;

    if (g_signEditing) {
        signScreen().handleInput(s, uiPressed, uiHeld);
        float now = nowSeconds();
        g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f;
        return;
    }

    if (gameProgressScreenUp()) {
        float now = nowSeconds();
        g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f;
        return;
    }

    if (g_worldBuilt && g_level.player && g_level.player->health <= 0 && !g_deadScreen)
        deadScreenOpen();
    if (g_deadScreen) {
        deadScreen().handleInput(s, uiPressed, uiHeld);

        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_paused) {

        soundStopWorld();
        pauseScreen().handleInput(s, uiPressed, uiHeld);
        float now = nowSeconds();
        g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f;
        return;
    }

    if (g_optionsOpen) {
        optionsScreen().handleInput(s, uiPressed, uiHeld);
        float now = nowSeconds();
        g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f;
        return;
    }

    if (g_worldBuilt && g_level.player && g_level.player->isSleeping()) {
        inBedScreen().handleInput(s, uiPressed, uiHeld);
        runTicks(s, 0, 128, 128);
        return;
    }

    if (g_craftOpen) {
        craftScreen().handleInput(s, uiPressed, uiHeld);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_armorOpen) {
        armorScreen().handleInput(s, uiPressed, uiHeld);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_furnaceOpen) {
        furnaceScreen().handleInput(s, uiPressed, uiHeld);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_chestOpen) {
        chestScreen().handleInput(s, uiPressed, uiHeld);
        runTicks(s, 0, 128, 128);
        return;
    }

    if (g_invOpen) {

        Inventory* inv = g_level.player->inventory;

        if (inv->selected >= Inventory::HOTBAR) inv->selectSlot(0);

        if (!g_classicPick) {
            int slotStep = 0;
            if (uiPressed & PSP_CTRL_LTRIGGER) slotStep = -1;
            if (uiPressed & PSP_CTRL_RTRIGGER) slotStep =  1;
            if (slotStep) {
                int sel = inv->selected + slotStep;
                if (sel < 0) sel = 0;
                if (sel > Inventory::HOTBAR - 1) sel = Inventory::HOTBAR - 1;
                if (sel != inv->selected) {
                    inv->selectSlot(sel);
                    soundPlay("random.click", 1.0f, 1.0f);
                }
            }
        }

        int mvx = 0, mvy = 0;
        if (uiPressed & PSP_CTRL_LEFT)  mvx = -1;
        if (uiPressed & PSP_CTRL_RIGHT) mvx =  1;
        if (uiPressed & PSP_CTRL_UP)    mvy = -1;
        if (uiPressed & PSP_CTRL_DOWN)  mvy =  1;
        {

            int adx = (int)pad.Lx - 128, ady = (int)pad.Ly - 128;
            int aadx = adx < 0 ? -adx : adx, aady = ady < 0 ? -ady : ady;
            int dx = 0, dy = 0;
            if (aady >= aadx) { if (ady < -50) dy = -1; else if (ady > 50) dy = 1; }
            else              { if (adx < -50) dx = -1; else if (adx > 50) dx = 1; }
            static float lastMove = 0.0f;
            if (dx || dy) {

                float nowA = nowSeconds();
                if (nowA - lastMove > 0.14f) {
                    lastMove = nowA;
                    if (!mvx) mvx = dx;
                    if (!mvy) mvy = dy;
                }
            } else lastMove = 0.0f;
        }

        if (inv->isCreative()) {
            creativeRebuild();
            const int n = creativeCount();

            int tabStep = 0;
            if (g_classicPick) {
                if (uiPressed & PSP_CTRL_LTRIGGER)  tabStep = -1;
                if (uiPressed & PSP_CTRL_RTRIGGER)  tabStep =  1;
            } else {
                if (uiPressed & PSP_CTRL_SQUARE)   tabStep = -1;
                if (uiPressed & PSP_CTRL_TRIANGLE) tabStep =  1;
            }
            if (tabStep) {
                g_creativeTab = (g_creativeTab + tabStep + CREATIVE_TABS) % CREATIVE_TABS;
                g_invCursor = 0;
                soundPlay("random.click", 1.0f, 1.0f);
            }

            {
                if (mvx < 0 && g_invCursor > 0) g_invCursor--;
                if (mvx > 0 && g_invCursor < n - 1) g_invCursor++;

                if (mvy < 0 && g_invCursor >= CREATIVE_COLS) g_invCursor -= CREATIVE_COLS;
                if (mvy > 0 && g_invCursor + CREATIVE_COLS < n) g_invCursor += CREATIVE_COLS;

                if (uiPressed & PSP_CTRL_CROSS) {
                    const CreativeEntry* e = creativeEntry(g_invCursor);
                    if (e) {

                        int slot = g_classicPick
                                 ? inv->classicCreativeAddItem(e->id, e->count, e->aux)
                                 : inv->creativeAddItem(e->id, e->count, e->aux);
                        soundPlay("random.pop2", 1.0f, 0.3f, SND_CAT_UI);

                        g_flashSlotStartTime = gameSeconds();
                        g_flashSlotIndex     = slot;

                        g_invFlashCursor = g_invCursor;
                        g_invFlashTicks  = 7;
                    }
                }
            }

            if (uiPressed & PSP_CTRL_CIRCLE) {
                g_invOpen = false;
                soundPlay("random.click", 1.0f, 1.0f);
                runTicks(s, 0, 128, 128);
                return;
            }
            runTicks(s, 0, 128, 128);
            return;
        }

        int act = -1;
        if (g_invHeaderSel >= 0) {

            if (mvx)
                for (int i = g_invHeaderSel + mvx; i >= 0 && i < INV_BTN_COUNT; i += mvx)
                    if (invHeaderButton(s, i)) { g_invHeaderSel = i; break; }
            if (mvy > 0) g_invHeaderSel = -1;
            if (uiPressed & PSP_CTRL_CROSS) act = g_invHeaderSel;
        } else {
            if (mvx < 0 && g_invCursor > 0) g_invCursor--;
            if (mvx > 0 && g_invCursor < inv->gridSize() - 1) g_invCursor++;
            if (mvy < 0) {
                if (g_invCursor >= INV_COLS) g_invCursor -= INV_COLS;
                else g_invHeaderSel = INV_BTN_BACK;
            }
            if (mvy > 0 && g_invCursor + INV_COLS < inv->gridSize()) g_invCursor += INV_COLS;
            if (uiPressed & PSP_CTRL_CROSS) {

                int slot = g_classicPick ? inv->classicSurvivalAddItem(g_invCursor)
                                         : inv->survivalAddItem(g_invCursor);
                soundPlay("random.pop2", 1.0f, 0.3f, SND_CAT_UI);
                g_flashSlotStartTime = gameSeconds();
                g_flashSlotIndex     = slot;
                g_invFlashCursor = g_invCursor;
                g_invFlashTicks = 7;
            }
        }

        if (uiPressed & PSP_CTRL_SQUARE)   act = INV_BTN_CRAFT;
        if (uiPressed & PSP_CTRL_TRIANGLE) act = INV_BTN_ARMOR;
        if (uiPressed & PSP_CTRL_CIRCLE)   act = INV_BTN_BACK;
        if (act >= 0 && invHeaderButton(s, act)) {
            g_invOpen = false;

            g_uiParentIsInventory = (act == INV_BTN_CRAFT || act == INV_BTN_ARMOR);
            if (act == INV_BTN_CRAFT)      craftOpen(Recipe::SIZE_2X2, CRAFT_WORKBENCH);
            else if (act == INV_BTN_ARMOR) armorOpen();
            else soundPlay("random.click", 1.0f, 1.0f);
            runTicks(s, 0, 128, 128);
            return;
        }

        runTicks(s, 0, 128, 128);
        return;
    }

    if (pressed & PSP_CTRL_SELECT) {
        g_paused = true;
        g_pauseSel = 0;

        soundStopAll();
        return;
    }

    s_uiOwned = false;
    if (uiOwnedLastFrame) s_swallow |= padIn.Buttons;
    pressed     &= ~s_swallow;
    pad.Buttons &= ~s_swallow;

    pressed     = controlSchemeRemap(pressed);
    pad.Buttons = controlSchemeRemap(pad.Buttons);

    controlSchemeCombos(pressed, pad.Buttons);

    if ((pressed & PSP_CTRL_START) && g_level.player && g_level.player->inventory->isCreative()) {
        static float lastJumpPress = -1.0f;
        float nowP = nowSeconds();
        if (lastJumpPress >= 0.0f && nowP - lastJumpPress < 0.3f) {
            g_level.player->flying = !g_level.player->flying;
            g_level.player->yd = 0.0f;
            lastJumpPress = -1.0f;
        } else {
            lastJumpPress = nowP;
        }
    }

    if (pressed & PSP_CTRL_LEFT)  { if (g_level.player->inventory->selected > 0) g_level.player->inventory->selected--; else g_level.player->inventory->selected = HOTBAR_SLOTS; }
    if (pressed & PSP_CTRL_RIGHT) { if (g_level.player->inventory->selected < HOTBAR_SLOTS) g_level.player->inventory->selected++; else g_level.player->inventory->selected = 0; }

    if (((pressed & PSP_CTRL_UP) && g_level.player->inventory->selected == HOTBAR_SLOTS)
        || (pressed & ACT_THIRDPERSON)) {
        optionsCycleCameraMode();
        soundPlay("random.click", 1.0f, 1.0f);
    }

    {
        const float DROP_TAP = 0.22f;
        const float DROP_FULL = 1.0f;
        static float dropStart = -1.0f;
        static bool  dropFired = false;

        bool up = !g_level.player->inventory->isCreative() &&
                  (((pad.Buttons & PSP_CTRL_UP) &&
                    g_level.player->inventory->selected < HOTBAR_SLOTS)
                   || (pad.Buttons & ACT_DROP) != 0);
        float nowD = nowSeconds();
        if (up) {
            if (dropStart < 0.0f) { dropStart = nowD; dropFired = false; }
            float dt = nowD - dropStart;
            if (!dropFired && dt >= DROP_FULL) {
                playerDropSelected(true);
                dropFired = true;
                g_dropCharge = -1.0f;
            } else if (!dropFired && dt >= DROP_TAP) {
                g_dropCharge = (dt - DROP_TAP) / (DROP_FULL - DROP_TAP);
            } else {
                g_dropCharge = -1.0f;
            }
        } else {
            if (dropStart >= 0.0f && !dropFired && (nowD - dropStart) < DROP_TAP)
                playerDropSelected(false);
            dropStart = -1.0f;
            dropFired = false;
            g_dropCharge = -1.0f;
        }
    }
    if (((pressed & PSP_CTRL_LTRIGGER) && g_level.player->inventory->selected == HOTBAR_SLOTS)
        || (pressed & ACT_INVENTORY)) {
        g_invOpen = true;
        soundPlay("random.click", 1.0f, 1.0f);
        g_invHeaderSel = -1;
        int tgt = (g_level.player->inventory->selected < HOTBAR_SLOTS) ? g_level.player->inventory->selected : 0;
        g_invCursor = 0;
        ItemInstance* sel = g_level.player->inventory->getLinked(tgt);
        for (int i = 0; sel && i < g_level.player->inventory->gridSize(); i++) {
            ItemInstance* it = g_level.player->inventory->gridItem(i);
            if (it && it->id == sel->id && it->data == sel->data) { g_invCursor = i; break; }
        }
        return;
    }

    if ((pressed & ACT_CRAFT) && !g_level.player->inventory->isCreative()) {
        craftOpen(Recipe::SIZE_2X2, CRAFT_WORKBENCH);
        return;
    }

    gameModeHandleInput(pressed, pad.Buttons);

    runTicks(s, pad.Buttons, pad.Lx, pad.Ly, pad.Rx, pad.Ry);
}

void playerSpawnEnsure() {

    if (!g_level.player) {
        g_level.player = new LocalPlayer(&g_level);
        g_level.player->x = WORLD_W * 0.5f;
        g_level.player->z = WORLD_D * 0.5f;
        g_level.player->yRot = 0.0f;
        g_level.player->xRot = 0.0f;

        g_level.player->health = g_level.player->getMaxHealth();
    }
}

void playerSpawnAt(float eyeY) {
    playerSpawnEnsure();
    LocalPlayer* p = g_level.player;
    p->y = eyeY;
    p->ySlideOffset = 0.0f;
    p->setPos(p->x, p->y, p->z);

    auto snapAxis = [](float& lo, float& hi, float size) {
        float s0 = std::round(lo * 16.0f) / 16.0f, s1 = std::round(hi * 16.0f) / 16.0f;
        if      (std::fabs(lo - s0) < 0.001f) { lo = s0; hi = s0 + size; }
        else if (std::fabs(hi - s1) < 0.001f) { hi = s1; lo = s1 - size; }
    };
    snapAxis(p->bb.x0, p->bb.x1, p->bbWidth);
    snapAxis(p->bb.y0, p->bb.y1, p->bbHeight);
    snapAxis(p->bb.z0, p->bb.z1, p->bbWidth);
    p->xd = p->yd = p->zd = 0.0f;
    p->onGround = true;
    p->flying = false;

    p->fallDistance = 0.0f;
    p->onFire = 0;
    p->xo = p->xOld = p->x; p->yo = p->yOld = p->y; p->zo = p->zOld = p->z;
    p->yRotO = p->yRot; p->xRotO = p->xRot;

    p->xBob = p->xBobO = p->xRot; p->yBob = p->yBobO = p->yRot;
}
