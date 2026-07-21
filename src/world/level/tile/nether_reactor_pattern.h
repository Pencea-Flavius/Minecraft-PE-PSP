
#ifndef MCPSP_WORLD_LEVEL_TILE_NETHER_REACTOR_PATTERN_H
#define MCPSP_WORLD_LEVEL_TILE_NETHER_REACTOR_PATTERN_H

class NetherReactorPattern {
public:
    NetherReactorPattern();
    void setTileAt(int level, int x, int z, int tile);
    int  getTileAt(int level, int x, int z) const;
private:
    int pattern[3][3][3];
};

#endif
