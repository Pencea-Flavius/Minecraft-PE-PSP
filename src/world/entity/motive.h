
#ifndef MCPSP_WORLD_ENTITY_MOTIVE_H
#define MCPSP_WORLD_ENTITY_MOTIVE_H

struct Motive {
    const char* name;
    int w, h;
    int uo, vo;
    bool isPublic;
};

enum {
    MOTIVE_COUNT = 28,

    MOTIVE_DEFAULT = 0
};

extern const Motive kMotives[MOTIVE_COUNT];

int motiveByName(const char* name);

#endif
