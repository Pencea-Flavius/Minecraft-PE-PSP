
#ifndef MCPSP_WORLD_PHYS_AABB_H
#define MCPSP_WORLD_PHYS_AABB_H

struct AABB {
    float x0, y0, z0;
    float x1, y1, z1;

    AABB() : x0(0), y0(0), z0(0), x1(1), y1(1), z1(1) {}
    AABB(float x0, float y0, float z0, float x1, float y1, float z1)
        : x0(x0), y0(y0), z0(z0), x1(x1), y1(y1), z1(z1) {}

    AABB* set(float ax0, float ay0, float az0, float ax1, float ay1, float az1) {
        x0 = ax0; y0 = ay0; z0 = az0; x1 = ax1; y1 = ay1; z1 = az1;
        return this;
    }
    void set(const AABB& b) { x0=b.x0; y0=b.y0; z0=b.z0; x1=b.x1; y1=b.y1; z1=b.z1; }

    AABB expand(float xa, float ya, float za) const {
        float _x0 = x0, _y0 = y0, _z0 = z0, _x1 = x1, _y1 = y1, _z1 = z1;
        if (xa < 0) _x0 += xa;
        if (xa > 0) _x1 += xa;
        if (ya < 0) _y0 += ya;
        if (ya > 0) _y1 += ya;
        if (za < 0) _z0 += za;
        if (za > 0) _z1 += za;
        return AABB(_x0, _y0, _z0, _x1, _y1, _z1);
    }
    AABB grow(float xa, float ya, float za) const {
        return AABB(x0 - xa, y0 - ya, z0 - za, x1 + xa, y1 + ya, z1 + za);
    }
    AABB cloneMove(float xa, float ya, float za) const {
        return AABB(x0 + xa, y0 + ya, z0 + za, x1 + xa, y1 + ya, z1 + za);
    }

    static constexpr float EPS = 1.0e-3f;
    float clipXCollide(const AABB& c, float xa) const {
        if (c.y1 <= y0 + EPS || c.y0 >= y1 - EPS) return xa;
        if (c.z1 <= z0 + EPS || c.z0 >= z1 - EPS) return xa;
        if (xa > 0 && c.x1 <= x0 + EPS) { float m = x0 - c.x1; if (m < xa) xa = m; }
        if (xa < 0 && c.x0 >= x1 - EPS) { float m = x1 - c.x0; if (m > xa) xa = m; }
        return xa;
    }
    float clipYCollide(const AABB& c, float ya) const {
        if (c.x1 <= x0 + EPS || c.x0 >= x1 - EPS) return ya;
        if (c.z1 <= z0 + EPS || c.z0 >= z1 - EPS) return ya;
        if (ya > 0 && c.y1 <= y0 + EPS) { float m = y0 - c.y1; if (m < ya) ya = m; }
        if (ya < 0 && c.y0 >= y1 - EPS) { float m = y1 - c.y0; if (m > ya) ya = m; }
        return ya;
    }
    float clipZCollide(const AABB& c, float za) const {
        if (c.x1 <= x0 + EPS || c.x0 >= x1 - EPS) return za;
        if (c.y1 <= y0 + EPS || c.y0 >= y1 - EPS) return za;
        if (za > 0 && c.z1 <= z0 + EPS) { float m = z0 - c.z1; if (m < za) za = m; }
        if (za < 0 && c.z0 >= z1 - EPS) { float m = z1 - c.z0; if (m > za) za = m; }
        return za;
    }

    bool intersects(const AABB& c) const {
        if (c.x1 <= x0 || c.x0 >= x1) return false;
        if (c.y1 <= y0 || c.y0 >= y1) return false;
        if (c.z1 <= z0 || c.z0 >= z1) return false;
        return true;
    }
    bool intersects(float ax0, float ay0, float az0, float ax1, float ay1, float az1) const {
        if (ax1 <= x0 || ax0 >= x1) return false;
        if (ay1 <= y0 || ay0 >= y1) return false;
        if (az1 <= z0 || az0 >= z1) return false;
        return true;
    }

    AABB* move(float xa, float ya, float za) {
        x0 += xa; y0 += ya; z0 += za; x1 += xa; y1 += ya; z1 += za;
        return this;
    }

    float getSize() const { return ((x1-x0) + (y1-y0) + (z1-z0)) / 3.0f; }
};

#endif
