
#ifndef MCPSP_WORLD_STORAGE_REGION_FILE_H
#define MCPSP_WORLD_STORAGE_REGION_FILE_H

#include <cstdio>
#include <map>
#include <string>

typedef std::map<int, bool> FreeSectorMap;

class RegionFile {
public:
    RegionFile(const std::string& basePath);
    ~RegionFile();

    bool open();

    bool readChunk(int x, int z, unsigned char** dest, int* destLen);
    bool writeChunk(int x, int z, const unsigned char* data, int len);

private:
    bool write(int sector, const unsigned char* data, int len);
    void close();

    FILE* file;
    std::string filename;
    int* offsets;
    int* emptyChunk;
    FreeSectorMap sectorFree;
};

#endif
