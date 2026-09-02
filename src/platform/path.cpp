#include "platform/path.h"

#include <pspiofilemgr.h>

#include <cstdio>
#include <cstring>

static char g_base[256] = "ms0:/PSP/GAME/MCPSP/";

void pathInit(const char* argv0) {
    if (!argv0 || !argv0[0])
        return;
    strncpy(g_base, argv0, sizeof(g_base) - 1);
    g_base[sizeof(g_base) - 1] = '\0';

    char* slash = strrchr(g_base, '/');
    if (slash)
        slash[1] = '\0';
}

const char* assetPath(const char* rel) {
    static char buf[320];
    snprintf(buf, sizeof(buf), "%s%s", g_base, rel);
    return buf;
}

const char* savePath(const char* rel) {
    return assetPath(rel);
}

const char* pathDevice(void) {
    static char dev[16];
    const char* colon = strchr(g_base, ':');
    if (!colon || (size_t)(colon - g_base) >= sizeof(dev) - 1) return "ms0:";
    const size_t n = (size_t)(colon - g_base) + 1;
    memcpy(dev, g_base, n);
    dev[n] = '\0';
    return dev;
}

void savePathInit(void) {
    sceIoMkdir(savePath("saves"), 0777);
}
