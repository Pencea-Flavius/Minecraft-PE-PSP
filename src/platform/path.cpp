#include "platform/path.h"

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
