#include "world/level/tile/tiles.h"
#include "world/level/chunk/chunk.h"

void tileQuartzBlock(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP)       { *col = 4; *row = 12; }
    else if (f == F_DOWN) { *col = 3; *row = 13; }
    else                  { *col = 4; *row = 13; }
}

void tileChiseledSandstone(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 0; *row = 11; }
    else                           { *col = 5; *row = 14; }
}

void tileSmoothSandstone(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 0; *row = 11; }
    else                           { *col = 6; *row = 14; }
}

void tileChiseledQuartz(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 6; *row = 12; }
    else                           { *col = 6; *row = 13; }
}

void tilePillarQuartz(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 5; *row = 12; }
    else                           { *col = 5; *row = 13; }
}
