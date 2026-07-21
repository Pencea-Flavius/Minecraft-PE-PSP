#include "world/level/tile/tiles.h"
#include "world/level/chunk/chunk.h"

void tileLeaves(unsigned char data, int f, int* col, int* row, unsigned int* tint) {
    if ((data & 3) == LEAF_SPRUCE) {
        *col = 4; *row = 8;
        *tint = 0xFF619961;
    } else if ((data & 3) == LEAF_BIRCH) {
        *col = 4; *row = 3;
        *tint = 0xFF619961;
    } else {
        *col = 4; *row = 3;
        *tint = 0xFF1ABF48;
    }
}
