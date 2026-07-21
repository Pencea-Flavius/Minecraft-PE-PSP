#include "world/level/tile/tiles.h"
#include "world/level/chunk/chunk.h"

void tileNetherReactor(unsigned char data, int f, int* col, int* row) {
    switch (data) {
        case 1:  *col = 9;  *row = 14; break;
        case 2:  *col = 8;  *row = 14; break;
        default: *col = 10; *row = 14; break;
    }
}
