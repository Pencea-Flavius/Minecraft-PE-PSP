#include "world/entity/motive.h"
#include <cstring>

const Motive kMotives[MOTIVE_COUNT] = {
    { "Kebab",        16, 16,  0*16,  0*16, true },
    { "Aztec2",       16, 16,  3*16,  0*16, true },
    { "Alban",        16, 16,  2*16,  0*16, true },
    { "Bomb",         16, 16,  4*16,  0*16, true },
    { "Plant",        16, 16,  5*16,  0*16, true },
    { "Wasteland",    16, 16,  6*16,  0*16, true },
    { "Pool",         32, 16,  0*16,  2*16, true },
    { "Courbet",      32, 16,  2*16,  2*16, true },
    { "Sea",          32, 16,  4*16,  2*16, true },
    { "Sunset",       32, 16,  6*16,  2*16, true },
    { "Creebet",      32, 16,  8*16,  2*16, true },
    { "Wanderer",     16, 32,  0*16,  4*16, true },
    { "Graham",       16, 32,  1*16,  4*16, true },
    { "Match",        32, 32,  0*16,  8*16, true },
    { "Bust",         32, 32,  2*16,  8*16, true },
    { "Stage",        32, 32,  4*16,  8*16, true },
    { "Void",         32, 32,  6*16,  8*16, true },
    { "SkullAndRoses",32, 32,  8*16,  8*16, true },
    { "Fighters",     64, 32,  0*16,  6*16, true },
    { "Pointer",      64, 64,  0*16, 12*16, true },
    { "Pigscene",     64, 64,  4*16, 12*16, true },
    { "BurningSkull", 64, 64,  8*16, 12*16, true },
    { "Skeleton",     64, 48, 12*16,  4*16, true },
    { "DonkeyKong",   64, 48, 12*16,  7*16, true },
    { "Earth",        32, 32,  0*16, 10*16, false },
    { "Wind",         32, 32,  2*16, 10*16, false },
    { "Fire",         32, 32,  4*16, 10*16, false },
    { "Water",        32, 32,  6*16, 10*16, false },
};

int motiveByName(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < MOTIVE_COUNT; i++)
        if (strcmp(kMotives[i].name, name) == 0) return i;
    return -1;
}
