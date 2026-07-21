#include "world/level/tile/tiles.h"
#include "world/level/chunk/chunk.h"

void tileFurnace(unsigned char data, int f, bool lit, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 14; *row = 3; }
    else if (f == data)            { *col = lit ? 13 : 12; *row = lit ? 3 : 2; }
    else                           { *col = 13; *row = 2; }
}

void tileChest(unsigned char data, int f, int* col, int* row) {
    if (f == F_TOP || f == F_DOWN) { *col = 9; *row = 1; }
    else if (f == data)            { *col = 11; *row = 1; }
    else                           { *col = 10; *row = 1; }
}
