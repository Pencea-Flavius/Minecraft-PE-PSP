#pragma once

#include "world/level/chunk/chunk.h"
struct Texture;

class ItemModelRenderer {
public:

    bool build(short id, unsigned char data, int bowStage = -1);

    bool isFlat() const { return m_flat; }
    int  count()  const { return m_count; }

    void draw(unsigned int brCol, bool noMip);

    static void drawMesh(ChunkVertex* m, int n, unsigned int brCol,
                         const Texture* tex, bool noMip);

    static void applyFlatPreTransform();

private:

    static const int MESH_MAX = 4700;
    ChunkVertex    m_base[MESH_MAX];
    int            m_count = 0;
    bool           m_flat  = false;
    short          m_id = -1;
    unsigned char  m_data = 0xFF;
    int            m_bowStage = -2;
    const Texture* m_tex = 0;
};
