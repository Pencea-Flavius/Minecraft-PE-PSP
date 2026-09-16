#include "client/skin/skin_pack.h"

#include <pspgu.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum { P_NONE = -1, P_DISPLAYNAME, P_THEMENAME, P_CAPEPATH, P_BOX, P_ANIM, P_OFFSET };

enum { T_SKIN = 0, T_CAPE = 1, T_LOCALISATION = 6, T_SKINPACK = 11 };

static bool locDisplayName(const unsigned char* d, unsigned int n, char* out, int cap) {
    unsigned int o = 0;
    auto rdInt = [&](int* v) {
        if (o + 4 > n) return false;
        *v = (int)((d[o] << 24) | (d[o + 1] << 16) | (d[o + 2] << 8) | d[o + 3]); o += 4; return true;
    };
    auto rdUtf = [&](const unsigned char** s, int* len) {
        if (o + 2 > n) return false;
        *len = (d[o] << 8) | d[o + 1]; o += 2;
        if (o + (unsigned)*len > n) return false;
        *s = d + o; o += *len; return true;
    };
    int version, langs, keys, len;
    const unsigned char* str;
    if (!rdInt(&version) || !rdInt(&langs) || langs <= 0 || langs > 64) return false;
    if (o + 1 > n) return false;
    o += 1;
    if (!rdInt(&keys) || keys <= 0 || keys > 100000) return false;
    int keyIdx = -1;
    for (int k = 0; k < keys; k++) {
        if (!rdUtf(&str, &len)) return false;
        if (keyIdx < 0 && len == 16 && memcmp(str, "IDS_DISPLAY_NAME", 16) == 0) keyIdx = k;
    }
    if (keyIdx < 0) return false;
    unsigned int blobStart[64], want = 0;
    unsigned int at = 0;
    for (int l = 0; l < langs; l++) {
        int size;
        if (!rdUtf(&str, &len) || !rdInt(&size) || size < 0) return false;
        blobStart[l] = at;
        if (len == 5 && memcmp(str, "en-EN", 5) == 0) want = (unsigned)l;
        at += (unsigned)size;
    }
    o += blobStart[want];
    int count, lv;
    if (!rdInt(&lv) || o + 1 > n) return false;
    o += 1;
    if (!rdUtf(&str, &len) || !rdInt(&count) || keyIdx >= count) return false;
    for (int k = 0; k <= keyIdx; k++)
        if (!rdUtf(&str, &len)) return false;
    int m = 0;
    for (int i = 0; i < len && m < cap - 1; i++)
        out[m++] = str[i] < 0x80 ? (char)str[i] : '?';
    out[m] = 0;
    return m > 0;
}

static bool s_be = false;

static unsigned int skinIdAnimOverride(const char* file, unsigned int fromFile) {
    if (strncmp(file, "dlcskin", 7) != 0) return fromFile;
    unsigned int id = (unsigned int)strtoul(file + 7, 0, 10);
    if (id >= 20000000u) id -= 20000000u;
    switch (id) {
    case 0x2:
    case 0x3:
    case 0xc8:
    case 0xc9:
    case 0x1f8:
    case 0x220:
    case 0x23a:
    case 0x23d:
    case 0x247:
    case 0x194:
    case 0x195:
        return 1u << SKIN_ANIM_ARMS_OUT_FRONT;
    case 0x1fa:
        return (1u << SKIN_ANIM_ARMS_OUT_FRONT) | (1u << SKIN_ANIM_NO_LEG_ANIM);
    case 0x1f4:
        return (1u << SKIN_ANIM_ARMS_DOWN) | (1u << SKIN_ANIM_NO_LEG_ANIM);
    case 0x1f7:
        return 0;
    default:
        return fromFile;
    }
}

static bool rd32(FILE* f, unsigned int* v) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    *v = s_be ? ((unsigned)b[0] << 24) | ((unsigned)b[1] << 16) | ((unsigned)b[2] << 8) | b[3]
              : ((unsigned)b[3] << 24) | ((unsigned)b[2] << 16) | ((unsigned)b[1] << 8) | b[0];
    return true;
}

static bool rdWstr(FILE* f, unsigned int nch, char* out, int cap) {
    if (nch > 1024) return false;
    int n = 0;
    bool ended = false;
    for (unsigned int i = 0; i < nch; i++) {
        unsigned char c[2];
        if (fread(c, 1, 2, f) != 2) return false;
        unsigned int w = s_be ? ((unsigned)c[0] << 8) | c[1] : c[0] | ((unsigned)c[1] << 8);
        if (w == 0) ended = true;
        if (ended || n >= cap - 1) continue;
        out[n++] = (w < 0x80) ? (char)w : '?';
    }
    out[n] = 0;
    return fseek(f, 4, SEEK_CUR) == 0;
}

static void copyStr(char* dst, int cap, const char* src) {
    size_t n = strlen(src);
    if (n > (size_t)cap - 1) n = (size_t)cap - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}

static bool parseBox(const char* s, SkinBox* b) {
    char part[10];
    if (sscanf(s, "%9s %f %f %f %f %f %f %f %f", part,
               &b->x, &b->y, &b->z, &b->w, &b->h, &b->d, &b->u, &b->v) != 9)
        return false;
    static const char* names[6] = { "HEAD", "BODY", "ARM0", "ARM1", "LEG0", "LEG1" };
    for (int i = 0; i < 6; i++)
        if (strcmp(part, names[i]) == 0) { b->part = (unsigned char)i; return true; }
    return false;
}

void skinPackClose(SkinPack* p) {
    free(p->skins);
    free(p->boxes);
    free(p->capes);
    memset(p, 0, sizeof(*p));
}

static bool skinPackRead(FILE* f, unsigned int base, SkinPack* out, unsigned int* nested) {
    struct Detail { unsigned int size, type; char file[32]; };
    Detail* details = 0;
    int boxCap = 0;
    bool ok = false;
    unsigned int version, nNames, nFiles;
    int map[32][2];
    int nMap = 0;
    bool hasXmlVersion = false;
    char buf[256];

    if (nested) *nested = 0;
    {

        unsigned char v0[4];
        if (fseek(f, (long)base, SEEK_SET) != 0 || fread(v0, 1, 4, f) != 4) goto done;
        s_be = (v0[0] == 0 && v0[1] == 0);
        fseek(f, (long)base, SEEK_SET);
    }
    if (!rd32(f, &version) || version < 3) goto done;
    if (!rd32(f, &nNames) || nNames > 64) goto done;
    for (unsigned int i = 0; i < nNames; i++) {
        unsigned int id, nch;
        if (!rd32(f, &id) || !rd32(f, &nch) || !rdWstr(f, nch, buf, sizeof(buf))) goto done;
        static const char* kNames[] = { "DISPLAYNAME", "THEMENAME", "CAPEPATH", "BOX", "ANIM", "OFFSET" };
        for (int k = 0; k < 6; k++)
            if (strcmp(buf, kNames[k]) == 0 && nMap < 32) { map[nMap][0] = (int)id; map[nMap][1] = k; nMap++; }

        if (strcmp(buf, "XMLVERSION") == 0) hasXmlVersion = true;
    }
    if (hasXmlVersion) { unsigned int xmlv; if (!rd32(f, &xmlv)) goto done; }

    if (!rd32(f, &nFiles) || nFiles > 1024) goto done;
    details = (Detail*)malloc(sizeof(Detail) * (nFiles ? nFiles : 1));
    out->skins = (SkinEntry*)calloc(SKIN_MAX_SKINS, sizeof(SkinEntry));
    out->capes = (SkinPack::Cape*)calloc(SKIN_MAX_SKINS, sizeof(SkinPack::Cape));
    if (!details || !out->skins || !out->capes) goto done;
    for (unsigned int i = 0; i < nFiles; i++) {
        unsigned int nch;
        Detail& d = details[i];
        if (!rd32(f, &d.size) || !rd32(f, &d.type) || !rd32(f, &nch) ||
            !rdWstr(f, nch, d.file, sizeof(d.file))) goto done;
    }

    for (unsigned int i = 0; i < nFiles; i++) {
        Detail& d = details[i];
        SkinEntry* sk = 0;
        if (d.type == T_SKIN && out->skinCount < SKIN_MAX_SKINS) {
            sk = &out->skins[out->skinCount];
            copyStr(sk->file, sizeof(sk->file), d.file);
            sk->boxFirst = out->boxCount;
        }
        unsigned int nParams;
        if (!rd32(f, &nParams) || nParams > 256) goto done;
        for (unsigned int j = 0; j < nParams; j++) {
            unsigned int id, nch;
            if (!rd32(f, &id) || !rd32(f, &nch) || !rdWstr(f, nch, buf, sizeof(buf))) goto done;
            if (!sk) continue;
            int kind = P_NONE;
            for (int k = 0; k < nMap; k++) if (map[k][0] == (int)id) kind = map[k][1];
            switch (kind) {
            case P_DISPLAYNAME:

                copyStr(sk->name, sizeof(sk->name),
                        strcmp(sk->file, "dlcskin00000109.png") == 0 ? "Zap" : buf);
                break;
            case P_THEMENAME: copyStr(sk->theme, sizeof(sk->theme), buf); break;
            case P_CAPEPATH:  copyStr(sk->capeFile, sizeof(sk->capeFile), buf); break;
            case P_ANIM:      sk->anim = (unsigned int)strtoul(buf, 0, 16); break;
            case P_OFFSET: {

                char part[16], axis[4];
                float v;
                if (sscanf(buf, "%15s %3s %f", part, axis, &v) == 3 &&
                    strcmp(part, "HELMET") == 0 && strcmp(axis, "Y") == 0)
                    sk->helmetY = v;
                break;
            }
            case P_BOX: {
                SkinBox b;
                if (sk->boxCount >= SKIN_MAX_BOXES || !parseBox(buf, &b)) break;
                if (out->boxCount == boxCap) {
                    int nc = boxCap ? boxCap * 2 : 64;
                    SkinBox* nb = (SkinBox*)realloc(out->boxes, sizeof(SkinBox) * nc);
                    if (!nb) break;
                    out->boxes = nb; boxCap = nc;
                }
                out->boxes[out->boxCount++] = b;
                sk->boxCount++;
                break;
            }
            default: break;
            }
        }
        if (sk) sk->anim = skinIdAnimOverride(sk->file, sk->anim);

        unsigned int offset = (unsigned int)ftell(f);
        if (sk) {
            unsigned char hdr[24];
            int pw = 0, ph = 0;
            if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr) &&
                hdr[1] == 'P' && hdr[2] == 'N' && hdr[3] == 'G') {
                pw = (hdr[16] << 24) | (hdr[17] << 16) | (hdr[18] << 8) | hdr[19];
                ph = (hdr[20] << 24) | (hdr[21] << 16) | (hdr[22] << 8) | hdr[23];
            }

            if (pw == 64 && (ph == 32 || ph == 64)) {
                sk->offset = offset; sk->size = d.size;
                sk->texH = (short)ph;
                out->skinCount++;
            } else {
                out->boxCount = sk->boxFirst;
                memset(sk, 0, sizeof(*sk));
            }
        } else if (d.type == T_LOCALISATION && !out->name[0] && d.size < 1024 * 1024) {
            unsigned char* blob = (unsigned char*)malloc(d.size);
            if (blob && fread(blob, 1, d.size, f) == d.size)
                locDisplayName(blob, d.size, out->name, sizeof(out->name));
            free(blob);
        } else if (d.type == T_SKINPACK && nested && !*nested) {
            *nested = offset;
        } else if (d.type == T_CAPE && out->capeCount < SKIN_MAX_SKINS) {
            SkinPack::Cape& c = out->capes[out->capeCount++];
            copyStr(c.file, sizeof(c.file), d.file);
            c.offset = offset; c.size = d.size;
        }
        if (fseek(f, (long)(offset + d.size), SEEK_SET) != 0) goto done;
    }
    ok = out->skinCount > 0;

done:
    free(details);
    return ok;
}

bool skinPackOpen(const char* path, SkinPack* out) {
    memset(out, 0, sizeof(*out));
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    copyStr(out->path, sizeof(out->path), path);

    unsigned int nested = 0;
    bool ok = skinPackRead(f, 0, out, &nested);
    if (!ok && nested) ok = skinPackRead(f, nested, out, 0);

    if (ok) {
        printf("skinPackOpen %s: %d skins, %d boxes, %d capes\n",
               path, out->skinCount, out->boxCount, out->capeCount);
    }
    fclose(f);
    if (!ok) skinPackClose(out);
    return ok;
}

bool skinPackLoadTexture(const SkinPack& p, int idx, Texture* out) {
    if (idx < 0 || idx >= p.skinCount) return false;
    return textureLoad16At(p.path, p.skins[idx].offset, out, GU_PSM_5551);
}

bool skinPackLoadCape(const SkinPack& p, int idx, Texture* out) {
    if (idx < 0 || idx >= p.skinCount || !p.skins[idx].capeFile[0]) return false;
    for (int i = 0; i < p.capeCount; i++)
        if (strcmp(p.capes[i].file, p.skins[idx].capeFile) == 0)
            return textureLoad16At(p.path, p.capes[i].offset, out, GU_PSM_5551);
    return false;
}

const char* skinDefaultName(int i) {
    static const char* names[SKIN_DEFAULT_COUNT] = {
        "Steve", "Tennis Steve", "Tuxedo Steve", "Athlete Steve",
        "Scottish Steve", "Prisoner Steve", "Cyclist Steve", "Boxer Steve",
    };
    return (i >= 0 && i < SKIN_DEFAULT_COUNT) ? names[i] : "";
}

bool skinDefaultLoadTexture(int i, Texture* out) {
    if (i < 0 || i >= SKIN_DEFAULT_COUNT) return false;
    char path[64];
    if (i == 0) snprintf(path, sizeof(path), "data/images/skins/char.png");
    else        snprintf(path, sizeof(path), "data/images/skins/char%d.png", i);
    return textureLoad16(path, out, GU_PSM_5551);
}

static char s_favs[SKIN_MAX_FAVORITES][96];
static int  s_favCount = 0;
static int  s_favPos = 0;

int skinFavoriteCount(void) { return s_favCount; }
const char* skinFavorite(int i) { return (i >= 0 && i < s_favCount) ? s_favs[i] : ""; }

int skinFavoriteFind(const char* entry) {
    for (int i = 0; i < s_favCount; i++) if (strcmp(s_favs[i], entry) == 0) return i;
    return -1;
}

void skinFavoriteAdd(const char* entry) {
    int at = skinFavoriteFind(entry);
    if (at >= 0) { s_favPos = at; return; }
    if (s_favCount < SKIN_MAX_FAVORITES) {
        at = s_favCount++;
    } else {
        at = (s_favPos + 1) % SKIN_MAX_FAVORITES;
    }
    copyStr(s_favs[at], sizeof(s_favs[0]), entry);
    s_favPos = at;
}

void skinFavoriteRemove(int i) {
    if (i < 0 || i >= s_favCount) return;
    for (int k = i; k + 1 < s_favCount; k++) strcpy(s_favs[k], s_favs[k + 1]);
    s_favCount--;
    if (s_favPos >= s_favCount) s_favPos = s_favCount > 0 ? s_favCount - 1 : 0;
}

void skinFavoritesOptionSet(const char* value) {
    s_favCount = 0;
    s_favPos = 0;
    char buf[SKIN_MAX_FAVORITES * 96];
    copyStr(buf, sizeof(buf), value);
    buf[strcspn(buf, "\r\n")] = 0;
    for (char* tok = strtok(buf, "|"); tok && s_favCount < SKIN_MAX_FAVORITES; tok = strtok(0, "|"))
        if (tok[0] && strchr(tok, '/')) copyStr(s_favs[s_favCount++], sizeof(s_favs[0]), tok);
    if (s_favCount) s_favPos = s_favCount - 1;
}

void skinFavoritesOptionGet(char* out, int cap) {
    int n = 0;
    out[0] = 0;
    for (int i = 0; i < s_favCount; i++)
        n += snprintf(out + n, n < cap ? cap - n : 0, "%s%s", i ? "|" : "", s_favs[i]);
}

static char      s_option[96];
static bool      s_loaded = false;
static bool      s_haveTex = false;
static Texture   s_tex;
static SkinPack  s_pack;
static int       s_index = -1;
static Texture   s_capeTex;
static bool      s_haveCape = false;
static unsigned  s_generation = 1;

void skinOptionSet(const char* value) {
    char v[sizeof(s_option)];
    copyStr(v, sizeof(v), value);
    v[strcspn(v, "\r\n")] = 0;
    if (s_loaded && strcmp(v, s_option) == 0) return;
    copyStr(s_option, sizeof(s_option), v);
    s_loaded = false;
}

const char* skinOptionGet(void) { return s_option; }

static void loadChosen(void) {
    s_loaded = true;
    s_generation++;
    if (s_haveTex) textureFree(&s_tex);
    s_haveTex = false;
    if (s_haveCape) textureFree(&s_capeTex);
    s_haveCape = false;
    skinPackClose(&s_pack);
    s_index = -1;

    if (strncmp(s_option, "default:", 8) == 0) {
        s_haveTex = skinDefaultLoadTexture(atoi(s_option + 8), &s_tex);
        if (s_haveTex) return;
    }
    const char* colon = strrchr(s_option, ':');
    if (s_option[0] && colon && strncmp(s_option, "default:", 8) != 0) {
        char path[160];
        snprintf(path, sizeof(path), "data/skinpacks/%.*s", (int)(colon - s_option), s_option);
        int idx = atoi(colon + 1);
        if (skinPackOpen(path, &s_pack) && skinPackLoadTexture(s_pack, idx, &s_tex)) {
            s_index = idx;
            s_haveTex = true;

            s_haveCape = skinPackLoadCape(s_pack, idx, &s_capeTex);
            return;
        }
        printf("skin %s not usable, falling back to the default skin\n", s_option);
        skinPackClose(&s_pack);
    }

    s_haveTex = textureLoad16("data/images/skins/skin.png", &s_tex, GU_PSM_5551) ||
                textureLoad16("data/images/skins/char.png", &s_tex, GU_PSM_5551);
}

Texture* skinTexture(void) {
    if (!s_loaded) loadChosen();
    return s_haveTex ? &s_tex : 0;
}

Texture* skinCapeTexture(void) {
    if (!s_loaded) loadChosen();
    return s_haveCape ? &s_capeTex : 0;
}

const SkinEntry* skinCurrent(void) {
    if (!s_loaded) loadChosen();
    return s_index >= 0 ? &s_pack.skins[s_index] : 0;
}

int skinCurrentPackSize(void) {
    return skinCurrent() ? s_pack.skinCount : 0;
}

const SkinBox* skinBoxes(void) {
    const SkinEntry* e = skinCurrent();
    return e && s_pack.boxes ? s_pack.boxes + e->boxFirst : 0;
}

unsigned int skinAnim(void) {
    const SkinEntry* e = skinCurrent();
    return e ? e->anim : 0;
}

unsigned int skinGeneration(void) {
    if (!s_loaded) loadChosen();
    return s_generation;
}
