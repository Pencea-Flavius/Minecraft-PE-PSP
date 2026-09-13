#include <pspiofilemgr.h>
#include "world/level/storage/region_file.h"

#include <cstring>
#include <cstdio>

#define STORAGE_LOG 0
#if STORAGE_LOG
#define LOGI printf
#else
#define LOGI(...) ((void)0)
#endif

#include <cstdlib>

static const int SECTOR_BYTES = 4096;
static const int SECTOR_INTS  = SECTOR_BYTES / 4;
static const int SECTOR_COLS  = 32;

static void logAssert(int actual, int expected) {
    if (actual != expected)
        LOGI("RegionFile: I/O op failed (%d vs %d)\n", actual, expected);
}

RegionFile::RegionFile(const char* path)
    : file(NULL), offsets(0), emptyChunk(0), freeBits(0), knownSectors(0) {
    snprintf(filename, sizeof(filename), "%s", path);

    offsets    = (int*)malloc(SECTOR_INTS * sizeof(int));
    emptyChunk = (int*)malloc(SECTOR_INTS * sizeof(int));
    freeBits   = (unsigned char*)malloc((RF_MAX_SECTORS + 7) / 8);
    if (emptyChunk) memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::~RegionFile() {
    close();
    free(offsets);
    free(emptyChunk);
    free(freeBits);
}

bool RegionFile::open() {
    close();
    if (!offsets || !emptyChunk || !freeBits) return false;
    memset(offsets, 0, SECTOR_INTS * sizeof(int));
    memset(freeBits, 0, (RF_MAX_SECTORS + 7) / 8);
    knownSectors = 0;

    file = fopen(filename, "r+b");
    if (file) {
        logAssert(fread(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        int fileSectors = (fsize > 0) ? (int)(fsize / SECTOR_BYTES) : 0;
        fseek(file, 0, SEEK_SET);

        if (fileSectors > RF_MAX_SECTORS) fileSectors = RF_MAX_SECTORS;
        for (int i = 1; i < fileSectors; i++)
            sectorSetFree(i, true);
        sectorSetFree(0, false);
        for (int sector = 0; sector < SECTOR_INTS; sector++) {
            int offset = offsets[sector];
            if (offset) {
                int base = offset >> 8;
                int count = offset & 0xff;
                if (count == 0 || base < 1 || base + count > fileSectors) {
                    offsets[sector] = 0;
                    continue;
                }
                for (int i = 0; i < count; i++)
                    sectorSetFree(base + i, false);
            }
        }
    } else {

        SceIoStat st;
        if (sceIoGetstat(filename, &st) >= 0) return false;

        file = fopen(filename, "w+b");
        if (!file) {
            LOGI("RegionFile: failed to create %s\n", filename);
            return false;
        }
        logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);
        sectorSetFree(0, false);
    }
    return file != NULL;
}

void RegionFile::close() {
    if (file) { fclose(file); file = NULL; }
}

bool RegionFile::readChunk(int x, int z, unsigned char** dest, int* destLen) {
    *dest = NULL; *destLen = 0;
    if (!file) return false;
    int idx = x + z * SECTOR_COLS;
    if (idx < 0 || idx >= SECTOR_INTS) return false;

    int offset = offsets[idx];
    if (offset == 0) return false;

    int sectorNum = offset >> 8;
    fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
    int length = 0;
    if (fread(&length, sizeof(int), 1, file) != 1) return false;

    if (length <= (int)sizeof(int) || length > (offset & 0xff) * SECTOR_BYTES)
        return false;
    length -= sizeof(int);

    unsigned char* data = new unsigned char[length];
    if ((int)fread(data, 1, length, file) != length) { delete[] data; return false; }
    *dest = data;
    *destLen = length;
    return true;
}

bool RegionFile::writeChunk(int x, int z, const unsigned char* data, int len) {
    if (!file) return false;
    int idx = x + z * SECTOR_COLS;
    if (idx < 0 || idx >= SECTOR_INTS) return false;

    int size = len + sizeof(int);
    int offset = offsets[idx];
    int sectorNum = offset >> 8;
    int sectorCount = offset & 0xff;
    int sectorsNeeded = (size / SECTOR_BYTES) + 1;

    if (sectorsNeeded > 256) {
        LOGI("RegionFile: chunk too big to save\n");
        return false;
    }

    if (sectorNum != 0 && sectorCount == sectorsNeeded) {

        write(sectorNum, data, len);
    } else {

        for (int i = 0; i < sectorCount; i++)
            sectorSetFree(sectorNum + i, true);

        int slot = 0, runLength = 0;
        bool extendFile = false;
        while (runLength < sectorsNeeded) {
            if (!sectorKnown(slot + runLength)) {
                extendFile = true;
                break;
            }
            if (sectorIsFree(slot + runLength)) {
                runLength++;
            } else {
                slot = slot + runLength + 1;
                runLength = 0;
            }
        }
        if (slot + sectorsNeeded > RF_MAX_SECTORS) {
            LOGI("RegionFile: region full\n");
            return false;
        }

        if (extendFile) {
            fseek(file, 0, SEEK_END);
            int extend = sectorsNeeded - runLength;
            for (int i = 0; i < extend; i++) {
                fwrite(emptyChunk, sizeof(int), SECTOR_INTS, file);
                sectorSetFree(slot + i, true);
            }
        }
        offsets[idx] = (slot << 8) | sectorsNeeded;
        for (int i = 0; i < sectorsNeeded; i++)
            sectorSetFree(slot + i, false);

        write(slot, data, len);

        fseek(file, idx * sizeof(int), SEEK_SET);
        fwrite(&offsets[idx], sizeof(int), 1, file);
    }
    fflush(file);
    return true;
}

bool RegionFile::write(int sector, const unsigned char* data, int len) {
    fseek(file, sector * SECTOR_BYTES, SEEK_SET);
    int size = len + sizeof(int);
    logAssert(fwrite(&size, sizeof(int), 1, file), 1);
    logAssert(fwrite(data, 1, len, file), len);
    return true;
}
