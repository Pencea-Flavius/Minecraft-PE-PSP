
#include <pspkernel.h>
#include <pspsysmem.h>
#include <pspiofilemgr.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspfpu.h>
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
#include "util/prof.h"
#include "platform/audio/sound.h"
#include "world/level/storage/worldlist.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/chunk/chunk_cache.h"
#include "world/level/storage/chunk_storage.h"
#include "world/level/tile/tile.h"
#include "client/gui/screens/menu.h"
#include "client/gui/screens/control_scheme.h"
#include "client/gui/screens/screen.h"
#include "client/gui/screens/panorama.h"
#include "client/gui/screens/world_icons.h"
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
float    g_gameSeconds = 0.0f;
bool     g_gameFrozen  = true;

PSP_MODULE_INFO("MinecraftPSP", 0, 0, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

PSP_HEAP_SIZE_KB(-1024);

int g_pspIsGo = 0;
static void detectPspGo(void) {
    SceUID d = sceIoDopen("ef0:/");
    if (d >= 0) { sceIoDclose(d); g_pspIsGo = 1; }
}

int g_lowMemPsp  = 0;
int g_lowMemHeap = 0;
static void detectLowMemPsp(void) {
    enum { MAX_BLOCKS = 64 };
    void* blocks[MAX_BLOCKS];
    int n = 0;
    while (n < MAX_BLOCKS) { void* p = malloc(1024 * 1024); if (!p) break; blocks[n++] = p; }
    for (int i = 0; i < n; i++) free(blocks[i]);

    int stillFreeMB = (int)(sceKernelTotalFreeMemSize() / (1024u * 1024u));
    g_lowMemHeap = (n < 32);
    g_lowMemPsp  = (n + stillFreeMB) < 32;
}

static volatile int g_exitRequested = 0;

static volatile int g_powerResumed = 0;

static int powerCallback(int , int pwrflags, void* ) {
    if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE)
        g_powerResumed = 1;
    return 0;
}

static float drawFaultCounters(MenuState& s, float ty) {
    extern unsigned int g_listPeakBytes, g_listOverruns;
    char buf[160];

    if (g_canaryBroken) {
        std::snprintf(buf, sizeof(buf), "GE-LIST CANARY BROKEN +%u", g_canaryBroken - 1);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF0000FFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_callCanaryBroken;
    if (g_callCanaryBroken) {
        std::snprintf(buf, sizeof(buf), "GU-STATE LIST OVERRAN +%u", g_callCanaryBroken - 1);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF0000FFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_listBadFinish, g_listBadFinishSite;
    extern int g_listBadFinishRet;
    if (g_listBadFinish) {
        static const char* kSite[] = { "?", "FRAME", "DIALOG", "RESUME", "PHOTO" };
        const unsigned si = (g_listBadFinishSite < 5u) ? g_listBadFinishSite : 0u;
        std::snprintf(buf, sizeof(buf), "GE-LIST BAD FINISH %u @%s 0x%08X",
                      g_listBadFinish, kSite[si], (unsigned)g_listBadFinishRet);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_guStompPhase, g_guStompWhat, g_guStompCount, g_guRepairCount;
    if (g_guStompPhase) {
        static const char* kPhase[] = { "?", "START", "TERRAIN", "WATER",
                                        "ENTITY", "2D", "PHOTO" };
        const unsigned ph = (g_guStompPhase < 7u) ? g_guStompPhase : 0u;
        std::snprintf(buf, sizeof(buf), "GU-GLOBALS STOMPED @%s w%u x%u",
                      kPhase[ph], g_guStompWhat, g_guStompCount);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF0000FFu, 1.0f);
        ty += 12.0f;
    }
    if (g_guRepairCount) {
        std::snprintf(buf, sizeof(buf), "GU-GLOBALS REPAIRED %u", g_guRepairCount);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }
    if (g_listOverruns) {
        std::snprintf(buf, sizeof(buf), "GE-LIST %uK/512K OVERRUN", g_listPeakBytes / 1024);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_vcSameRefresh, g_vcDrops;
    extern int g_vcLast, g_vcMin, g_vcMax;
    if (g_vcSameRefresh) {
        std::snprintf(buf, sizeof(buf), "PRESENT x2/REFRESH %u  d=%d %d..%d  drop %u",
                      g_vcSameRefresh, g_vcLast, g_vcMin, g_vcMax, g_vcDrops);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF50FFFFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_musCutShort, g_musCutLeftMs, g_musLoopStops;
    if (g_musCutShort) {
        std::snprintf(buf, sizeof(buf), "MUSIC CUT %u  owed %ums  LOOP %u",
                      g_musCutShort, g_musCutLeftMs, g_musLoopStops);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF50FFFFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_frameAllocFails, g_frameAllocListFails;
    if (g_frameAllocFails) {

        std::snprintf(buf, sizeof(buf), "GU-SCRATCH FULL %u LIST %u",
                      g_frameAllocFails, g_frameAllocListFails);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF50FFFFu, 1.0f);
        ty += 12.0f;
    }

#if MESH_RESERVE_CHECK
    extern unsigned int g_sinkOverruns;
    if (g_sinkOverruns) {
        std::snprintf(buf, sizeof(buf), "MESH RESERVE %u", g_sinkOverruns);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF0000FFu, 1.0f);
        ty += 12.0f;
    }
#endif

    extern unsigned int g_deferStalls;
    if (g_deferStalls) {
        std::snprintf(buf, sizeof(buf), "GU-DEFER STALL %u", g_deferStalls);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF50FFFFu, 1.0f);
        ty += 12.0f;
    }
    extern unsigned int g_drawLiveHits, g_drawLiveDrv;
    if (g_drawLiveHits) {

        std::snprintf(buf, sizeof(buf), "DRAW-LIVE %u/%u", g_drawLiveHits, g_drawLiveDrv);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF50FFFFu, 1.0f);
        ty += 12.0f;
    }
    extern unsigned int g_textureBindFailures;
    if (g_textureBindFailures) {
        std::snprintf(buf, sizeof(buf), "TEX FAIL %u", g_textureBindFailures);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }

    extern unsigned int g_textureLoadFailures;
    extern const char*  g_textureFailReason;
    extern char g_textureLastFailed[80];
    if (g_textureLoadFailures) {
        const char* base = std::strrchr(g_textureLastFailed, '/');

        extern unsigned int g_textureFailHeapUsed, g_textureFailHeapBig;
        std::snprintf(buf, sizeof(buf), "TEX %u %.32s (%s) big %uK used %uK vram %uK",
                      g_textureLoadFailures,
                      base ? base + 1 : g_textureLastFailed,
                      g_textureFailReason,
                      g_textureFailHeapBig / 1024, g_textureFailHeapUsed / 1024,
                      guVramFree() / 1024);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }

    extern World g_world;
    if (g_blockOomDrops || g_world.lightOomDrops) {
        std::snprintf(buf, sizeof(buf), "STORAGE OOM block %u light %u",
                      g_blockOomDrops, g_world.lightOomDrops);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF4040FFu, 1.0f);
        ty += 12.0f;
    }
    extern bool g_haveTerrain, g_worldBuilt;
    if (g_worldBuilt && !g_haveTerrain) {
        fontDrawTextShadow(&s.font, 10, ty,
                           "NO TERRAIN ATLAS (data/images/terrain.png)",
                           0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }
    return ty;
}

static int exitCallback(int , int , void* ) {
    g_exitRequested = 1;
    return 0;
}

static int callbackThread(SceSize , void* ) {
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, 0);
    sceKernelRegisterExitCallback(cbid);

    int pcb = sceKernelCreateCallback("Power Callback", powerCallback, 0);

    if (pcb >= 0) scePowerRegisterCallback(-1, pcb);
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

static bool loadTex16(Texture* out, const char* rel, int psm) {
    return textureLoad16(assetPath(rel), out, psm) || textureLoad16(rel, out, psm);
}

static bool loadTexVram(Texture* out, const char* rel, int psm) {
    return textureLoadVram(assetPath(rel), out, psm) || textureLoadVram(rel, out, psm);
}

static bool screenNeedsTouchGui(int screen, bool worldLoaded) {
    if (screen == SCREEN_OPTIONS) return !worldLoaded;
    return screen == SCREEN_TITLE  || screen == SCREEN_WORLDS ||
           screen == SCREEN_DELETE || screen == SCREEN_CREATE ||
           screen == SCREEN_JOIN   || screen == SCREEN_ADD_SERVER;
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

#define MEM_OVERLAY 0

int main(int argc, char* argv[]) {
    Item::initItems();
    Tile::initTiles();
    scePowerSetClockFrequency(333, 333, 166);

    pspFpuSetEnable(0);
    setupCallbacks();
    pathInit(argc > 0 ? argv[0] : 0);
    savePathInit();

    detectLowMemPsp();
    detectPspGo();
    soundInit();
    optionsLoad();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    guInit();

    static const float INTRO_WHITE    = 0.9f;
    static const float INTRO_FADE_IN  = 0.7f;
    static const float INTRO_HOLD     = 1.1f;
    static const float INTRO_FADE_OUT = 0.7f;
    static const float INTRO_LOGO_UP  = INTRO_WHITE + INTRO_FADE_IN;
    static const float INTRO_DARK_AT  = INTRO_LOGO_UP + INTRO_HOLD;
    static const float INTRO_END      = INTRO_DARK_AT + INTRO_FADE_OUT;

    Texture mojangSplash;
    if (loadTex(&mojangSplash, "data/images/logo.png")) {
        const float startTime = nowSeconds();
        float skipAt = -1.0f;
        while (!g_exitRequested) {
            float t = nowSeconds() - startTime;

            if (skipAt >= 0.0f) t = INTRO_DARK_AT + (nowSeconds() - skipAt);
            if (t >= INTRO_END) break;

            unsigned int ground = 0xFFFFFFFFu;
            float dark = 0.0f;
            if (t > INTRO_DARK_AT) {
                dark = (t - INTRO_DARK_AT) / INTRO_FADE_OUT;
                if (dark > 1.0f) dark = 1.0f;
                const unsigned int g = (unsigned int)(255.0f * (1.0f - dark));
                ground = 0xFF000000u | (g << 16) | (g << 8) | g;
            }

            scePowerTick(0);
            if (!guStartFrame(ground)) continue;
            guOrtho();
            sceGuDisable(GU_DEPTH_TEST);

            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

            float scaleW = 480.0f / mojangSplash.realW;
            float scaleH = 272.0f / mojangSplash.realH;
            float scale  = (scaleW < scaleH) ? scaleW : scaleH;
            float w = mojangSplash.realW * scale, h = mojangSplash.realH * scale;
            float x = (480.0f - w) / 2.0f,        y = (272.0f - h) / 2.0f;

            float a = 0.0f;
            if (t >= INTRO_WHITE)   a = (t - INTRO_WHITE) / INTRO_FADE_IN;
            if (a > 1.0f)           a = 1.0f;
            a *= (1.0f - dark);
            if (a > 0.0f) {
                const unsigned int al = (unsigned int)(255.0f * a);
                textureBind(&mojangSplash);
                spriteDraw(&mojangSplash, x, y, w, h,
                           0, 0, mojangSplash.realW, mojangSplash.realH,
                           (al << 24) | 0x00FFFFFFu);
            }
            guEndFrame();

            if (skipAt < 0.0f && t >= INTRO_WHITE) {
                SceCtrlData splashPad;
                sceCtrlReadBufferPositive(&splashPad, 1);
                if (splashPad.Buttons & PSP_CTRL_CROSS) skipAt = nowSeconds();
            }
        }
        textureFree(&mojangSplash);
    }

    static MenuState s;

    s.haveFont         = loadFnt(&s.font, "data/images/font/default8.png");

    s.haveGui          = loadTexVram(&s.guiAtlas, "data/images/gui/gui_game.png", GU_PSM_4444);

    g_btnIconsHave     = loadTexVram(&g_btnIcons, "data/images/gui/tooltips.png", GU_PSM_5551);

    extern bool g_haveGuiBlocks;
    extern Texture g_guiBlocks;
    if (!g_haveGuiBlocks)
        g_haveGuiBlocks = loadTexVram(&g_guiBlocks, "data/images/gui/gui_blocks.png", GU_PSM_5551);
    s.haveLogo         = loadTex16(&s.logo, "data/images/gui/title.png", GU_PSM_5551);
    s.haveBg           = loadTex(&s.dirtBg, "data/images/gui/background.png");
    s.haveTouch        = loadTex(&s.touchGui, "data/images/gui/touchgui.png");
    s.haveDefaultWorld = loadTex16(&s.defaultWorld, "data/images/gui/default_world.png", GU_PSM_5551);

    loadCharIfNeeded();

    s.screen = SCREEN_TITLE;
    worldListScan(&s.worlds);
    s.worldSelected = 0;
    s.deleteSelected = 1;
    createFormReset(s);
    joinListReset(s);
    addServerFormReset(s);
    s.uiRow = 1;
    s.topSelected = 0;
    s.listScrollX = 0.0f;
    s.selected = 1;
    s.optFocus = 1;
    s.optCategory = 0;
    s.optTabHighlight = 0;
    s.optItemHighlight = 0;
    s.optScroll = 0.0f;
    s.statusMsg[0] = '\0';

    s.statusMsg[0] = '\0';

    SceCtrlData initPad;
    sceCtrlReadBufferPositive(&initPad, 1);
    unsigned int lastBtn = initPad.Buttons;

    float fps = 0.0f;
    float fpsLastTime = nowSeconds();
    int fpsFrames = 0;

    while (!g_exitRequested) {

        if (g_powerResumed) {
            g_powerResumed = 0;

            guResumeFromSleep();
            chunkStorageDropOpenFiles();

            extern bool g_worldBuilt;
            if (g_worldBuilt) {
                extern int g_autosaveTick;
                g_saveRequested = true;
                g_autosaveTick  = 0;
            }

            soundPowerResume();
            scePowerTick(0);
        }

        float now = nowSeconds();

        if (now - fpsLastTime >= 1.0f) {
            fps = fpsFrames / (now - fpsLastTime);
            fpsFrames = 0;
            fpsLastTime = now;
        }

        scePowerTick(0);

        SceCtrlData pad;
        sceCtrlPeekBufferPositive(&pad, 1);

        controlSchemeNotePad(pad.Buttons, pad.Rx, pad.Ry);

        const bool systemFrame = (pad.Buttons & (PSP_CTRL_HOME | PSP_CTRL_HOLD)) != 0;
        if (systemFrame) {
            pad.Buttons = 0;
            pad.Lx = pad.Ly = 128;
            pad.Rx = pad.Ry = 128;
        }

        pad.Buttons &= ~(PSP_CTRL_HOME | PSP_CTRL_HOLD |
                         PSP_CTRL_NOTE | PSP_CTRL_SCREEN |
                         PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN |
                         PSP_CTRL_WLAN_UP | PSP_CTRL_REMOTE |
                         PSP_CTRL_DISC | PSP_CTRL_MS);
        unsigned int currentBtn = pad.Buttons;

        extern bool g_invOpen, g_chestOpen, g_furnaceOpen, g_craftOpen, g_armorOpen;
        extern bool g_paused, g_optionsOpen;
        bool inGameMenu = g_invOpen || g_chestOpen || g_furnaceOpen || g_craftOpen ||
                          g_armorOpen || g_paused || g_optionsOpen;

        if (menuScreen(s.screen) || inGameMenu) {

            const int DZ = 48;
            if (pad.Lx < 128 - DZ) currentBtn |= PSP_CTRL_LEFT;
            if (pad.Lx > 128 + DZ) currentBtn |= PSP_CTRL_RIGHT;
            if (pad.Ly < 128 - DZ) currentBtn |= PSP_CTRL_UP;
            if (pad.Ly > 128 + DZ) currentBtn |= PSP_CTRL_DOWN;
        }

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
            }

        }

        if (pressed & PSP_CTRL_SELECT) {
            if (s.screen == SCREEN_WORLDS) {
                createFormReset(s);
                s.screen = SCREEN_CREATE;
            }
        }

        const unsigned int NAV = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT
                               | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER;
        bool navOnly = pressed && (pressed & ~NAV) == 0;
        unsigned int sigBefore = menuSelectionSig(s);
        AppScreen screenBefore = s.screen;

        unsigned int pMenu = pressed | repeat;
        if (Screen* cur = menuScreen(s.screen)) {

            cur->handleInput(s, menuFaceSwap(controlSchemeMenuAlias(pMenu)),
                             menuFaceSwap(controlSchemeMenuAlias(pad.Buttons)));
        } else {

            gameUpdate(s, inGameMenu ? pMenu
                       : (pressed | (repeat & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT))), pad);
        }

        if (pressed && (screenBefore != SCREEN_GAME || g_optionsOpen) &&
            (!navOnly || menuSelectionSig(s) != sigBefore))
            soundPlay("random.click", 1.0f, 1.0f);

        touchGuiSetLoaded(s, screenNeedsTouchGui(s.screen, g_worldBuilt));

        panoramaSetLoaded(s.screen != SCREEN_GAME && !g_worldBuilt);

        worldIconsSetLoaded(s.screen == SCREEN_WORLDS || s.screen == SCREEN_DELETE);

        if (!guStartFrame(s.screen == SCREEN_GAME ? g_skyColorNow : 0xFF000000u)) continue;
        fpsFrames++;

        if (s.screen == SCREEN_GAME) {
            gameRender(s);

            guOrtho();
            sceGuDisable(GU_DEPTH_TEST);

            { extern bool g_photoPending;
              if (gameProgressScreenUp() && !g_photoPending) { guEndFrame(); continue; } }
            extern bool g_invOpen;
            extern int g_showFps, g_showCoords;
            extern int g_hideGui;
            extern bool g_photoPending;

            if (!g_invOpen && !g_photoPending && !g_hideGui) {

                float ty = 10.0f;
                if (g_showFps) {
                    char fpsBuf[32];
#if PROF

                    extern int g_profLines;
                    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d  P%d", (int)(fps + 0.5f), g_profLines);
#else
                    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d", (int)(fps + 0.5f));
#endif
                    fontDrawTextShadow(&s.font, 10, ty, fpsBuf, 0xFFE0E0E0u, 1.0f);
                    ty += 12.0f;
                }

                {
#if MCPSP_DIAG

                    {
                        extern unsigned int g_drawLiveHits, g_drawLiveOurs;
                        if (g_drawLiveHits) {
                            char dlBuf[64];

                            extern unsigned int g_drawLiveDrv;
                            std::snprintf(dlBuf, sizeof(dlBuf), "DRAW-LIVE %u/%u drv %u",
                                          g_drawLiveHits, g_drawLiveOurs, g_drawLiveDrv);
                            fontDrawTextShadow(&s.font, 10, ty, dlBuf, 0xFF50FFFFu, 1.0f);
                            ty += 12.0f;
                        }
                    }

                    {
                        extern unsigned int g_listPeakBytes, g_listOverruns;

    extern unsigned int g_callCanaryBroken;
    if (g_callCanaryBroken) {
        std::snprintf(buf, sizeof(buf), "GU-STATE LIST OVERRAN +%u", g_callCanaryBroken - 1);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF0000FFu, 1.0f);
        ty += 12.0f;
    }
    extern unsigned int g_listBadFinish;
    if (g_listBadFinish) {
        std::snprintf(buf, sizeof(buf), "GE-LIST BAD FINISH %u", g_listBadFinish);
        fontDrawTextShadow(&s.font, 10, ty, buf, 0xFF5050FFu, 1.0f);
        ty += 12.0f;
    }
    if (g_listOverruns) {
                            char lbBuf[64];
                            std::snprintf(lbBuf, sizeof(lbBuf), "GE-LIST %uK/512K OVERRUN",
                                          g_listPeakBytes / 1024);
                            fontDrawTextShadow(&s.font, 10, ty, lbBuf, 0xFF5050FFu, 1.0f);
                            ty += 12.0f;
                        }
                    }
#endif

#if MEM_OVERLAY

                    if (g_worldBuilt) {
                        struct mallinfo mi = mallinfo();
                        char memBuf[96];

                        std::snprintf(memBuf, sizeof(memBuf),
                                      "MEM used %.1fM  world %.1fM (blk %.2fM data %.2fM light %.2fM)",
                                      (unsigned int)mi.uordblks / 1048576.0f,
                                      worldMemBytes(&g_world) / 1048576.0f,
                                      blockBytes(&g_world) / 1048576.0f,
                                      worldDataBytes(&g_world) / 1048576.0f,
                                      lightBytes(&g_world) / 1048576.0f);
                        fontDrawTextShadow(&s.font, 10, ty, memBuf, 0xFFE0E0E0u, 1.0f);
                        ty += 12.0f;

                        {
                            static int su = 0, sp = 0, sr = 0;
                            static unsigned int lastTick = 0;
                            unsigned int now = sceKernelGetSystemTimeLow();
                            if (!lastTick || now - lastTick > 1000000u) {
                                blockStats(&g_world, &su, &sp, &sr);
                                lastTick = now;
                            }
                            char secBuf[64];
                            std::snprintf(secBuf, sizeof(secBuf),
                                          "SEC free %d  pal %d  raw %d  (of %d)",
                                          su, sp, sr, BS_SECTIONS);
                            fontDrawTextShadow(&s.font, 10, ty, secBuf, 0xFFE0E0E0u, 1.0f);
                            ty += 12.0f;
                        }
                        {
                            extern float g_viewDist, g_viewDistEff;
                            if (g_viewDistEff > 0.0f && g_viewDistEff < g_viewDist) {
                                char rdBuf[48];
                                std::snprintf(rdBuf, sizeof(rdBuf), "RENDER DIST -> %d (low memory)",
                                              (int)g_viewDistEff);
                                fontDrawTextShadow(&s.font, 10, ty, rdBuf, 0xFF40C0FFu, 1.0f);
                                ty += 12.0f;
                            }
                        }

                        if (g_world.lightOomDrops) {
                            char oomBuf[48];
                            std::snprintf(oomBuf, sizeof(oomBuf), "LIGHT OOM %u",
                                          g_world.lightOomDrops);
                            fontDrawTextShadow(&s.font, 10, ty, oomBuf, 0xFF4040FFu, 1.0f);
                            ty += 12.0f;
                        }

                        if (g_blockOomDrops) {
                            char oomBuf[48];
                            std::snprintf(oomBuf, sizeof(oomBuf), "BLOCK OOM %u", g_blockOomDrops);
                            fontDrawTextShadow(&s.font, 10, ty, oomBuf, 0xFF4040FFu, 1.0f);
                            ty += 12.0f;
                        }
                    }
#endif
                }

                ty = drawFaultCounters(s, ty);

                if (g_showCoords && g_worldBuilt && g_level.player) {
                    char posBuf[48];
                    std::snprintf(posBuf, sizeof(posBuf), "X %d  Y %d  Z %d",
                                  (int)floorf(g_level.player->x),
                                  (int)floorf(g_level.player->y - 1.62f),
                                  (int)floorf(g_level.player->z));
                    fontDrawTextShadow(&s.font, 10, ty, posBuf, 0xFFE0E0E0u, 1.0f);
                }
            }

            if (!g_photoPending)
                if (Screen* over = overlayScreen()) { over->render(s); }

            gameHintsDraw(s);

            sceGuEnable(GU_DEPTH_TEST);

            {
                extern bool g_photoPending;
                extern Entity* g_photoCamera;
                extern bool g_photoIsIcon;
                extern char g_photoIconPath[320];
                if (g_photoPending && g_photoIsIcon) {

                    guFinishFrame();
                    guSavePhotoPng(g_photoIconPath, 4);
                    g_photoPending = false;
                    g_photoIsIcon  = false;
                    continue;
                }
                if (g_photoPending) {
                    guFinishFrame();

                    sceIoMkdir("ms0:/PSP", 0777);
                    sceIoMkdir("ms0:/PSP/PHOTO", 0777);
                    sceIoMkdir("ms0:/PSP/PHOTO/Minecraft", 0777);
                    char full[320];
                    for (int i = 0; i < 10000; i++) {
                        std::snprintf(full, sizeof(full),
                                      "ms0:/PSP/PHOTO/Minecraft/img_%04d.png", i);
                        FILE* probe = fopen(full, "rb");
                        if (!probe) break;
                        fclose(probe);
                    }
                    if (!guSavePhotoPng(full, 1)) {
                        sceIoMkdir(assetPath("photos"), 0777);
                        char rel[64];
                        for (int i = 0; i < 10000; i++) {
                            std::snprintf(rel, sizeof(rel), "photos/img_%04d.png", i);
                            std::strncpy(full, assetPath(rel), sizeof(full) - 1);
                            full[sizeof(full) - 1] = '\0';
                            FILE* probe = fopen(full, "rb");
                            if (!probe) break;
                            fclose(probe);
                        }
                        guSavePhotoPng(full, 1);
                    }
                    g_photoPending = false;
                    g_photoCamera = 0;

                    continue;
                }
            }

            guEndFrame();
            continue;
        }

        if (Screen* cur = menuScreen(s.screen)) { cur->render(s); }
        menuHintsDraw(s);

        drawFaultCounters(s, 10.0f);

        guEndFrame();
    }

    soundShutdown();

    if (g_worldBuilt) releaseWorldAndPlayer();
    worldGenWorkerStop();

    if (s.haveFont)  fontFree(&s.font);
    if (s.haveGui)   textureFree(&s.guiAtlas);
    if (g_btnIconsHave) textureFree(&g_btnIcons);
    if (s.haveLogo)  textureFree(&s.logo);
    if (s.haveBg) textureFree(&s.dirtBg);
    if (s.haveTouch) textureFree(&s.touchGui);
    panoramaSetLoaded(false);

    guTerm();
    sceKernelExitGame();
    return 0;
}
