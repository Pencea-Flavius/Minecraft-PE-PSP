#include "world/level/tile/tiles.h"
#include "world/level/chunk/chunk.h"

void tileStonecutter(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP)       { *col = 9; *row = 10; }
    else if (f == F_DOWN) { *col = 14; *row = 3; }
    else if (f == F_LEFT || f == F_RIGHT) { *col = 8; *row = 10; }
    else                  { *col = 13; *row = 2; }
}

void tileCraftingTable(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP)       { *col = 11; *row = 2; }
    else if (f == F_DOWN) { *col = 4; *row = 0; }
    else if (f == F_LEFT || f == F_BACK) { *col = 12; *row = 3; }
    else                  { *col = 11; *row = 3; }
}
