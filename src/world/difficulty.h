
#ifndef MCPSP_WORLD_DIFFICULTY_H
#define MCPSP_WORLD_DIFFICULTY_H

class Difficulty {
public:
    static const int PEACEFUL = 0;
    static const int EASY     = 1;
    static const int NORMAL   = 2;
    static const int HARD     = 3;
};

extern int g_difficulty;

#endif
