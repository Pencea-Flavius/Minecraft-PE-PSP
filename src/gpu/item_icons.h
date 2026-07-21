
#ifndef MCPSP_GPU_ITEM_ICONS_H
#define MCPSP_GPU_ITEM_ICONS_H

#define II_BOOTS_CHAIN        112
#define II_BOW_PULL_0         115
#define II_BOW_PULL_1         116
#define II_BOW_PULL_2         117
#define II_CAMERA             114
#define II_CHESTPLATE_CHAIN   110
#define II_EGG                113
#define II_HELMET_CHAIN       109
#define II_LEGGINGS_CHAIN     111
#define II_SLOT_BOOTS         130
#define II_SLOT_CHESTPLATE    128
#define II_SLOT_HELMET        127
#define II_SLOT_LEGGINGS      129
#define II_SPAWN_CHICKEN      120
#define II_SPAWN_COW          119
#define II_SPAWN_CREEPER      123
#define II_SPAWN_PIG          118
#define II_SPAWN_PIGZOMBIE    126
#define II_SPAWN_SHEEP        121
#define II_SPAWN_SKELETON     124
#define II_SPAWN_SPIDER       125
#define II_SPAWN_ZOMBIE       122

static const short kItemIcon[256] = {
      46,   45,   47,  103,  106,   19,   62,   -1,
      25,   22,   21,   18,   57,   38,   36,   39,
      58,   42,   41,   43,   59,   49,   48,   50,
      37,   67,   66,   60,   53,   52,   54,   61,
      63,  100,   40,   44,   17,   51,   55,   14,
     105,  104,   70,   72,   73,   74,  109,  110,
     111,  112,   75,   76,   77,   78,   79,   80,
      81,   82,   83,   84,   85,   86,   64,   28,
      29,    5,   -1,   20,    3,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   98,   -1,   71,   -1,
      31,   30,   10,   96,   97,   -1,   -1,   -1,
     113,   -1,   -1,   -1,   99,   -1,   -1,   -1,
      95,  101,   -1,    4,   -1,   -1,   -1,   56,
      68,   -1,   15,   32,   33,   34,   35,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   27,   69,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
     114,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};

static const short kItemIconCoal[16] = { 102, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static const short kItemIconDye[16] = { -1, 24, 26, -1, 65, 87, 88, -1, -1, 89, 90, 91, 92, 93, 94, 16 };

static inline short itemIconSpawnEgg(short data) {
    switch (data) {
        case 10: return II_SPAWN_CHICKEN;
        case 11: return II_SPAWN_COW;
        case 12: return II_SPAWN_PIG;
        case 13: return II_SPAWN_SHEEP;
        case 32: return II_SPAWN_ZOMBIE;
        case 33: return II_SPAWN_CREEPER;
        case 34: return II_SPAWN_SKELETON;
        case 35: return II_SPAWN_SPIDER;
        case 36: return II_SPAWN_PIGZOMBIE;
        default: return -1;
    }
}

#endif
