
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_TILE_ENTITY_H

class Level;
class CompoundTag;

enum TileEntityRendererId {
    TR_NO_RENDER = 0,
    TR_SIGN_RENDERER
};

enum TileEntityType {
    TE_CHEST   = 1,
    TE_FURNACE = 2,
    TE_REACTOR = 3,
    TE_SIGN    = 4
};

class TileEntity {
public:
    explicit TileEntity(int type);
    virtual ~TileEntity() {}

    virtual void setLevelAndPos(Level* level, int x, int y, int z);

    virtual void tick() {}

    virtual bool save(CompoundTag* tag);
    virtual void load(CompoundTag* tag);

    int getTile() const;
    int getData() const;

    int type;
    int x, y, z;
    Level* level;
    int rendererId;
    bool removed;
};

#endif
