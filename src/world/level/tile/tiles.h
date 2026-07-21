#ifndef TILES_H__
#define TILES_H__

void tileGlass(unsigned char data, int f, int* col, int* row);
void tileWool(unsigned char data, int f, int* col, int* row);
void tileLeaves(unsigned char data, int f, int* col, int* row, unsigned int* tint);
void tileFurnace(unsigned char data, int f, bool lit, int* col, int* row);
void tileChest(unsigned char data, int f, int* col, int* row);
void tileQuartzBlock(unsigned char data, int f, int* col, int* row);
void tileChiseledSandstone(unsigned char data, int f, int* col, int* row);
void tileSmoothSandstone(unsigned char data, int f, int* col, int* row);
void tileChiseledQuartz(unsigned char data, int f, int* col, int* row);
void tilePillarQuartz(unsigned char data, int f, int* col, int* row);
void tileNetherReactor(unsigned char data, int f, int* col, int* row);
void tileStonecutter(unsigned char data, int f, int* col, int* row);
void tileCraftingTable(unsigned char data, int f, int* col, int* row);

#endif
