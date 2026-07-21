
#include "world/level/tile/tile_behavior.h"

bool supportCanSurvive(World* w, unsigned char id, int x, int y, int z, int data) {
    switch (id) {
        case BLOCK_TOPSNOW:
            return isSolidPhys(worldBlock(w, x, y - 1, z));
        case BLOCK_SIGN:
            return isSolidPhys(worldBlock(w, x, y - 1, z));
        case BLOCK_WALL_SIGN: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;
            if (d == 2) return isSolidPhys(worldBlock(w, x, y, z + 1));
            if (d == 3) return isSolidPhys(worldBlock(w, x, y, z - 1));
            if (d == 4) return isSolidPhys(worldBlock(w, x + 1, y, z));
            if (d == 5) return isSolidPhys(worldBlock(w, x - 1, y, z));
            return false;
        }
        case BLOCK_DOOR_WOOD: case BLOCK_DOOR_IRON: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;
            if (d & 8) return worldBlock(w, x, y - 1, z) == id;
            unsigned char below = worldBlock(w, x, y - 1, z);
            return isSolidPhys(below) && worldBlock(w, x, y + 1, z) == id;
        }
        case BLOCK_TRAPDOOR: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;
            int dir = d & 3;
            if (dir == 0) return isTrapdoorAttachable(worldBlock(w, x, y, z + 1));
            if (dir == 1) return isTrapdoorAttachable(worldBlock(w, x, y, z - 1));
            if (dir == 2) return isTrapdoorAttachable(worldBlock(w, x + 1, y, z));
            if (dir == 3) return isTrapdoorAttachable(worldBlock(w, x - 1, y, z));
            return false;
        }
        case BLOCK_LADDER: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;
            if (d == 2) return isOpaque(worldBlock(w, x, y, z + 1));
            if (d == 3) return isOpaque(worldBlock(w, x, y, z - 1));
            if (d == 4) return isOpaque(worldBlock(w, x + 1, y, z));
            if (d == 5) return isOpaque(worldBlock(w, x - 1, y, z));
            return false;
        }
        case BLOCK_TORCH: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;

            if (d == 5) { unsigned char below = worldBlock(w, x, y - 1, z);
                          return isOpaque(below) || isFence(below) || below == BLOCK_GLASS; }
            if (d == 1) return isOpaque(worldBlock(w, x - 1, y, z));
            if (d == 2) return isOpaque(worldBlock(w, x + 1, y, z));
            if (d == 3) return isOpaque(worldBlock(w, x, y, z - 1));
            if (d == 4) return isOpaque(worldBlock(w, x, y, z + 1));
            return false;
        }
        case BLOCK_BED: {
            unsigned char d = (data == -1) ? worldData(w, x, y, z) : data;
            bool isHead = (d & 8) != 0;
            int dir = d & 3;
            int otherX = x, otherZ = z;
            if (isHead) {
                if (dir == 0) otherZ -= 1;
                if (dir == 1) otherX += 1;
                if (dir == 2) otherZ += 1;
                if (dir == 3) otherX -= 1;
            } else {
                if (dir == 0) otherZ += 1;
                if (dir == 1) otherX -= 1;
                if (dir == 2) otherZ -= 1;
                if (dir == 3) otherX += 1;
            }
            return worldBlock(w, otherX, y, otherZ) == BLOCK_BED;
        }
        default:
            return true;
    }
}
