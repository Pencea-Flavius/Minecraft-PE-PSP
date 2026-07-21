
#ifndef MCPSP_WORLD_DIRECTION_H
#define MCPSP_WORLD_DIRECTION_H

namespace Facing {
    enum { DOWN = 0, UP = 1, NORTH = 2, SOUTH = 3, WEST = 4, EAST = 5 };
}

namespace Direction {
    enum { UNDEFINED = -1, SOUTH = 0, WEST = 1, NORTH = 2, EAST = 3 };

    static const int FACING_DIRECTION[6] = {
        UNDEFINED, UNDEFINED, NORTH, SOUTH, WEST, EAST
    };

    static const int DIRECTION_OPPOSITE[4] = { NORTH, EAST, SOUTH, WEST };
}

#endif
