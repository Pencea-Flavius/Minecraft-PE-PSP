
#include "client/renderer/level/near_patch.h"
#include "client/renderer/level/near_patch_math.h"
#include "client/renderer/level/near_patch_cut.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "platform/dcache.h"
#include "gpu/gu.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

extern int g_lowMemPsp;

struct Budget { int pieceVerts; };

static const Budget BUDGET_NORMAL = { 32000 };
static const Budget BUDGET_LOWMEM = { 14000 };

static const float REBUILD_MOVE = 0.20f;
static const float MARGIN       = 0.60f;
static const float BOB_SLACK    = 0.15f;

static const unsigned int PICK_BUDGET_US = 700;

static const float SOURCE_EDGE = 1.42f;

static const int MAX_DEPTH = GRID_MAX_DEPTH;

static const float DIST_QUANTUM = 0.25f;

static const int DEPTH_BIAS = -80;

static const float INFLATE_PX = 0.75f;

static const int MAX_KEYS = 27;

#define NEAR_PATCH_DEBUG_TINT 0

static const unsigned int PIECE_FMT = GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D;

struct Pieces {
    PieceVertex* buf[2];
    int cap;
    int front;

    int beg[NEAR_PATCH_RANGES * MAX_KEYS], end[NEAR_PATCH_RANGES * MAX_KEYS];
    int n;
    bool valid;
};

static Pieces s_pieces;

static bool allocPieces(Pieces& p, int cap) {
    p.cap = cap;

    p.buf[0] = (PieceVertex*)memalign(64, (size_t)cap * sizeof(PieceVertex));
    p.buf[1] = p.buf[0] ? (PieceVertex*)memalign(64, (size_t)cap * sizeof(PieceVertex)) : 0;
    return p.buf[1] != 0;
}
static void freePieces(Pieces& p) {

    if (p.buf[0]) guDeferFree(p.buf[0]);
    if (p.buf[1]) guDeferFree(p.buf[1]);
    p.buf[0] = p.buf[1] = 0;
    p.valid = false;
}

struct SecKey { const ChunkSection* s; unsigned short gen; };

struct PickSet {
    SecKey keys[MAX_KEYS];
    int    nKeys;
    int    origin[MAX_KEYS][3];
    bool   ownWater[MAX_KEYS];

    int    pBeg[NEAR_PATCH_RANGES * MAX_KEYS], pEnd[NEAR_PATCH_RANGES * MAX_KEYS];
    float  pLo[NEAR_PATCH_RANGES * MAX_KEYS][3], pHi[NEAR_PATCH_RANGES * MAX_KEYS][3];
    int    pieceN;
    bool   pieceFull;
    bool   hadMissing;
    bool   started;
    float  ex, ey, ez;
};
static PickSet s_set[2];
static int     s_live = 0;
static bool    s_built = false;

static bool s_stageActive = false;
static int  s_stageNextKey = 0;
static int  s_stagePieceBuf = -1;
static unsigned int s_stageWorstUs = 0;

static float s_minEdge = 0.0f, s_safePerEdge = 0.0f, s_fov = 0.0f;

static float s_worldPerPxPerDepth = 0.0f;

static bool s_allocFailed = false;
static bool s_frameOpen   = false;
static int  s_frontAtFrame = 0;
static bool s_swappedThisFrame  = false;

static const ChunkSection* s_ownWater[MAX_KEYS];
static int s_nOwnWater = 0;

static bool s_drawSlot[NEAR_PATCH_RANGES * MAX_KEYS];
static unsigned int s_maskUs = 0;
static int s_drawnSlots = 0;

static const float BAND_SAFETY = 0.9f;
static float s_curEx = 0, s_curEy = 0, s_curEz = 0;
static unsigned int s_pickUs = 0;
static int  s_state = 2;

static inline float overscaleFor(int range) {
    return range == NEAR_PATCH_WATER ? SEAM_OVERSCALE_TRANS : SEAM_OVERSCALE_OPAQUE;
}

static int splitDepth(const float eye[3], const PV& a, const PV& b, const PV& c) {
    float lo[3] = { a.x, a.y, a.z }, hi[3] = { a.x, a.y, a.z };
    const PV* p[2] = { &b, &c };
    for (int k = 0; k < 2; k++) {
        const float v[3] = { p[k]->x, p[k]->y, p[k]->z };
        for (int i = 0; i < 3; i++) {
            lo[i] = v[i] < lo[i] ? v[i] : lo[i];
            hi[i] = v[i] > hi[i] ? v[i] : hi[i];
        }
    }
    float d2 = 0.0f;
    for (int i = 0; i < 3; i++) {
        float q = eye[i] < lo[i] ? lo[i] - eye[i] : (eye[i] > hi[i] ? eye[i] - hi[i] : 0.0f);
        d2 += q * q;
    }

    const float farEnough = s_safePerEdge * SOURCE_EDGE + MARGIN;
    if (d2 >= farEnough * farEnough) return 0;

    const float d = floorf(sqrtf(d2) / DIST_QUANTUM) * DIST_QUANTUM;
    float e2 = dist2(a, b);
    float t = dist2(b, c); if (t > e2) e2 = t;
    t = dist2(c, a);       if (t > e2) e2 = t;
    float edge = sqrtf(e2);

    int depth = 0;
    while (depth < MAX_DEPTH && edge > s_minEdge && d - MARGIN < s_safePerEdge * edge) {
        edge *= 0.5f;
        depth++;
    }
    return depth;
}

static inline int min3(int a, int b, int c) { int m = a < b ? a : b; return m < c ? m : c; }
static inline int max3(int a, int b, int c) { int m = a > b ? a : b; return m > c ? m : c; }

static const DrawVertex* layerOf(const ChunkSection* s, int range, int* first, int* end) {
    *first = 0;
    switch (range) {
    case NEAR_PATCH_OPAQUE: *end = s->vertexCount;    return s->mesh;
    case NEAR_PATCH_CUTOUT: *end = s->noMipLavaStart; return s->noMip;
    case NEAR_PATCH_LAVA:   *first = s->noMipLavaStart; *end = s->noMipCount; return s->noMip;
    case NEAR_PATCH_LEAVES: *end = s->leavesCount;    return s->leaves;
    default:                *end = s->waterCount;     return s->water;
    }
}

static float reachNow(void) { return s_safePerEdge * SOURCE_EDGE + MARGIN; }

template <class Fn>
static void forTriangles(const PickSet& ps, int k, int range, bool all, Fn fn) {
    const ChunkSection* s = ps.keys[k].s;
    int first, end;
    const DrawVertex* vb = layerOf(s, range, &first, &end);
    if (!vb || end - first < 3) return;
    const float reach = reachNow();
    const float eye[3] = { ps.ex, ps.ey, ps.ez };

    if (!all) {
        const float ox = (float)s->ox, oz = (float)s->oz, slack = 0.5f;
        if (eye[0] + reach < ox - slack || eye[0] - reach > ox + CHUNK_SZ + slack ||
            eye[2] + reach < oz - slack || eye[2] - reach > oz + CHUNK_SZ + slack ||
            eye[1] + reach < s->by0 - slack || eye[1] - reach > s->by1 + slack) return;
    }
    const float os = overscaleFor(range), inv = (float)POS_ENC / os;
    const int* o = ps.origin[k];

    const int lx0 = (int)((eye[0] - reach - o[0]) * inv) - 1, lx1 = (int)((eye[0] + reach - o[0]) * inv) + 1;
    const int ly0 = (int)((eye[1] - reach - o[1]) * inv) - 1, ly1 = (int)((eye[1] + reach - o[1]) * inv) + 1;
    const int lz0 = (int)((eye[2] - reach - o[2]) * inv) - 1, lz1 = (int)((eye[2] + reach - o[2]) * inv) + 1;

    const int pad = (int)(SOURCE_EDGE * inv) + 1;
    const int n = end - (end - first) % 3;
    for (int i = first; i < n; i += 3) {
        const DrawVertex& A = vb[i];
        if (!all && (A.x < lx0 - pad || A.x > lx1 + pad || A.z < lz0 - pad || A.z > lz1 + pad ||
                     A.y < ly0 - pad || A.y > ly1 + pad)) continue;
        const DrawVertex& B = vb[i + 1], &C = vb[i + 2];
        const bool inReach =
            !(min3(A.x, B.x, C.x) > lx1 || max3(A.x, B.x, C.x) < lx0 ||
              min3(A.z, B.z, C.z) > lz1 || max3(A.z, B.z, C.z) < lz0 ||
              min3(A.y, B.y, C.y) > ly1 || max3(A.y, B.y, C.y) < ly0);
        if (!inReach && !all) continue;
        PV a, b, c;
        if (all) {
            decode(A, o, os, a); decode(B, o, os, b); decode(C, o, os, c);
        } else {

            const float kk = os / (float)POS_ENC;
            a.x = A.x * kk + o[0]; a.y = A.y * kk + o[1]; a.z = A.z * kk + o[2];
            b.x = B.x * kk + o[0]; b.y = B.y * kk + o[1]; b.z = B.z * kk + o[2];
            c.x = C.x * kk + o[0]; c.y = C.y * kk + o[1]; c.z = C.z * kk + o[2];
        }
        if (!fn(i, a, b, c, inReach ? splitDepth(eye, a, b, c) : 0)) return;
    }
}

static int collectKeys(const World* w, float ex, float ey, float ez, float reach, SecKey* keys) {
    int n = 0;
    const int cx0 = ((int)floorf(ex - reach)) >> 4, cx1 = ((int)floorf(ex + reach)) >> 4;
    const int cz0 = ((int)floorf(ez - reach)) >> 4, cz1 = ((int)floorf(ez + reach)) >> 4;
    int sy0 = (int)floorf(ey - reach) / SECTION_SY, sy1 = (int)floorf(ey + reach) / SECTION_SY;
    if (ey - reach < 0.0f) sy0 = 0;
    if (sy1 > N_SECTIONS - 1) sy1 = N_SECTIONS - 1;
    for (int cx = cx0; cx <= cx1; cx++)
        for (int cz = cz0; cz <= cz1; cz++)
            for (int si = sy0; si <= sy1 && n < MAX_KEYS; si++) {
                const LevelChunk* slot = worldSlot(w, cx, cz);

                if (!slot->isAt(cx, cz) || worldSlotBusy(slot)) { keys[n].s = 0; keys[n].gen = 0; n++; continue; }
                const ChunkSection* s = &worldMesh(w, cx, cz)->sec[si];
                keys[n].s = s; keys[n].gen = s->gen; n++;
            }
    return n;
}

static bool setStale(const PickSet& ps) {

    if (ps.hadMissing) return true;
    for (int i = 0; i < ps.nKeys; i++)
        if (ps.keys[i].s && ps.keys[i].s->gen != ps.keys[i].gen) return true;
    return false;
}

static bool allocAll(void) {
    if (s_pieces.buf[0]) return true;
    if (s_allocFailed) return false;
    const Budget& b = g_lowMemPsp ? BUDGET_LOWMEM : BUDGET_NORMAL;
    if (!allocPieces(s_pieces, b.pieceVerts)) {
        freePieces(s_pieces);
        s_allocFailed = true;
        return false;
    }
    return true;
}

static float secDist2(const ChunkSection* s, float ex, float ey, float ez) {
    const float lo[3] = { (float)s->ox, s->by0, (float)s->oz };
    const float hi[3] = { (float)(s->ox + CHUNK_SZ), s->by1, (float)(s->oz + CHUNK_SZ) };
    const float e[3] = { ex, ey, ez };
    float d2 = 0.0f;
    for (int i = 0; i < 3; i++) {
        const float q = e[i] < lo[i] ? lo[i] - e[i] : (e[i] > hi[i] ? e[i] - hi[i] : 0.0f);
        d2 += q * q;
    }
    return d2;
}

static float dist2Eye(const PickSet& ps, float ex, float ey, float ez) {
    const float dx = ex - ps.ex, dy = ey - ps.ey, dz = ez - ps.ez;
    return dx * dx + dy * dy + dz * dz;
}

static void stageStart(const World* w, float ex, float ey, float ez) {
    PickSet& ps = s_set[1 - s_live];
    ps.ex = ex; ps.ey = ey; ps.ez = ez;
    ps.nKeys = collectKeys(w, ex, ey, ez, reachNow(), ps.keys);

    for (int i = 1; i < ps.nKeys; i++) {
        const SecKey kv = ps.keys[i];
        const float di = kv.s ? secDist2(kv.s, ex, ey, ez) : 1e30f;
        int j = i - 1;
        for (; j >= 0; j--) {
            const float dj = ps.keys[j].s ? secDist2(ps.keys[j].s, ex, ey, ez) : 1e30f;
            if (dj <= di) break;
            ps.keys[j + 1] = ps.keys[j];
        }
        ps.keys[j + 1] = kv;
    }
    for (int k = 0; k < ps.nKeys; k++) {
        ps.ownWater[k] = false;
        if (ps.keys[k].s) {
            ps.origin[k][0] = ps.keys[k].s->ox;
            ps.origin[k][1] = ps.keys[k].s->oy;
            ps.origin[k][2] = ps.keys[k].s->oz;
        }
    }
    for (int i = 0; i < NEAR_PATCH_RANGES * MAX_KEYS; i++) {
        ps.pBeg[i] = ps.pEnd[i] = 0;
        for (int j = 0; j < 3; j++) { ps.pLo[i][j] = 0.0f; ps.pHi[i][j] = 0.0f; }
    }
    ps.pieceN = 0; ps.pieceFull = false; ps.started = true;
    ps.hadMissing = false;
    for (int k = 0; k < ps.nKeys; k++) if (!ps.keys[k].s) { ps.hadMissing = true; break; }
    s_stagePieceBuf = 1 - s_frontAtFrame;
    s_stageNextKey = 0;
    s_stageWorstUs = 0;
    s_stageActive = true;
}

static void storeBox(PickSet& ps, int slot, const float lo[3], const float hi[3]) {
    const float SLACK = 0.05f;
    for (int j = 0; j < 3; j++) { ps.pLo[slot][j] = lo[j] - SLACK; ps.pHi[slot][j] = hi[j] + SLACK; }
}

static void stageStepKey(int k) {
    PickSet& ps = s_set[1 - s_live];
    Out o = { s_pieces.buf[s_stagePieceBuf], ps.pieceN, s_pieces.cap, false };
    const float worldPerLocal = SEAM_OVERSCALE_OPAQUE / (float)POS_ENC;
    for (int range = 0; range < NEAR_PATCH_RANGES; range++) {
        const int slot = range * MAX_KEYS + k;
        ps.pBeg[slot] = ps.pEnd[slot] = o.n;
        if (!ps.keys[k].s || ps.pieceFull) continue;

        float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
        const int slotFirst = o.n;

        if (range == NEAR_PATCH_WATER) {

            bool needs = false;
            forTriangles(ps, k, NEAR_PATCH_WATER, false,
                         [&](int, const PV&, const PV&, const PV&, int depth) {
                needs = depth > 0;
                return !needs;
            });
            if (!needs) continue;
            const int before = o.n;
            forTriangles(ps, k, NEAR_PATCH_WATER, true,
                         [&](int, const PV& a, const PV& b, const PV& c, int depth) {
                emitGrid(o, a, b, c, depth);
                return !o.full;
            });

            if (o.full) { o.n = before; ps.pieceFull = true; continue; }
            ps.ownWater[k] = true;
            ps.pEnd[slot] = o.n;

            const ChunkSection* sc = ps.keys[k].s;
            const float wlo[3] = { (float)sc->ox, sc->wby0, (float)sc->oz };
            const float whi[3] = { (float)(sc->ox + CHUNK_SZ), sc->wby1, (float)(sc->oz + CHUNK_SZ) };
            storeBox(ps, slot, wlo, whi);
            continue;
        }

        int first, end;
        const DrawVertex* vb = layerOf(ps.keys[k].s, range, &first, &end);
        if (!vb) continue;
        forTriangles(ps, k, range, false,
                     [&](int i, const PV& a, const PV& b, const PV& cc, int depth) {
            if (depth == 0) return true;

            if (range != NEAR_PATCH_LAVA) {
                const float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
                const float vx = cc.x - a.x, vy = cc.y - a.y, vz = cc.z - a.z;
                const float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                const float nl = sqrtf(nx * nx + ny * ny + nz * nz);
                if (nl > 0.0f &&
                    (nx * (ps.ex - a.x) + ny * (ps.ey - a.y) + nz * (ps.ez - a.z)) < -MARGIN * nl)
                    return true;
            }
            if (i + 3 > end) return true;

            PV p[3];
            float grow[3];
            const PV* q[3] = { &a, &b, &cc };
            for (int v = 0; v < 3; v++) {
                decode(vb[i + v], ps.origin[k], SEAM_OVERSCALE_OPAQUE, p[v]);

                const float dx = q[v]->x - ps.ex, dy = q[v]->y - ps.ey, dz = q[v]->z - ps.ez;
                float d = sqrtf(dx * dx + dy * dy + dz * dz);
                if (d < NEAR_PATCH_Z) d = NEAR_PATCH_Z;
                grow[v] = INFLATE_PX * d * s_worldPerPxPerDepth;
            }
#if NEAR_PATCH_DEBUG_TINT
            for (int v = 0; v < 3; v++) { p[v].g = p[v].g * 55 / 100; p[v].b = p[v].b * 80 / 100; }
#endif
            inflateCorners(p, grow, worldPerLocal);
            emitGrid(o, p[0], p[1], p[2], depth);
            if (o.full) { ps.pieceFull = true; return false; }

            for (int v = 0; v < 3; v++) {
                const float pos[3] = { p[v].x, p[v].y, p[v].z };
                for (int j = 0; j < 3; j++) {
                    if (pos[j] < lo[j]) lo[j] = pos[j];
                    if (pos[j] > hi[j]) hi[j] = pos[j];
                }
            }
            return true;
        });
        ps.pEnd[slot] = o.n;
        if (o.n > slotFirst) storeBox(ps, slot, lo, hi);
    }
    ps.pieceN = o.n;
}

static void stageCommit(void) {
    PickSet& ps = s_set[1 - s_live];
    if (ps.pieceN) dcacheFlush(s_pieces.buf[s_stagePieceBuf], (size_t)ps.pieceN * sizeof(PieceVertex));
    s_pieces.front = s_stagePieceBuf;
    s_pieces.n = ps.pieceN;
    s_pieces.valid = true;
    for (int i = 0; i < NEAR_PATCH_RANGES * MAX_KEYS; i++) {
        s_pieces.beg[i] = ps.pBeg[i];
        s_pieces.end[i] = ps.pEnd[i];
    }
    for (int i = 0; i < NEAR_PATCH_RANGES * MAX_KEYS; i++) s_drawSlot[i] = true;

    s_nOwnWater = 0;
    for (int k = 0; k < ps.nKeys; k++)
        if (ps.ownWater[k] && s_nOwnWater < MAX_KEYS) s_ownWater[s_nOwnWater++] = ps.keys[k].s;
    s_live = 1 - s_live;
    s_built = true;
    s_stageActive = false;
    s_swappedThisFrame = true;
    s_pickUs = s_stageWorstUs;
}

static void stageRun(unsigned int budgetUs) {
    const unsigned int t0 = sceKernelGetSystemTimeLow();
    PickSet& ps = s_set[1 - s_live];
    do {
        stageStepKey(s_stageNextKey++);
    } while (s_stageNextKey < ps.nKeys &&
             (budgetUs == 0 || sceKernelGetSystemTimeLow() - t0 < budgetUs));
    const unsigned int spent = sceKernelGetSystemTimeLow() - t0;
    if (spent > s_stageWorstUs) s_stageWorstUs = spent;
    if (s_stageNextKey >= ps.nKeys) stageCommit();
}

static void pickNow(const World* w, float ex, float ey, float ez) {
    stageStart(w, ex, ey, ez);
    stageRun(0);
}

static void computeDrawMask(void) {
    const unsigned int t0 = sceKernelGetSystemTimeLow();
    s_drawnSlots = 0;
    if (!s_built || !s_pieces.valid) { s_maskUs = 0; return; }
    ScePspFMatrix4 proj, view, clip;
    sceGumMatrixMode(GU_PROJECTION); sceGumStoreMatrix(&proj);
    sceGumMatrixMode(GU_VIEW);       sceGumStoreMatrix(&view);
    sceGumMatrixMode(GU_MODEL);
    gumMultMatrix(&clip, &proj, &view);
    const float* m = (const float*)&clip;
    const PickSet& ps = s_set[s_live];
    for (int r = 0; r < NEAR_PATCH_RANGES; r++)
        for (int k = 0; k < ps.nKeys; k++) {
            const int slot = r * MAX_KEYS + k;
            if (s_pieces.end[slot] <= s_pieces.beg[slot]) { s_drawSlot[slot] = false; continue; }

            if (r == NEAR_PATCH_WATER) { s_drawSlot[slot] = true; s_drawnSlots++; continue; }

            const float lo[3] = { ps.pLo[slot][0] - g_relBaseX, ps.pLo[slot][1] - g_relBaseY,
                                  ps.pLo[slot][2] - g_relBaseZ };
            const float hi[3] = { ps.pHi[slot][0] - g_relBaseX, ps.pHi[slot][1] - g_relBaseY,
                                  ps.pHi[slot][2] - g_relBaseZ };
            s_drawSlot[slot] = !nearpatch::boxAllInBand(m, lo, hi, NEAR_PATCH_Z, BAND_SAFETY);
            if (s_drawSlot[slot]) s_drawnSlots++;
        }
    s_maskUs = sceKernelGetSystemTimeLow() - t0;
}

static bool coversFrame(void) {
    return s_built && s_pieces.valid && !s_set[s_live].pieceFull;
}

void nearPatchReserve(void) { allocAll(); }

void nearPatchSplitParams(float* minEdge, float* safePerEdge) {
    if (s_safePerEdge > 0.0f) { *minEdge = s_minEdge; *safePerEdge = s_safePerEdge; return; }

    *minEdge     = nearpatch::minEdge(NEAR_PATCH_Z, 60.0f);
    *safePerEdge = nearpatch::safeDist(1.0f, 70.0f);
}

bool nearPatchUpdate(const World* w, float ex, float ey, float ez, float fov) {
    s_frameOpen = true;
    s_frontAtFrame = s_pieces.front;
    s_swappedThisFrame = false;
    s_curEx = ex; s_curEy = ey; s_curEz = ez;
    if (!allocAll()) { s_state = 2; return false; }
    if (fov != s_fov || s_safePerEdge == 0.0f) {
        s_fov = fov;
        s_minEdge     = nearpatch::minEdge(NEAR_PATCH_Z, fov);
        s_safePerEdge = nearpatch::safeDist(1.0f, fov);
        s_worldPerPxPerDepth = nearpatch::tanV(fov) / nearpatch::HALF_H_PX;
        pickNow(w, ex, ey, ez);
    } else if (!s_built) {
        pickNow(w, ex, ey, ez);
    } else {
        const float safe = MARGIN - BOB_SLACK;
        const bool liveExpired = dist2Eye(s_set[s_live], ex, ey, ez) > safe * safe;
        if (s_stageActive) {
            const PickSet& st = s_set[1 - s_live];
            if (dist2Eye(st, ex, ey, ez) > safe * safe) {
                pickNow(w, ex, ey, ez);
            } else if (liveExpired) {
                stageRun(0);
            }
        } else if (liveExpired) {
            pickNow(w, ex, ey, ez);
        } else if (setStale(s_set[s_live])) {

            stageStart(w, ex, ey, ez);
            stageRun(PICK_BUDGET_US);
        }
    }
    s_state = coversFrame() ? 0 : 1;
    return coversFrame();
}

void nearPatchRefresh(const World* w) {
    if (!s_frameOpen) return;
    s_frameOpen = false;
    if (!s_pieces.buf[0]) return;

    if (s_built && s_stageActive) {

        if (setStale(s_set[1 - s_live])) s_stageActive = false;
        else                             stageRun(PICK_BUDGET_US);
    }
    if (s_built && !s_stageActive && !s_swappedThisFrame &&
        dist2Eye(s_set[s_live], s_curEx, s_curEy, s_curEz) > REBUILD_MOVE * REBUILD_MOVE) {

        stageStart(w, s_curEx, s_curEy, s_curEz);
        stageRun(PICK_BUDGET_US);
    }
    computeDrawMask();
    s_state = coversFrame() ? 0 : 1;
}

bool nearPatchOwnsWater(const ChunkSection* sec) {
    if (!s_pieces.valid || !s_built) return false;
    for (int i = 0; i < s_nOwnWater; i++)
        if (s_ownWater[i] == sec) return true;
    return false;
}

bool nearPatchHas(int range) {
    if (!s_pieces.valid || !s_built) return false;
    for (int k = 0; k < MAX_KEYS; k++) {
        const int slot = range * MAX_KEYS + k;
        if (s_drawSlot[slot] && s_pieces.end[slot] > s_pieces.beg[slot]) return true;
    }
    return false;
}

void nearPatchDraw(int range) {
    if (!nearPatchHas(range)) return;
    const Pieces& p = s_pieces;
    const PickSet& ps = s_set[s_live];

    const bool bias = range != NEAR_PATCH_WATER;
    if (bias) sceGuDepthOffset(DEPTH_BIAS);
    for (int k = 0; k < ps.nKeys; k++) {
        const int slot = range * MAX_KEYS + k;
        if (!s_drawSlot[slot]) continue;
        const int first = p.beg[slot], end = p.end[slot];
        if (end <= first) continue;

        chunkSetModelOrigin(ps.origin[k][0], ps.origin[k][1], ps.origin[k][2], overscaleFor(range));
        sceGumDrawArray(GU_TRIANGLES, PIECE_FMT, end - first, 0, p.buf[p.front] + first);
    }
    if (bias) sceGuDepthOffset(0);
}

void nearPatchFree(void) {
    freePieces(s_pieces);
    s_set[0].nKeys = s_set[1].nKeys = 0;
    s_set[0].started = s_set[1].started = false;
    s_nOwnWater = 0;
    for (int i = 0; i < NEAR_PATCH_RANGES * MAX_KEYS; i++) s_drawSlot[i] = false;
    s_allocFailed = false;
    s_built = false; s_stageActive = false;
    s_frameOpen = false;
    s_safePerEdge = 0.0f;
    s_state = 2;
}

void nearPatchStats(int* verts, int* cands, unsigned int* buildUs, unsigned int* frameUs, int* state) {
    *verts = s_pieces.valid ? s_pieces.n : 0;
    *cands = s_drawnSlots;
    *buildUs = s_pickUs;
    *frameUs = s_maskUs;
    *state = s_state;
}
