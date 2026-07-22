
#ifndef MCPSP_WORLD_LEVEL_TILE_TILE_H
#define MCPSP_WORLD_LEVEL_TILE_TILE_H

#include "client/player/physics.h"

class Random;
class Player;
class Entity;

struct Drop { short id; int count; short aux; };

enum TileShape {
    SHAPE_AIR = 0, SHAPE_CUBE, SHAPE_CROSS, SHAPE_LIQUID,
    SHAPE_CACTUS, SHAPE_TOPSNOW, SHAPE_REEDS, SHAPE_WHEAT, SHAPE_MELON_STEM,
    SHAPE_SLAB, SHAPE_STAIRS, SHAPE_PANE, SHAPE_FENCE, SHAPE_FENCEGATE,
    SHAPE_DOOR, SHAPE_TRAPDOOR, SHAPE_LADDER, SHAPE_TORCH, SHAPE_BED,
    SHAPE_SIGN, SHAPE_CHEST
};

enum TileSound {
    SOUND_SILENT = 0, SOUND_STONE, SOUND_WOOD, SOUND_GRAVEL, SOUND_GRASS,
    SOUND_METAL, SOUND_GLASS, SOUND_CLOTH, SOUND_SAND,
    SOUND_TYPE_COUNT
};

struct SoundType {
    float       volume;
    float       pitch;
    const char* breakSound;
    const char* stepSound;
};

extern const SoundType g_tileSounds[SOUND_TYPE_COUNT];

class Tile {
public:
    unsigned char id;
    int  shape;

    bool solidPhys;
    bool cube;
    bool opaque;
    bool replaceable;
    bool randomTicks;
    unsigned char lightBlock;
    unsigned char lightEmission;
    unsigned char soundType;

    explicit Tile(unsigned char id_)
        : id(id_), shape(SHAPE_CUBE), solidPhys(true), cube(true),
          opaque(true), replaceable(false), randomTicks(false),
          lightBlock(15), lightEmission(0), soundType(SOUND_STONE) {}
    virtual ~Tile() {}

    virtual int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]);

    virtual void getTexture(unsigned char data, int face, int* col, int* row, unsigned int* tint);

    virtual bool mayPlace(World* w, int x, int y, int z, int face);
    virtual bool mayPlace(World* w, int x, int y, int z);
    virtual bool canSurvive(World* w, int x, int y, int z);

    virtual void neighborChanged(World* w, int x, int y, int z);

    virtual void entityInside(World* w, int x, int y, int z, Entity* e);
    virtual void randomTick(World* w, int x, int y, int z);

    virtual bool use(World* w, int x, int y, int z, Player* player) { return false; }

    virtual int getPlacedOnFaceDataValue(World* w, int x, int y, int z, int face,
                                         float clickX, float clickY, float clickZ, int itemValue) {
        return itemValue;
    }

    virtual void setPlacedBy(World* w, int x, int y, int z, Player* placer) {}

    virtual Drop getResource(int data);
    virtual int  getResourceCount(int data, Random& rng);
    virtual void spawnResources(World* w, int x, int y, int z, int data, Random& rng);

    static Tile* tiles[256];
    static void  initTiles();
};

#endif
