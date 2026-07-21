#include "world/level/tile/tiles.h"

void tileWool(unsigned char data, int f, int* col, int* row) {
    int d = data & 0xF;
    int tex;
    if (d == 0) tex = 64;
    else { int di = (~d) & 0xF; tex = 7*16+1 + ((di & 8) >> 3) + ((di & 7) * 16); }
    *col = tex % 16; *row = tex / 16;
}
