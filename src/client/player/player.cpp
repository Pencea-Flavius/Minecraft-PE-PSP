
#include "client/player/player.h"
#include "client/renderer/render.h"

#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/storage/level_storage.h"
#include "world/level/storage/worldlist.h"
#include "platform/audio/sound.h"
#include "platform/me/me.h"
#include "platform/path.h"
#include "platform/time.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/particle.h"
#include "world/level/tile/redstone_ore.h"
#include "world/item/crafting/recipe.h"
#include "client/gui/hud.h"

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

int g_viewBobbing = 1;
int g_fancyGraphics = 0;
int g_cloudTicks = 0;
extern int g_autosave;
int g_autosaveTick = 0;

bool  g_invOpen   = false;

float g_dropCharge = -1.0f;
int   g_invCursor = 0;
float g_flashSlotStartTime = -1.0f;
int   g_invFlashCursor = -1;
int   g_invFlashTicks = 0;

#define TICKS_PER_SECOND      20.0f
#define MAX_TICKS_PER_UPDATE  10
float g_timerPassed = 0.0f;
float g_timerLast   = 0.0f;
float g_timerAlpha  = 0.0f;

static const float DEG2RAD = 3.14159265f / 180.0f;

static inline int facingFromYaw(float yawDeg) {
    static const int kQuadrantFace[4] = { F_BACK, F_LEFT, F_FORWARD, F_RIGHT };
    int q = ((int)floorf(yawDeg / 90.0f + 0.5f)) & 3;
    return kQuadrantFace[q];
}

bool g_saveRequested = false;

bool g_quitAfterSave = false;

void playerRespawn() {
    LocalPlayer* p = g_level.player;
    if (!p) return;
    p->health = p->getMaxHealth();
    p->deathTime = 0; p->hurtTime = 0; p->invulnerableTime = 0;
    p->onFire = 0;

    if (g_bedSpawnY >= 0 &&
        worldBlock(&g_world, g_bedSpawnX, g_bedSpawnY, g_bedSpawnZ) == BLOCK_AIR &&
        worldBlock(&g_world, g_bedSpawnX, g_bedSpawnY + 1, g_bedSpawnZ) == BLOCK_AIR) {
        p->x = g_bedSpawnX + 0.5f; p->z = g_bedSpawnZ + 0.5f;
        playerSpawnAt(g_bedSpawnY + PLAYER_EYE);
        return;
    }
    int sx, sz, feetY;
    worldFindSpawn(&g_world, &sx, &sz, &feetY);
    p->x = sx + 0.5f; p->z = sz + 0.5f;
    playerSpawnAt(feetY + PLAYER_EYE);
}

void quitToMenuNoSave(MenuState& s) {
    worldFree(&g_world);
    g_level.removeAllEntities();
    g_level.removeAllTileEntities();
    delete g_level.player; g_level.player = 0;
    gameModeShutdown();
    g_worldBuilt = false;

    worldListScan(&s.worlds);
    s.worldSelected = 0;
    s.screen = SCREEN_WORLDS;
}

static void runTicks(MenuState& s, unsigned int btn, unsigned char lx, unsigned char ly) {

    float now = nowSeconds();

    if (!g_worldBuilt) { g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f; return; }
    float passed = now - g_timerLast;
    g_timerLast = now;
    if (passed < 0.0f) passed = 0.0f;
    if (passed > 1.0f) passed = 1.0f;
    g_timerPassed += passed * TICKS_PER_SECOND;
    int ticks = (int)g_timerPassed;
    g_timerPassed -= ticks;
    if (ticks > MAX_TICKS_PER_UPDATE) ticks = MAX_TICKS_PER_UPDATE;
    g_timerAlpha = g_timerPassed;

    for (int i = 0; i < ticks; i++) {
        if (g_level.player) g_level.player->aiStep(btn, lx, ly);

        if (g_worldBuilt && !meBusy()) worldTick(&g_world);
        if (g_worldBuilt) g_level.tickEntities();
        if (g_worldBuilt) g_level.tickTileEntities();
        if (g_worldBuilt && g_level.player) particlesTick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z);

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
        g_cloudTicks++;
    }
}

void gameUpdate(MenuState& s, unsigned int pressed, const SceCtrlData& pad) {

    if (g_signEditing) {
        signHandleInput(s, pressed);
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
        deadHandleInput(s, pressed);

        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_paused) {
        pauseHandleInput(s, pressed);
        float now = nowSeconds();
        g_timerLast = now; g_timerPassed = 0.0f; g_timerAlpha = 0.0f;
        return;
    }

    if (g_worldBuilt && g_level.player && g_level.player->isSleeping()) {
        inBedHandleInput(s, pressed);
        runTicks(s, 0, 128, 128);
        return;
    }

    if (g_craftOpen) {
        craftHandleInput(s, pressed);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_armorOpen) {
        armorHandleInput(s, pressed);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_furnaceOpen) {
        furnaceHandleInput(s, pressed, pad.Buttons);
        runTicks(s, 0, 128, 128);
        return;
    }
    if (g_chestOpen) {
        chestHandleInput(s, pressed, pad.Buttons);
        runTicks(s, 0, 128, 128);
        return;
    }

    if (g_invOpen) {

        if (pressed & PSP_CTRL_LEFT)  { if (g_invCursor > 0) g_invCursor--; }
        if (pressed & PSP_CTRL_RIGHT) { if (g_invCursor < g_inv.gridSize() - 1) g_invCursor++; }
        if (pressed & PSP_CTRL_UP) { if (g_invCursor >= INV_COLS) g_invCursor -= INV_COLS; }
        if (pressed & PSP_CTRL_DOWN)  { if (g_invCursor + INV_COLS < g_inv.gridSize()) g_invCursor += INV_COLS; }

        if (!g_inv.isCreative()) {
            if (pressed & PSP_CTRL_SQUARE) {
                g_invOpen = false;
                craftOpen(Recipe::SIZE_2X2, CRAFT_WORKBENCH);
                runTicks(s, 0, 128, 128);
                return;
            }
            if (pressed & PSP_CTRL_TRIANGLE) {
                g_invOpen = false;
                armorOpen();
                runTicks(s, 0, 128, 128);
                return;
            }
        }

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
                    if (dx < 0 && g_invCursor > 0) g_invCursor--;
                    if (dx > 0 && g_invCursor < g_inv.gridSize() - 1) g_invCursor++;
                    if (dy < 0 && g_invCursor >= INV_COLS) g_invCursor -= INV_COLS;
                    if (dy > 0 && g_invCursor + INV_COLS < g_inv.gridSize()) g_invCursor += INV_COLS;
                }
            } else lastMove = 0.0f;
        }
        if (pressed & PSP_CTRL_CROSS) {

            g_inv.pickToHotbar(g_invCursor);
            soundPlay("random.pop2", 1.0f, 0.3f);
            g_flashSlotStartTime = nowSeconds();
            g_invFlashCursor = g_invCursor;
            g_invFlashTicks = 7;
        }
        if (pressed & PSP_CTRL_CIRCLE) {
            g_invOpen = false;
            soundPlay("random.click", 1.0f, 1.0f);
        }

        runTicks(s, 0, 128, 128);
        return;
    }

    if (pressed & PSP_CTRL_SELECT) {
        g_paused = true;
        g_pauseSel = 0;
        return;
    }

    if ((pressed & PSP_CTRL_START) && g_level.player && g_inv.isCreative()) {
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

    if (pressed & PSP_CTRL_LEFT)  { if (g_inv.selected > 0) g_inv.selected--; else g_inv.selected = HOTBAR_SLOTS; }
    if (pressed & PSP_CTRL_RIGHT) { if (g_inv.selected < HOTBAR_SLOTS) g_inv.selected++; else g_inv.selected = 0; }

    {
        const float DROP_TAP = 0.22f;
        const float DROP_FULL = 1.0f;
        static float dropStart = -1.0f;
        static bool  dropFired = false;
        bool up = (pad.Buttons & PSP_CTRL_UP) != 0 && !g_inv.isCreative();
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
    if ((pressed & PSP_CTRL_LTRIGGER) && g_inv.selected == HOTBAR_SLOTS) {
        g_invOpen = true;
        soundPlay("random.click", 1.0f, 1.0f);
        int tgt = (g_inv.selected < HOTBAR_SLOTS) ? g_inv.selected : 0;
        g_invCursor = 0;
        ItemInstance* sel = g_inv.getLinked(tgt);
        for (int i = 0; sel && i < g_inv.gridSize(); i++) {
            ItemInstance* it = g_inv.getItem(i);
            if (it && it->id == sel->id && it->data == sel->data) { g_invCursor = i; break; }
        }
        return;
    }

    gameModeHandleInput(pressed, pad.Buttons);

    runTicks(s, pad.Buttons, pad.Lx, pad.Ly);
}

void playerSpawnEnsure() {

    g_bedSpawnY = -1;
    if (!g_level.player) {
        g_level.player = new LocalPlayer(&g_level);
        g_level.player->x = WORLD_W * 0.5f;
        g_level.player->z = WORLD_D * 0.5f;
        g_level.player->yRot = 45.0f;
        g_level.player->xRot = -35.0f;

        g_level.player->health = g_level.player->getMaxHealth();
    }
}

void playerSpawnAt(float eyeY) {
    playerSpawnEnsure();
    LocalPlayer* p = g_level.player;
    p->y = eyeY;
    p->setPos(p->x, p->y, p->z);
    p->xd = p->yd = p->zd = 0.0f;
    p->onGround = true;
    p->flying = false;
    p->xo = p->xOld = p->x; p->yo = p->yOld = p->y; p->zo = p->zOld = p->z;
    p->yRotO = p->yRot; p->xRotO = p->xRot;
}
