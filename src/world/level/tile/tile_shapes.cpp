
#include "world/level/tile/tile_shapes.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/fire.h"

#define SET(dst, X0, Y0, Z0, X1, Y1, Z1) \
    do { (dst)[0] = (X0); (dst)[1] = (Y0); (dst)[2] = (Z0); \
         (dst)[3] = (X1); (dst)[4] = (Y1); (dst)[5] = (Z1); } while (0)

void trapdoorShape(unsigned char data, float out[6]) {
    const float r = 3.0f / 16.0f;
    float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 1.0f, y1 = r, z1 = 1.0f;
    if (data & 4) {
        y1 = 1.0f;
        switch (data & 3) {
            case 0: z0 = 1.0f - r;            break;
            case 1:            z1 = r;        break;
            case 2: x0 = 1.0f - r;            break;
            default:           x1 = r;        break;
        }
    }
    out[0] = x0; out[1] = y0; out[2] = z0; out[3] = x1; out[4] = y1; out[5] = z1;
}

static bool stairLockAttached(const World* w, int x, int y, int z, unsigned char myData) {
    unsigned char nb = worldBlock(w, x, y, z);
    if (isStairs(nb)) {
        unsigned char nbData = worldData(w, x, y, z);
        return (nbData & STAIR_UPSIDEDOWN_BIT) == (myData & STAIR_UPSIDEDOWN_BIT);
    }
    return false;
}

int stairShapeBoxes(const World* w, int gx, int y, int gz, unsigned char data, float out[3][6]) {
    int dir = data & STAIR_DIR_MASK;
    bool upsideDown = (data & STAIR_UPSIDEDOWN_BIT) != 0;
    int n = 0;

    float by0 = upsideDown ? 0.5f : 0.0f;
    float by1 = upsideDown ? 1.0f : 0.5f;
    out[n][0] = 0.0f; out[n][1] = by0; out[n][2] = 0.0f;
    out[n][3] = 1.0f; out[n][4] = by1; out[n][5] = 1.0f;
    n++;

    float sx0 = 0.0f, sx1 = 1.0f;
    float sy0 = upsideDown ? 0.0f : 0.5f;
    float sy1 = upsideDown ? 0.5f : 1.0f;
    float sz0 = 0.0f, sz1 = 1.0f;
    bool checkInnerPiece = true;

    if (dir == STAIR_DIR_EAST) {
        sx0 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx + 1, y, gz);
        if (isStairs(backTile) && stairLockAttached(w, gx + 1, y, gz, data)) {
            int backDir = worldData(w, gx + 1, y, gz) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                sz1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                sz0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_WEST) {
        sx1 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx - 1, y, gz);
        if (isStairs(backTile) && stairLockAttached(w, gx - 1, y, gz, data)) {
            int backDir = worldData(w, gx - 1, y, gz) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                sz1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                sz0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_SOUTH) {
        sz0 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx, y, gz + 1);
        if (isStairs(backTile) && stairLockAttached(w, gx, y, gz + 1, data)) {
            int backDir = worldData(w, gx, y, gz + 1) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_WEST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                sx1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_EAST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                sx0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_NORTH) {
        sz1 = 0.5f;
        unsigned char backTile = worldBlock(w, gx, y, gz - 1);
        if (isStairs(backTile) && stairLockAttached(w, gx, y, gz - 1, data)) {
            int backDir = worldData(w, gx, y, gz - 1) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_WEST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                sx1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_EAST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                sx0 = 0.5f; checkInnerPiece = false;
            }
        }
    }
    out[n][0] = sx0; out[n][1] = sy0; out[n][2] = sz0;
    out[n][3] = sx1; out[n][4] = sy1; out[n][5] = sz1;
    n++;

    if (checkInnerPiece) {
        float ix0 = 0.0f, ix1 = 0.5f;
        float iy0 = upsideDown ? 0.0f : 0.5f;
        float iy1 = upsideDown ? 0.5f : 1.0f;
        float iz0 = 0.5f, iz1 = 1.0f;
        bool hasInner = false;

        if (dir == STAIR_DIR_EAST) {
            unsigned char frontTile = worldBlock(w, gx - 1, y, gz);
            if (isStairs(frontTile) && stairLockAttached(w, gx - 1, y, gz, data)) {
                int frontDir = worldData(w, gx - 1, y, gz) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                    iz0 = 0.0f; iz1 = 0.5f; hasInner = true;
                } else if (frontDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                    iz0 = 0.5f; iz1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_WEST) {
            unsigned char frontTile = worldBlock(w, gx + 1, y, gz);
            if (isStairs(frontTile) && stairLockAttached(w, gx + 1, y, gz, data)) {
                ix0 = 0.5f; ix1 = 1.0f;
                int frontDir = worldData(w, gx + 1, y, gz) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                    iz0 = 0.0f; iz1 = 0.5f; hasInner = true;
                } else if (frontDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                    iz0 = 0.5f; iz1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_SOUTH) {
            unsigned char frontTile = worldBlock(w, gx, y, gz - 1);
            if (isStairs(frontTile) && stairLockAttached(w, gx, y, gz - 1, data)) {
                iz0 = 0.0f; iz1 = 0.5f;
                int frontDir = worldData(w, gx, y, gz - 1) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_WEST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                    hasInner = true;
                } else if (frontDir == STAIR_DIR_EAST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                    ix0 = 0.5f; ix1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_NORTH) {
            unsigned char frontTile = worldBlock(w, gx, y, gz + 1);
            if (isStairs(frontTile) && stairLockAttached(w, gx, y, gz + 1, data)) {
                int frontDir = worldData(w, gx, y, gz + 1) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_WEST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                    hasInner = true;
                } else if (frontDir == STAIR_DIR_EAST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                    ix0 = 0.5f; ix1 = 1.0f; hasInner = true;
                }
            }
        }

        if (hasInner) {
            out[n][0] = ix0; out[n][1] = iy0; out[n][2] = iz0;
            out[n][3] = ix1; out[n][4] = iy1; out[n][5] = iz1;
            n++;
        }
    }
    return n;
}

int tileShapeBoxes(const World* w, int x, int y, int z, unsigned char id,
                   unsigned char data, float out[3][6]) {
    if (id == BLOCK_AIR) return 0;
    if (id == BLOCK_TOPSNOW) {
        SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 0.125f, z + 1.0f);
        return 1;
    } else if (id == BLOCK_WHEAT) {

        float yy1 = (data + 1) / 8.0f;
        SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + yy1, z + 1.0f);
        return 1;
    } else if (id == BLOCK_MELON_STEM) {

        float yy1 = (data * 2 + 2) / 16.0f;
        SET(out[0], x + 0.375f, y + 0.0f, z + 0.375f, x + 0.625f, y + yy1, z + 0.625f);
        return 1;
    } else if (id == BLOCK_SIGN) {

        SET(out[0], x + 0.25f, y + 0.0f, z + 0.25f, x + 0.75f, y + 1.0f, z + 0.75f);
        return 1;
    } else if (id == BLOCK_WALL_SIGN) {

        unsigned char face = worldData(w, x, y, z);
        const float h0 = 4.5f/16.0f, h1 = 12.5f/16.0f, d0 = 2.0f/16.0f;
        if (face == 2)      SET(out[0], x + 0.0f, y + h0, z + 1.0f - d0, x + 1.0f, y + h1, z + 1.0f);
        else if (face == 3) SET(out[0], x + 0.0f, y + h0, z + 0.0f, x + 1.0f, y + h1, z + d0);
        else if (face == 4) SET(out[0], x + 1.0f - d0, y + h0, z + 0.0f, x + 1.0f, y + h1, z + 1.0f);
        else                SET(out[0], x + 0.0f, y + h0, z + 0.0f, x + d0, y + h1, z + 1.0f);
        return 1;
    } else if (id == BLOCK_CAKE) {

        unsigned char data = worldData(w, x, y, z) & 7;
        SET(out[0], x + (2 * data + 1) / 16.0f, y + 0.0f, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 8.0f/16.0f, z + 15.0f/16.0f);
        return 1;
    } else if (isBed(id)) {
        SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 9.0f/16.0f, z + 1.0f);
        return 1;
    } else if (isSlab(id)) {
        float y0 = y + ((data & SLAB_TOP_SLOT_BIT) ? 0.5f : 0.0f);
        float y1 = y + ((data & SLAB_TOP_SLOT_BIT) ? 1.0f : 0.5f);
        SET(out[0], x + 0.0f, y0, z + 0.0f, x + 1.0f, y1, z + 1.0f);
        return 1;
    } else if (isStairs(id)) {

        float b[3][6];
        int nb = stairShapeBoxes(w, x, y, z, worldData(w, x, y, z), b);
        for (int i = 0; i < nb; i++)
            SET(out[i], x + b[i][0], y + b[i][1], z + b[i][2],
                        x + b[i][3], y + b[i][4], z + b[i][5]);
        return nb;
    } else if (isPane(id)) {

        auto att = [&](int bx, int by, int bz) -> bool {
            unsigned char nb = worldBlock(w, bx, by, bz);
            return isSolidPhys(nb) || isPane(nb) || isGlass(nb);
        };
        bool north = att(x, y, z - 1), south = att(x, y, z + 1);
        bool west  = att(x - 1, y, z), east  = att(x + 1, y, z);
        float minX = 7.0f/16.0f, maxX = 9.0f/16.0f, minZ = 7.0f/16.0f, maxZ = 9.0f/16.0f;
        if ((west && east) || (!west && !east && !north && !south)) { minX = 0.0f; maxX = 1.0f; }
        else if (west && !east) minX = 0.0f;
        else if (!west && east) maxX = 1.0f;
        if ((north && south) || (!west && !east && !north && !south)) { minZ = 0.0f; maxZ = 1.0f; }
        else if (north && !south) minZ = 0.0f;
        else if (!north && south) maxZ = 1.0f;
        SET(out[0], x + minX, y + 0.0f, z + minZ, x + maxX, y + 1.0f, z + maxZ);
        return 1;
    } else if (isFence(id)) {

        bool fn = connectsFence(worldBlock(w, x, y, z - 1));
        bool fs = connectsFence(worldBlock(w, x, y, z + 1));
        bool fw = connectsFence(worldBlock(w, x - 1, y, z));
        bool fe = connectsFence(worldBlock(w, x + 1, y, z));
        float fx0 = fw ? (float)x : x + 6.0f/16.0f;
        float fx1 = fe ? x + 1.0f : x + 10.0f/16.0f;
        float fz0 = fn ? (float)z : z + 6.0f/16.0f;
        float fz1 = fs ? z + 1.0f : z + 10.0f/16.0f;

        SET(out[0], fx0, (float)y, fz0, fx1, y + 1.0f, fz1);
        return 1;
    } else if (isDoor(id)) {
        bool isUpper = (data & 8) != 0;
        int lowerData = isUpper ? worldData(w, x, y - 1, z) : data;
        int upperData = isUpper ? data : worldData(w, x, y + 1, z);

        float r = 3.0f / 16.0f;
        int dir = lowerData & 3;
        bool open = (lowerData & 4) != 0;
        bool rightHinge = (upperData & 1) != 0;
        int shapeDir = 0;
        if (dir == 0) {
            if (open) {
                if (!rightHinge) shapeDir = 0;
                else shapeDir = 2;
            } else shapeDir = 3;
        } else if (dir == 1) {
            if (open) {
                if (!rightHinge) shapeDir = 1;
                else shapeDir = 3;
            } else shapeDir = 0;
        } else if (dir == 2) {
            if (open) {
                if (!rightHinge) shapeDir = 2;
                else shapeDir = 0;
            } else shapeDir = 1;
        } else if (dir == 3) {
            if (open) {
                if (!rightHinge) shapeDir = 3;
                else shapeDir = 1;
            } else shapeDir = 2;
        }

        float box_y0 = (float)y;
        float box_y1 = (float)(y + 1);

        if (shapeDir == 0) SET(out[0], x + 0.0f, box_y0, z + 0.0f, x + 1.0f, box_y1, z + r);
        else if (shapeDir == 1) SET(out[0], x + 1.0f - r, box_y0, z + 0.0f, x + 1.0f, box_y1, z + 1.0f);
        else if (shapeDir == 2) SET(out[0], x + 0.0f, box_y0, z + 1.0f - r, x + 1.0f, box_y1, z + 1.0f);
        else if (shapeDir == 3) SET(out[0], x + 0.0f, box_y0, z + 0.0f, x + r, box_y1, z + 1.0f);

        return 1;
    } else if (isTrapdoor(id)) {
        float sh[6];
        trapdoorShape(worldData(w, x, y, z), sh);
        SET(out[0], x + sh[0], y + sh[1], z + sh[2], x + sh[3], y + sh[4], z + sh[5]);
        return 1;
    } else if (isLadder(id)) {
        float r = 2.0f / 16.0f;
        if (data == 2) SET(out[0], x + 0.0f, y + 0.0f, z + 1.0f - r, x + 1.0f, y + 1.0f, z + 1.0f);
        else if (data == 3) SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + r);
        else if (data == 4) SET(out[0], x + 1.0f - r, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
        else if (data == 5) SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + r, y + 1.0f, z + 1.0f);
        else SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
        return 1;
    } else if (isTorch(id)) {
        float hw = 0.15f;
        if (data == 1) SET(out[0], x + 0.0f, y + 0.2f, z + 0.5f - hw, x + hw * 2.0f, y + 0.8f, z + 0.5f + hw);
        else if (data == 2) SET(out[0], x + 1.0f - hw * 2.0f, y + 0.2f, z + 0.5f - hw, x + 1.0f, y + 0.8f, z + 0.5f + hw);
        else if (data == 3) SET(out[0], x + 0.5f - hw, y + 0.2f, z + 0.0f, x + 0.5f + hw, y + 0.8f, z + hw * 2.0f);
        else if (data == 4) SET(out[0], x + 0.5f - hw, y + 0.2f, z + 1.0f - hw * 2.0f, x + 0.5f + hw, y + 0.8f, z + 1.0f);
        else SET(out[0], x + 0.5f - hw, y + 0.0f, z + 0.5f - hw, x + 0.5f + hw, y + 0.6f, z + 0.5f + hw);
        return 1;
    } else if (isFenceGate(id)) {

        int dir = data & 3;

        if (dir == 1 || dir == 3) {
            SET(out[0], x + 6.0f/16.0f, y + 0.0f, z + 0.0f, x + 10.0f/16.0f, y + 1.0f, z + 1.0f);
        } else {
            SET(out[0], x + 0.0f, y + 0.0f, z + 6.0f/16.0f, x + 1.0f, y + 1.0f, z + 10.0f/16.0f);
        }
        return 1;
    } else if (id == BLOCK_CACTUS) {

        float r = 1.0f/16.0f;
        SET(out[0], x + r, y + 0.0f, z + r, x + 1.0f - r, y + 1.0f, z + 1.0f - r);
        return 1;
    } else if (id == BLOCK_COCOA) {

        int age = (data >> COCOA_AGE_SHIFT) & COCOA_AGE_MASK;
        float size = 4.0f/16.0f + age * (2.0f/16.0f);
        float half = size / 2.0f;
        SET(out[0], x + 0.5f - half, y + 0.5f - half, z + 0.5f - half,
                    x + 0.5f + half, y + 0.5f + half, z + 0.5f + half);
        return 1;
    } else if (id == BLOCK_REEDS) {

        const float ss = 6.0f/16.0f;
        SET(out[0], x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + 1.0f, z + 0.5f + ss);
        return 1;
    } else if (id == BLOCK_CHEST) {

        float b[6]; chestShapeBox(w, x, y, z, b);
        SET(out[0], x + b[0], y + b[1], z + b[2], x + b[3], y + b[4], z + b[5]);
        return 1;
    } else if (id == BLOCK_FARMLAND) {

        SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 15.0f/16.0f, z + 1.0f);
        return 1;
    } else if (id == BLOCK_FLOWER || id == BLOCK_ROSE) {

        const float ss = 0.2f;
        SET(out[0], x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + ss * 3.0f, z + 0.5f + ss);
        return 1;
    } else if (id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED) {

        const float ss = 0.2f;
        SET(out[0], x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + ss * 2.0f, z + 0.5f + ss);
        return 1;
    } else if (id == BLOCK_SAPLING || id == BLOCK_TALLGRASS) {

        const float ss = 0.4f;
        SET(out[0], x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + 0.8f, z + 0.5f + ss);
        return 1;
    } else if (id == BLOCK_FIRE) {

        if (isSolidBlocking(worldBlock(w, x, y - 1, z)) || fireCanBurn(w, x, y - 1, z)) {
            SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
        } else {
            const float r = 0.2f;
            if      (fireCanBurn(w, x - 1, y, z)) SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + r, y + 1.0f, z + 1.0f);
            else if (fireCanBurn(w, x + 1, y, z)) SET(out[0], x + 1.0f - r, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
            else if (fireCanBurn(w, x, y, z - 1)) SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + r);
            else if (fireCanBurn(w, x, y, z + 1)) SET(out[0], x + 0.0f, y + 0.0f, z + 1.0f - r, x + 1.0f, y + 1.0f, z + 1.0f);
            else                                  SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
        }
        return 1;
    }

    SET(out[0], x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f);
    return 1;
}
