
#ifndef MCPSP_PLATFORM_PNG_LOADER_H
#define MCPSP_PLATFORM_PNG_LOADER_H

struct PngReader;

extern char g_pngLastError[64];

PngReader* pngOpen(const char* path, int* outW, int* outH);

PngReader* pngOpenAt(const char* path, unsigned int offset, int* outW, int* outH);

PngReader* pngOpenMem(const unsigned char* data, unsigned int size, int* outW, int* outH);

bool pngReadRow(PngReader* r, unsigned char* rgbaRow);
void pngClose(PngReader* r);

#endif
