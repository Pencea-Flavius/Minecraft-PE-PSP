#include "world/level/storage/worldlist.h"
#include "world/level/storage/level_storage.h"

#include "platform/path.h"

#include <pspiofilemgr.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

long worldSeedFromString(const char* str) {
    if (!str || str[0] == '\0')
        return (long)time(NULL);

    const char* p = str;
    if (*p == '-' || *p == '+') p++;
    bool numeric = (*p != '\0');
    for (const char* q = p; *q; q++)
        if (*q < '0' || *q > '9') { numeric = false; break; }
    if (numeric)
        return strtol(str, NULL, 10);

    unsigned int h = 0;
    for (const char* q = str; *q; q++)
        h = h * 31u + (unsigned char)*q;
    return (long)(int)h;
}

void worldListScan(WorldList* out) {
    out->count = 0;

    const char* dir = assetPath("saves");
    sceIoMkdir(dir, 0777);

    SceUID d = sceIoDopen(dir);
    if (d < 0)
        return;

    SceIoDirent entry;
    memset(&entry, 0, sizeof(entry));
    while (sceIoDread(d, &entry) > 0 && out->count < MCPSP_MAX_WORLDS) {
        if (FIO_S_ISDIR(entry.d_stat.st_mode) &&
            strcmp(entry.d_name, ".") != 0 && strcmp(entry.d_name, "..") != 0) {
            strncpy(out->names[out->count], entry.d_name, sizeof(out->names[0]) - 1);
            out->names[out->count][sizeof(out->names[0]) - 1] = '\0';

            ScePspDateTime* t = &entry.d_stat.sce_st_mtime;
            snprintf(out->dates[out->count], sizeof(out->dates[0]), "%02d/%02d/%04d %02d:%02d:%02d",
                     t->month, t->day, t->year, t->hour, t->minute, t->second);

            out->gameModes[out->count] = 0;
            out->seeds[out->count] = 0;
            strncpy(out->displayNames[out->count], entry.d_name, sizeof(out->displayNames[0]) - 1);
            out->displayNames[out->count][sizeof(out->displayNames[0]) - 1] = '\0';

            char infoPath[320];
            snprintf(infoPath, sizeof(infoPath), "saves/%s/level.txt", entry.d_name);
            SceUID fd = sceIoOpen(assetPath(infoPath), PSP_O_RDONLY, 0777);
            if (fd < 0) {

                char dirRel[320];
                snprintf(dirRel, sizeof(dirRel), "saves/%s", entry.d_name);
                char dirAbs[320];
                strncpy(dirAbs, assetPath(dirRel), sizeof(dirAbs) - 1);
                dirAbs[sizeof(dirAbs) - 1] = '\0';
                int gm = 0; long sd = 0;
                if (LevelStorage::readInfo(dirAbs, out->displayNames[out->count],
                                           sizeof(out->displayNames[0]), &gm, &sd)) {
                    out->gameModes[out->count] = gm;
                    out->seeds[out->count] = sd;
                    if (out->displayNames[out->count][0] == '\0') {
                        strncpy(out->displayNames[out->count], entry.d_name, sizeof(out->displayNames[0]) - 1);
                        out->displayNames[out->count][sizeof(out->displayNames[0]) - 1] = '\0';
                    }
                }
            }
            if (fd >= 0) {
                char buf[256] = {0};
                int n = sceIoRead(fd, buf, sizeof(buf) - 1);
                sceIoClose(fd);
                if (n > 0) {
                    buf[n] = '\0';
                    out->gameModes[out->count] = atoi(buf);

                    char* nl = strchr(buf, '\n');
                    if (nl && *(nl + 1) != '\0') {
                        char* name = nl + 1;
                        char* end = strchr(name, '\n');
                        if (end && *(end + 1) != '\0') out->seeds[out->count] = strtol(end + 1, NULL, 10);
                        if (end) *end = '\0';
                        if (name[0] != '\0') {
                            strncpy(out->displayNames[out->count], name, sizeof(out->displayNames[0]) - 1);
                            out->displayNames[out->count][sizeof(out->displayNames[0]) - 1] = '\0';
                        }
                    }
                }
            }

            out->count++;
        }
        memset(&entry, 0, sizeof(entry));
    }
    sceIoDclose(d);

    long keys[MCPSP_MAX_WORLDS];
    for (int i = 0; i < out->count; i++) {
        keys[i] = 0;
        SceIoStat st;
        char p[320];
        snprintf(p, sizeof(p), "saves/%s/lastplayed", out->names[i]);
        int ok = sceIoGetstat(assetPath(p), &st) >= 0;
        if (!ok) {
            snprintf(p, sizeof(p), "saves/%s/level.dat", out->names[i]);
            ok = sceIoGetstat(assetPath(p), &st) >= 0;
        }
        if (ok) {
            ScePspDateTime* t = &st.sce_st_mtime;
            keys[i] = ((((long)t->year * 12 + t->month) * 31 + t->day) * 24 + t->hour) * 3600
                      + t->minute * 60 + t->second;
        }
    }
    for (int i = 1; i < out->count; i++)
        for (int j = i; j > 0 && keys[j] > keys[j - 1]; j--) {
            long k = keys[j]; keys[j] = keys[j - 1]; keys[j - 1] = k;
            char tmp[64];
            memcpy(tmp, out->names[j], 64);        memcpy(out->names[j], out->names[j - 1], 64);               memcpy(out->names[j - 1], tmp, 64);
            memcpy(tmp, out->displayNames[j], 64); memcpy(out->displayNames[j], out->displayNames[j - 1], 64); memcpy(out->displayNames[j - 1], tmp, 64);
            memcpy(tmp, out->dates[j], 64);        memcpy(out->dates[j], out->dates[j - 1], 64);               memcpy(out->dates[j - 1], tmp, 64);
            int g = out->gameModes[j]; out->gameModes[j] = out->gameModes[j - 1]; out->gameModes[j - 1] = g;
            long sd = out->seeds[j];   out->seeds[j] = out->seeds[j - 1];         out->seeds[j - 1] = sd;
        }
}

void worldListTouch(const char* absDir) {
    char p[336];
    snprintf(p, sizeof(p), "%s/lastplayed", absDir);
    SceUID fd = sceIoOpen(p, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) { sceIoWrite(fd, "1", 1); sceIoClose(fd); }
}

bool worldListCreate(WorldList* list, const char* inName, char* outName, int gamemode, long seed) {
    if (list->count >= MCPSP_MAX_WORLDS)
        return false;

    char displayName[64];
    if (inName && inName[0] != '\0') {
        strncpy(displayName, inName, sizeof(displayName) - 1);
        displayName[sizeof(displayName) - 1] = '\0';
    } else {
        strcpy(displayName, "New world");
    }

    char candidate[64];
    strncpy(candidate, displayName, sizeof(candidate) - 1);
    candidate[sizeof(candidate) - 1] = '\0';
    char full[320];
    for (int suffix = 0; suffix < 60; suffix++) {
        if (suffix > 0)
            strcat(candidate, "-");

        bool taken = false;
        for (int i = 0; i < list->count; i++) {
            if (strcmp(list->names[i], candidate) == 0) {
                taken = true;
                break;
            }
        }
        if (!taken)
            break;
    }

    snprintf(full, sizeof(full), "saves/%s", candidate);
    sceIoMkdir(assetPath(full), 0777);

    char infoPath[320];
    snprintf(infoPath, sizeof(infoPath), "saves/%s/level.txt", candidate);
    SceUID fd = sceIoOpen(assetPath(infoPath), PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%d\n%s\n%ld\n", gamemode, displayName, seed);
        sceIoWrite(fd, buf, strlen(buf));
        sceIoClose(fd);
    }

    strncpy(list->names[list->count], candidate, sizeof(list->names[0]) - 1);
    list->names[list->count][sizeof(list->names[0]) - 1] = '\0';

    strncpy(list->displayNames[list->count], displayName, sizeof(list->displayNames[0]) - 1);
    list->displayNames[list->count][sizeof(list->displayNames[0]) - 1] = '\0';

    snprintf(list->dates[list->count], sizeof(list->dates[0]), "Just now");
    list->gameModes[list->count] = gamemode;
    list->seeds[list->count] = seed;

    strncpy(outName, candidate, 64);
    list->count++;
    return true;
}

bool worldListDelete(WorldList* list, int index) {
    if (index < 0 || index >= list->count)
        return false;

    char dirRel[320];
    snprintf(dirRel, sizeof(dirRel), "saves/%s", list->names[index]);
    char dirFull[320];
    strncpy(dirFull, assetPath(dirRel), sizeof(dirFull) - 1);
    dirFull[sizeof(dirFull) - 1] = '\0';

    SceUID d = sceIoDopen(dirFull);
    if (d >= 0) {
        SceIoDirent entry;
        memset(&entry, 0, sizeof(entry));
        while (sceIoDread(d, &entry) > 0) {
            if (!FIO_S_ISDIR(entry.d_stat.st_mode) &&
                strcmp(entry.d_name, ".") != 0 && strcmp(entry.d_name, "..") != 0) {
                char fileRel[384];
                snprintf(fileRel, sizeof(fileRel), "%s/%s", dirRel, entry.d_name);
                sceIoRemove(assetPath(fileRel));
            }
            memset(&entry, 0, sizeof(entry));
        }
        sceIoDclose(d);
    }
    sceIoRmdir(dirFull);

    for (int i = index; i < list->count - 1; i++) {
        strcpy(list->names[i], list->names[i + 1]);
        strcpy(list->displayNames[i], list->displayNames[i + 1]);
        strcpy(list->dates[i], list->dates[i + 1]);
        list->gameModes[i] = list->gameModes[i + 1];
    }
    list->count--;
    return true;
}
