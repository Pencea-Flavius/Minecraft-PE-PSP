
#include <pspkernel.h>
#include <pspsysmem.h>
#include <pspctrl.h>
#include <psppower.h>
#include <cstring>
#include <malloc.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include "gpu/gu.h"
#include "gpu/texture.h"
#include "gpu/sprite.h"
#include "gpu/font.h"
#include "platform/path.h"
#include "platform/audio/sound.h"
#include "world/level/storage/worldlist.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/tile.h"
#include "client/gui/screens/menu.h"
#include "client/player/player.h"
#include "client/renderer/render.h"

#include "platform/time.h"
#include "world/level/level.h"
#include "world/entity/entity.h"
#include "world/entity/local_player.h"
#include "world/entity/item_entity.h"
#include "world/entity/entity_types.h"
#include "client/renderer/item_hand.h"

#include <pspgu.h>
#include <pspgum.h>

SceInt64 g_timeBootUs = 0;

PSP_MODULE_INFO("MinecraftPSP", 0, 0, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
#ifndef PSP_LARGE_MEMORY
#define PSP_LARGE_MEMORY(flag) int psp_large_memory = (flag)
#endif
PSP_LARGE_MEMORY(1);

int g_lowMemPsp = 0;
static void detectLowMemPsp(void) {
    size_t freeMem = sceKernelMaxFreeMemSize();
    g_lowMemPsp = (freeMem < 28 * 1024 * 1024);
}

static volatile int g_exitRequested = 0;

static int exitCallback(int , int , void* ) {
    g_exitRequested = 1;
    return 0;
}

static int callbackThread(SceSize , void* ) {
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, 0);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callbackThread,
                                     0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
}

static bool loadTex(Texture* out, const char* rel) {
    return textureLoad(assetPath(rel), out) || textureLoad(rel, out);
}

static bool loadTex4444(Texture* out, const char* rel) {
    return textureLoad4444(assetPath(rel), out) || textureLoad4444(rel, out);
}

static bool loadTex16(Texture* out, const char* rel, int psm) {
    return textureLoad16(assetPath(rel), out, psm) || textureLoad16(rel, out, psm);
}

static void touchGuiSetLoaded(MenuState& s, bool want) {
    if (want == s.haveTouch) return;
    if (want) {

        textureForgetFailures();
        s.haveTouch = loadTex(&s.touchGui, "data/images/gui/touchgui.png");
    }
    else { textureFree(&s.touchGui); s.haveTouch = false; }
}

static bool loadFnt(Font* out, const char* rel) {
    return fontLoad(assetPath(rel), out) || fontLoad(rel, out);
}

World g_world;
bool  g_worldBuilt = false;

#include "world/level/level.h"
Level g_level(&g_world);

#include "world/difficulty.h"
int g_difficulty = Difficulty::NORMAL;

#include "world/item/item.h"

int main(int argc, char* argv[]) {
    Item::initItems();
    Tile::initTiles();
    scePowerSetClockFrequency(333, 333, 166);
    setupCallbacks();
    pathInit(argc > 0 ? argv[0] : 0);
    soundInit();
    optionsLoad();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    guInit();

    Texture mojangSplash;
    if (loadTex(&mojangSplash, "data/images/logo.png")) {
        float startTime = nowSeconds();
        while (!g_exitRequested && (nowSeconds() - startTime) < 2.0f) {
            guStartFrame(0xFFFFFFFF);
            guOrtho();
            sceGuDisable(GU_DEPTH_TEST);

            textureBind(&mojangSplash);

            float scaleW = 480.0f / mojangSplash.realW;
            float scaleH = 272.0f / mojangSplash.realH;
            float scale = (scaleW < scaleH) ? scaleW : scaleH;

            float w = mojangSplash.realW * scale;
            float h = mojangSplash.realH * scale;
            float x = (480.0f - w) / 2.0f;
            float y = (272.0f - h) / 2.0f;

            spriteDraw(&mojangSplash, x, y, w, h, 0, 0, mojangSplash.realW, mojangSplash.realH, 0xFFFFFFFF);

            guEndFrame();

            SceCtrlData splashPad;
            sceCtrlReadBufferPositive(&splashPad, 1);
            if (splashPad.Buttons != 0) break;
        }
        textureFree(&mojangSplash);
    }

    detectLowMemPsp();

    static MenuState s;

    s.haveFont         = loadFnt(&s.font, "data/images/font/default8.png");
    s.haveGui          = loadTex4444(&s.guiAtlas, "data/images/gui/gui_game.png");
    s.haveLogo         = loadTex16(&s.logo, "data/images/gui/title.png", GU_PSM_5551);
    s.haveBg           = loadTex(&s.dirtBg, "data/images/gui/background.png");
    s.haveTouch        = loadTex(&s.touchGui, "data/images/gui/touchgui.png");
    s.haveDefaultWorld = loadTex16(&s.defaultWorld, "data/images/gui/default_world.png", GU_PSM_5551);

    loadCharIfNeeded();

    s.screen = SCREEN_TITLE;
    worldListScan(&s.worlds);
    s.worldSelected = 0;
    s.deleteSelected = 1;
    s.createSelected = 1;
    s.newWorldGamemode = 0;
    strcpy(s.newWorldName, "New world");
    s.newWorldSeed[0] = '\0';
    s.joinListSelected = 0;
    s.joinIpSelected = 1;
    s.joinIp[0] = '\0';
    s.uiRow = 1;
    s.topSelected = 0;
    s.listScrollX = 0.0f;
    s.selected = 1;
    s.optFocus = 1;
    s.optCategory = 0;
    s.optTabHighlight = 0;
    s.optItemHighlight = 0;
    s.optEditingRow = -1;
    s.statusMsg[0] = '\0';

    s.statusMsg[0] = '\0';

    SceCtrlData initPad;
    sceCtrlReadBufferPositive(&initPad, 1);
    unsigned int lastBtn = initPad.Buttons;

    float fps = 0.0f;
    float fpsLastTime = nowSeconds();
    int fpsFrames = 0;

    while (!g_exitRequested) {
        float now = nowSeconds();
        fpsFrames++;
        if (now - fpsLastTime >= 1.0f) {
            fps = fpsFrames / (now - fpsLastTime);
            fpsFrames = 0;
            fpsLastTime = now;
        }

        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);

        unsigned int currentBtn = pad.Buttons & ~(PSP_CTRL_HOME | PSP_CTRL_HOLD |
                                                  PSP_CTRL_NOTE | PSP_CTRL_SCREEN |
                                                  PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN);
        unsigned int pressed = currentBtn & ~lastBtn;
        g_heldButtons = currentBtn;
        lastBtn = currentBtn;

        unsigned int repeat = 0;
        {
            static const unsigned int RDIRS = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT;
            static unsigned int s_lastDirs = 0;
            static unsigned int s_holdUs[4] = {0,0,0,0}, s_repUs[4] = {0,0,0,0};
            const unsigned int dirs[4] = {PSP_CTRL_UP, PSP_CTRL_DOWN, PSP_CTRL_LEFT, PSP_CTRL_RIGHT};
            unsigned int nowUs = sceKernelGetSystemTimeLow();
            for (int i = 0; i < 4; i++) {
                if (currentBtn & dirs[i]) {
                    if (!(s_lastDirs & dirs[i])) { s_holdUs[i] = nowUs; s_repUs[i] = nowUs; }
                    else if (nowUs - s_holdUs[i] >= 350000 && nowUs - s_repUs[i] >= 80000) {
                        repeat |= dirs[i]; s_repUs[i] = nowUs;
                    }
                }
            }
            s_lastDirs = currentBtn & RDIRS;
        }

        if (menuOskUpdate(s)) continue;

        if (pressed & PSP_CTRL_START) {
            if (s.screen == SCREEN_TITLE) {
                s.screen = SCREEN_WORLDS;
                s.statusMsg[0] = '\0';
            } else if (s.screen == SCREEN_WORLDS) {
                if (s.worldSelected < s.worlds.count) {
                    std::snprintf(s.statusMsg, sizeof(s.statusMsg), "Loading: %s", s.worlds.names[s.worldSelected]);
                    s.screen = SCREEN_GAME;
                }
            } else if (s.screen != SCREEN_GAME) {
                break;
            }
        }

        if (pressed & PSP_CTRL_SELECT) {
            if (s.screen == SCREEN_WORLDS) {
                s.createSelected = 1;
                s.newWorldName[0] = '\0';
                s.newWorldSeed[0] = '\0';
                s.newWorldGamemode = 0;
                s.screen = SCREEN_CREATE;
            }
        }

        const unsigned int NAV = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT
                               | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER;
        bool navOnly = pressed && (pressed & ~NAV) == 0;
        unsigned int sigBefore = menuSelectionSig(s);
        AppScreen screenBefore = s.screen;

        extern bool g_invOpen, g_chestOpen, g_furnaceOpen, g_craftOpen, g_armorOpen, g_paused;
        bool inGameMenu = g_invOpen || g_chestOpen || g_furnaceOpen || g_craftOpen || g_armorOpen || g_paused;
        unsigned int pMenu = pressed | repeat;
        switch (s.screen) {
            case SCREEN_TITLE:   titleHandleInput(s, pMenu);   break;
            case SCREEN_WORLDS:  worldsHandleInput(s, pMenu);  break;
            case SCREEN_DELETE:  deleteHandleInput(s, pMenu);  break;
            case SCREEN_CREATE:  createHandleInput(s, pMenu);  break;
            case SCREEN_JOIN:    joinHandleInput(s, pMenu);    break;
            case SCREEN_JOIN_IP: joinIpHandleInput(s, pMenu);  break;
            case SCREEN_OPTIONS: optionsHandleInput(s, pMenu); break;
            case SCREEN_GAME:    gameUpdate(s, inGameMenu ? pMenu : pressed, pad); break;
        }

        if (pressed && screenBefore != SCREEN_GAME &&
            (!navOnly || menuSelectionSig(s) != sigBefore))
            soundPlay("random.click", 1.0f, 1.0f);

        touchGuiSetLoaded(s, s.screen != SCREEN_GAME);
        guStartFrame(s.screen == SCREEN_GAME ? g_skyColorNow : 0xFF000000u);

        if (s.screen == SCREEN_GAME) {
            gameRender(s);

            guOrtho();
            sceGuDisable(GU_DEPTH_TEST);

            if (gameProgressScreenUp()) { guEndFrame(); continue; }
            extern bool g_invOpen;
            extern int g_showFps, g_showCoords;
            extern bool g_photoPending;
            if (!g_invOpen && !g_photoPending) {

                float ty = 10.0f;
                if (g_showFps) {
                    char fpsBuf[32];
                    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d", (int)(fps + 0.5f));
                    fontDrawTextShadow(&s.font, 10, ty, fpsBuf, 0xFFE0E0E0u, 1.0f);
                    ty += 12.0f;
                }

                if (g_showCoords && g_worldBuilt && g_level.player) {
                    char posBuf[48];
                    std::snprintf(posBuf, sizeof(posBuf), "X %d  Y %d  Z %d",
                                  (int)floorf(g_level.player->x),
                                  (int)floorf(g_level.player->y - 1.62f),
                                  (int)floorf(g_level.player->z));
                    fontDrawTextShadow(&s.font, 10, ty, posBuf, 0xFFE0E0E0u, 1.0f);
                }
            }

            extern bool g_paused;

            if (g_craftOpen) craftRender(s);
            if (g_armorOpen) armorRender(s);
            if (g_furnaceOpen) furnaceRender(s);
            if (g_chestOpen) chestRender(s);
            if (g_worldBuilt && g_level.player && g_level.player->isSleeping())
                inBedRender(s);
            if (g_paused) pauseRender(s);

            if (g_deadScreen) deadRender(s);

            if (g_signEditing) signRender(s);

            gameHintsDraw(s);

            sceGuEnable(GU_DEPTH_TEST);

            {
                extern bool g_photoPending;
                extern Entity* g_photoCamera;
                if (g_photoPending) {
                    guFinishFrame();
                    sceIoMkdir(assetPath("photos"), 0777);
                    char rel[64], full[320];
                    for (int i = 0; i < 10000; i++) {
                        std::snprintf(rel, sizeof(rel), "photos/img_%04d.png", i);
                        std::strncpy(full, assetPath(rel), sizeof(full) - 1);
                        full[sizeof(full) - 1] = '\0';
                        FILE* probe = fopen(full, "rb");
                        if (!probe) break;
                        fclose(probe);
                    }
                    guSavePhotoPng(full);
                    g_photoPending = false;
                    g_photoCamera = 0;

                    continue;
                }
            }

            guEndFrame();
            continue;
        }

        switch (s.screen) {
            case SCREEN_TITLE:   titleRender(s);   break;
            case SCREEN_WORLDS:  worldsRender(s);  break;
            case SCREEN_DELETE:  deleteRender(s);  break;
            case SCREEN_CREATE:  createRender(s);  break;
            case SCREEN_JOIN:    joinRender(s);    break;
            case SCREEN_JOIN_IP: joinIpRender(s);  break;
            case SCREEN_OPTIONS: optionsRender(s); break;
        }
        menuHintsDraw(s);

        guEndFrame();
    }

    if (s.haveFont)  fontFree(&s.font);
    if (s.haveGui)   textureFree(&s.guiAtlas);
    if (s.haveLogo)  textureFree(&s.logo);
    if (s.haveBg) textureFree(&s.dirtBg);
    if (s.haveTouch) textureFree(&s.touchGui);

    guTerm();
    sceKernelExitGame();
    return 0;
}
