#include "world/level/storage/chunk_storage.h"
#include "world/level/storage/region_file.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <pspkernel.h>
#include <pspthreadman.h>

#define STORAGE_LOG 0
#if STORAGE_LOG
#define LOGI printf
#else
#define LOGI(...) ((void)0)
#endif

static const int CH_BLOCKS = 16 * 16 * 128;
static const int CH_NIBBLE = CH_BLOCKS / 2;
static const int CH_COLS   = 256;
static const int CH_PAYLOAD = CH_BLOCKS + CH_NIBBLE * 3 + CH_COLS;
static const int OFF_DATA = CH_BLOCKS;
static const int OFF_SKY  = OFF_DATA + CH_NIBBLE;
static const int OFF_BLK  = OFF_SKY  + CH_NIBBLE;
static const int OFF_UPD  = OFF_BLK  + CH_NIBBLE;

static const unsigned char CH_UNPOPULATED = 0x5A;

static const int OFF_CRC = OFF_UPD + 1;

static unsigned int crc32(const unsigned char* p, int n) {
    unsigned int c = 0xFFFFFFFFu;
    for (int i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (unsigned int)(-(int)(c & 1)));
    }
    return ~c;
}

static unsigned int payloadCrc(unsigned char* buf) {
    unsigned char save[4];
    memcpy(save, buf + OFF_CRC, 4);
    memset(buf + OFF_CRC, 0, 4);
    unsigned int c = crc32(buf, CH_PAYLOAD);
    memcpy(buf + OFF_CRC, save, 4);
    return c;
}
static void crcPut(unsigned char* buf, unsigned int c) {
    for (int i = 0; i < 4; i++) buf[OFF_CRC + i] = (unsigned char)(c >> (i * 8));
}
static unsigned int crcGet(const unsigned char* buf) {
    unsigned int c = 0;
    for (int i = 0; i < 4; i++) c |= (unsigned int)buf[OFF_CRC + i] << (i * 8);
    return c;
}

static inline int chunkIdx(int lx, int lz, int y) { return (lx << 11) | (lz << 7) | y; }
static inline void nibSet(unsigned char* base, int idx, int v) {
    unsigned char& b = base[idx >> 1];
    if (idx & 1) b = (b & 0x0F) | ((v & 0x0F) << 4);
    else         b = (b & 0xF0) | (v & 0x0F);
}
static inline int nibGet(const unsigned char* base, int idx) {
    unsigned char b = base[idx >> 1];
    return (idx & 1) ? (b >> 4) & 0x0F : b & 0x0F;
}

unsigned int g_chunkCrcFails = 0;

#define REGION_CACHE 4

struct OpenRegion {
    RegionFile* rf;
    int rx, rz;
    bool valid;
};
static OpenRegion s_cache[REGION_CACHE];
static int  s_next = 0;
static char s_dir[256];
static bool s_haveDir = false;

static unsigned char* s_payload = 0;

static inline int regionOf(int c) { return c >> 5; }

static void regionPath(char* out, size_t n, int rx, int rz) {

    if (rx == 0 && rz == 0) snprintf(out, n, "%s/chunks.dat", s_dir);
    else                    snprintf(out, n, "%s/r.%d.%d.dat", s_dir, rx, rz);
}

static RegionFile* regionFor(int cx, int cz, bool create) {
    if (!s_haveDir) return 0;
    int rx = regionOf(cx), rz = regionOf(cz);
    for (int i = 0; i < REGION_CACHE; i++)
        if (s_cache[i].valid && s_cache[i].rx == rx && s_cache[i].rz == rz)
            return s_cache[i].rf;

    char path[320];
    regionPath(path, sizeof(path), rx, rz);
    if (!create) {
        FILE* f = fopen(path, "rb");
        if (!f) return 0;
        fclose(f);
    }

    OpenRegion* slot = &s_cache[s_next];
    s_next = (s_next + 1) % REGION_CACHE;
    if (slot->valid) { delete slot->rf; slot->rf = 0; slot->valid = false; }

    RegionFile* rf = new (std::nothrow) RegionFile(path);
    if (!rf) return 0;
    if (!rf->open()) { delete rf; return 0; }
    slot->rf = rf; slot->rx = rx; slot->rz = rz; slot->valid = true;
    return rf;
}

static SceUID s_lock = -1;
static void storageLock() {
    if (s_lock < 0) s_lock = sceKernelCreateSema("mcChunkStore", 0, 1, 1, NULL);
    if (s_lock >= 0) sceKernelWaitSema(s_lock, 1, NULL);
}
static void storageUnlock() {
    if (s_lock >= 0) sceKernelSignalSema(s_lock, 1);
}

namespace { struct StorageGuard {
    StorageGuard()  { storageLock(); }
    ~StorageGuard() { storageUnlock(); }
}; }

void chunkStorageInit(const char* absDir) {
    chunkStorageShutdown();
    snprintf(s_dir, sizeof(s_dir), "%s", absDir);
    s_haveDir = true;
}

void chunkStorageShutdown() {
    StorageGuard guard;
    for (int i = 0; i < REGION_CACHE; i++) {
        if (s_cache[i].valid) delete s_cache[i].rf;
        s_cache[i].rf = 0; s_cache[i].valid = false;
    }
    s_next = 0;
    s_haveDir = false;
    free(s_payload); s_payload = 0;
}

void chunkStorageDropOpenFiles() {
    StorageGuard guard;
    for (int i = 0; i < REGION_CACHE; i++) {
        if (s_cache[i].valid) delete s_cache[i].rf;
        s_cache[i].rf = 0; s_cache[i].valid = false;
    }
    s_next = 0;

}

bool chunkStorageHasSave(const char* absDir) {

    char path[320];
    snprintf(path, sizeof(path), "%s/chunks.dat", absDir);
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

static unsigned char* payload() {
    if (!s_payload) s_payload = (unsigned char*)malloc(CH_PAYLOAD);
    return s_payload;
}

bool chunkStorageLoad(World* w, int cx, int cz, bool* outGotLight, bool* outPopulated) {
    if (outGotLight) *outGotLight = true;
    if (outPopulated) *outPopulated = true;

    StorageGuard guard;
    RegionFile* rf = regionFor(cx, cz, false);
    if (!rf) return false;

    unsigned char* buf = NULL;
    int len = 0;
    if (!rf->readChunk(cx & 31, cz & 31, &buf, &len)) return false;
    if (len < OFF_DATA + CH_NIBBLE) { delete[] buf; return false; }

    if (len >= OFF_CRC + 4) {
        unsigned int stored = crcGet(buf);
        if (stored && stored != payloadCrc(buf)) {
            LOGI("chunkStorage: chunk %d,%d fails its checksum -- regenerating\n", cx, cz);
            g_chunkCrcFails++;
            delete[] buf;
            return false;
        }
    }

    if (len < OFF_UPD && outGotLight) *outGotLight = false;
    if (outPopulated && len > OFF_UPD && buf[OFF_UPD] == CH_UNPOPULATED) *outPopulated = false;

    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            int gx = cx * 16 + lx, gz = cz * 16 + lz;
            int dstBase = chunkIdx(lx, lz, 0);

            blockColumnPut(w, gx, gz, buf + dstBase);

            worldDataColumnPut(w, gx, gz, buf + OFF_DATA + (dstBase >> 1));
            for (int y = 0; y < 128; y++) {

                if (buf[dstBase + y] == BLOCK_ORE_REDSTONE_LIT)
                    worldScheduleTick(w, gx, y, gz, BLOCK_ORE_REDSTONE_LIT, 30);
            }
        }
    }
    delete[] buf;
    return true;
}

bool chunkStorageSave(World* w, int cx, int cz) {
    StorageGuard guard;
    RegionFile* rf = regionFor(cx, cz, true);
    if (!rf) return false;
    unsigned char* buf = payload();
    if (!buf) { LOGI("chunkStorage: no room for the save buffer\n"); return false; }

    memset(buf, 0, CH_PAYLOAD);
    if (!worldSlot(w, cx, cz)->terrainPopulated) buf[OFF_UPD] = CH_UNPOPULATED;
    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            int gx = cx * 16 + lx, gz = cz * 16 + lz;
            int dstBase = chunkIdx(lx, lz, 0);

            blockColumnGet(w, gx, gz, buf + dstBase);

            worldDataColumnGet(w, gx, gz, buf + OFF_DATA + (dstBase >> 1));
        }
    }

    for (int y = 0; y < 128; y++) {
        bool skyDark = lightPlaneAllDark(w, 0, cx * 16, y, cz * 16);
        bool blkDark = lightPlaneAllDark(w, 1, cx * 16, y, cz * 16);
        if (skyDark && blkDark) continue;
        for (int lx = 0; lx < 16; lx++) {
            for (int lz = 0; lz < 16; lz++) {
                int gx = cx * 16 + lx, gz = cz * 16 + lz;
                int idx = chunkIdx(lx, lz, y);
                if (!skyDark) nibSet(buf + OFF_SKY, idx, lightSkyGet(w, gx, y, gz));
                if (!blkDark) nibSet(buf + OFF_BLK, idx, lightBlockGet(w, gx, y, gz));
            }
        }
    }
    crcPut(buf, payloadCrc(buf));
    if (!rf->writeChunk(cx & 31, cz & 31, buf, CH_PAYLOAD)) return false;
    worldSlot(w, cx, cz)->unsaved = false;
    return true;
}
