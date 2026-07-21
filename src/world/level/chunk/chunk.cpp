
#include "world/level/chunk/chunk.h"
#include "world/level/tile/tiles.h"

#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <malloc.h>

unsigned int g_brightColor[16];

float g_brightRamp[16];
void chunkInitBrightRamp(void) {
    const float amb = 0.05f;
    for (int i = 0; i <= 15; i++) {
        float v = 1.0f - i / 16.0f;
        float b = ((1.0f - v) / (v * 3.0f + 1.0f)) * (1.0f - amb) + amb * 3.0f;
        if (b > 1.0f) b = 1.0f;
        if (b < 0.0f) b = 0.0f;
        unsigned int c = (unsigned int)(b * 255.0f + 0.5f);
        g_brightColor[i] = 0xFF000000u | (c << 16) | (c << 8) | c;
        g_brightRamp[i] = b;
    }
}

const signed char kFaceNeighbor[6][3] = {
    {-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1},
};

const unsigned int kFaceShade[6] = {
    0xFF999999u,
    0xFF999999u,
    0xFF7F7F7Fu,
    0xFFFFFFFFu,
    0xFFCCCCCCu,
    0xFFCCCCCCu,
};

void tileForBlock(unsigned char id, unsigned char data, int f, int* col, int* row, unsigned int* tint) {
    Tile::tiles[id]->getTexture(data, f, col, row, tint);
}
