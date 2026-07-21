#include "world/level/storage/region_file.h"

#include <cstring>
#include <cstdio>
#define LOGI printf

static const int SECTOR_BYTES = 4096;
static const int SECTOR_INTS  = SECTOR_BYTES / 4;
static const int SECTOR_COLS  = 32;
static const char* const REGION_DAT_NAME = "chunks.dat";

static void logAssert(int actual, int expected) {
    if (actual != expected)
        LOGI("RegionFile: I/O op failed (%d vs %d)\n", actual, expected);
}

RegionFile::RegionFile(const std::string& basePath)
    : file(NULL) {
    filename = basePath;
    filename += "/";
    filename += REGION_DAT_NAME;
    offsets = new int[SECTOR_INTS];
    emptyChunk = new int[SECTOR_INTS];
    memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::~RegionFile() {
    close();
    delete[] offsets;
    delete[] emptyChunk;
}

bool RegionFile::open() {
    close();
    memset(offsets, 0, SECTOR_INTS * sizeof(int));

    file = fopen(filename.c_str(), "r+b");
    if (file) {
        logAssert(fread(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        int fileSectors = (fsize > 0) ? (int)(fsize / SECTOR_BYTES) : 0;
        fseek(file, 0, SEEK_SET);

        for (int i = 1; i < fileSectors; i++)
            sectorFree[i] = true;
        sectorFree[0] = false;
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
                    sectorFree[base + i] = false;
            }
        }
    } else {

        file = fopen(filename.c_str(), "w+b");
        if (!file) {
            LOGI("RegionFile: failed to create %s\n", filename.c_str());
            return false;
        }
        logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);
        sectorFree[0] = false;
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
            sectorFree[sectorNum + i] = true;

        int slot = 0, runLength = 0;
        bool extendFile = false;
        while (runLength < sectorsNeeded) {
            if (sectorFree.find(slot + runLength) == sectorFree.end()) {
                extendFile = true;
                break;
            }
            if (sectorFree[slot + runLength] == true) {
                runLength++;
            } else {
                slot = slot + runLength + 1;
                runLength = 0;
            }
        }

        if (extendFile) {
            fseek(file, 0, SEEK_END);
            int extend = sectorsNeeded - runLength;
            for (int i = 0; i < extend; i++) {
                fwrite(emptyChunk, sizeof(int), SECTOR_INTS, file);
                sectorFree[slot + i] = true;
            }
        }
        offsets[idx] = (slot << 8) | sectorsNeeded;
        for (int i = 0; i < sectorsNeeded; i++)
            sectorFree[slot + i] = false;

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
