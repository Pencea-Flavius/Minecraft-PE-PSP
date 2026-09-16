#include "world/level/storage/level_storage.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/storage/region_file.h"
#include "world/level/storage/chunk_storage.h"
#include "world/level/chunk/chunk_cache.h"
#include "world/level/levelgen/mcpegen.h"

#include "world/level/world.h"
#include "util/mth.h"
#include "world/level/level.h"
#include "world/entity/entity.h"
#include "world/entity/entity_factory.h"
#include "world/level/tile/entity/tile_entity.h"
#include "world/level/tile/entity/sign_tile_entity.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/level/tile/entity/reactor_tile_entity.h"
#include "world/inventory/inventory.h"
#include "world/item/item_instance.h"
#include "client/player/player_state.h"

#include "nbt/nbt_io.h"

#include <cstdio>

#define STORAGE_LOG 0
#if STORAGE_LOG
#define LOGI printf
#else
#define LOGI(...) ((void)0)
#endif
#include <cstdlib>
#include <cstring>
#include <string>

static const int STORAGE_VERSION = 3;

static std::string join(const char* dir, const char* name) {
    std::string s = dir; s += "/"; s += name; return s;
}

static char s_activeDir[320] = "";
static char s_activeName[64] = "World";
static long s_activeSeed = 0;
static int  s_activeGameType = 1;
static int  s_activeWorldType = WORLD_TYPE_OLD;
static int  s_activeGenMask = GEN_FEATURES_ALL_ON;

extern bool g_saveShowProgress;
static void saveChunks(World* w, bool onlyDirty) {

    for (int i = 0; i < w->slotN * w->slotN; i++) {
        LevelChunk* c = &w->slots[i];
        if (!c->resident) continue;

        if (worldSlotBusy(c)) continue;
        if (onlyDirty && !c->unsaved) continue;
        if (!chunkStorageSave(w, c->x, c->z)) {

            LOGI("LevelStorage: chunk write failed, dropping meshes\n");
            for (int k = 0; k < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; k++) {
                chunkFreeMesh(&w->chunks[k]);
                for (int si = 0; si < N_SECTIONS; si++) w->chunks[k].sec[si].dirty = true;
            }
            chunkStorageSave(w, c->x, c->z);
        }
        if (g_saveShowProgress) g_terrainProgress = (i + 1) * 100 / (w->slotN * w->slotN);
    }
}

static void loadChunks(World* w, int cxCentre, int czCentre) {
    int done = 0, total = w->slotN * w->slotN;

    if (worldFitsInWindow(w)) {
        for (int cz = 0; cz < WORLD_SIZE_CHUNKS; cz++)
            for (int cx = 0; cx < WORLD_SIZE_CHUNKS; cx++) {
                worldGetChunk(w, cx, cz);
                g_terrainProgress = (++done) * 60 / total;
            }
        return;
    }
    const int R = w->slotN / 2;
    for (int dz = -R; dz < R; dz++)
        for (int dx = -R; dx < R; dx++) {
            worldGetChunk(w, cxCentre + dx, czCentre + dz);
            g_terrainProgress = (++done) * 60 / total;
        }
}

int g_autosave = 18000;

bool g_saveShowProgress = true;

static bool fileExists(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f) { fclose(f); return true; }
    return false;
}

static CompoundTag* readDatFile(const char* absDir, const char* name, int hdr) {
    std::string base = join(absDir, name);
    for (int attempt = 0; attempt < 2; attempt++) {
        FILE* f = fopen(attempt ? (base + "_old").c_str() : base.c_str(), "rb");
        if (!f) continue;
        CompoundTag* tag = NULL;
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char head[12];
        int size = 0;
        if (fsize > hdr && (int)fread(head, 1, hdr, f) == hdr) {
            memcpy(&size, head + hdr - 4, 4);
            int version = 0;
            memcpy(&version, head + hdr - 8, 4);
            bool headerOk = (hdr == 12) ? memcmp(head, "ENT", 3) == 0 : version >= 2;
            if (headerOk && size > 0 && size <= fsize - hdr) {
                unsigned char* buf = (unsigned char*)malloc(size);
                if (buf && (int)fread(buf, 1, size, f) == size) {
                    MemReader mr(buf, size);
                    tag = NbtIo::read(&mr);
                    if (tag && mr.failed()) { tag->deleteChildren(); delete tag; tag = NULL; }
                }
                free(buf);
            }
        }
        fclose(f);
        if (tag) return tag;
    }
    return NULL;
}

static bool writeDatFile(const char* absDir, const char* name,
                         const unsigned char* head, int hdr, const MemWriter& mw) {
    std::string dat = join(absDir, name);
    std::string tmp = dat + "_new";
    std::string old = dat + "_old";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    int size = (int)mw.buf.size();
    bool ok = (int)fwrite(head, 1, hdr, f) == hdr;
    if (ok && size > 0) ok = (int)fwrite(&mw.buf[0], 1, size, f) == size;
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(tmp.c_str()); return false; }
    remove(old.c_str());
    if (fileExists(dat)) rename(dat.c_str(), old.c_str());
    return rename(tmp.c_str(), dat.c_str()) == 0;
}

static CompoundTag* buildPlayerTag(World* w) {
    (void)w;
    CompoundTag* p = new CompoundTag();

    float px = g_level.player->x, py = g_level.player->bb.y0 + PLAYER_EYE, pz = g_level.player->z;
    if (!(px == px) || !(py == py) || !(pz == pz) || py < 0.0f) {
        int fx, fz, feetY; worldFindSpawn(w, &fx, &fz, &feetY);
        px = fx + 0.5f; py = feetY + PLAYER_EYE; pz = fz + 0.5f;
    }
    p->put("Pos",      floatList(px, py, pz));
    p->put("Motion",   floatList(0.0f, 0.0f, 0.0f));
    p->put("Rotation", floatList(g_level.player->yRot, g_level.player->xRot));
    p->putShort("Health", (short)g_level.player->health);

    p->putBoolean("Sleeping", g_level.player->sleeping);
    p->putShort("SleepTimer", g_level.player->sleepCounter);
    p->putInt("BedPositionX", g_level.player->bedX);
    p->putInt("BedPositionY", g_level.player->bedY);
    p->putInt("BedPositionZ", g_level.player->bedZ);

    p->putInt("SpawnX", g_level.player->respawnX);
    p->putInt("SpawnY", g_level.player->respawnY);
    p->putInt("SpawnZ", g_level.player->respawnZ);

    ListTag* inv = new ListTag();
    if (g_level.player->inventory->isCreative()) {
        for (int i = 0; i < Inventory::HOTBAR; i++) {
            ItemInstance* it = g_level.player->inventory->getLinked(i);
            if (!it || it->isNull()) continue;
            CompoundTag* slot = new CompoundTag();
            slot->putByte("Slot", (char)i);
            slot->putShort("id", it->id);
            slot->putByte("Count", (char)it->count);
            slot->putShort("Damage", it->data);
            inv->add(slot);
        }
    } else {

        const int LINKS = 9;
        for (int i = 0; i < LINKS; i++) {
            int link = (i < Inventory::HOTBAR) ? g_level.player->inventory->linkedSlots[i].inventorySlot : -1;
            CompoundTag* slot = new CompoundTag();
            slot->putByte("Slot", (char)i);
            slot->putShort("id", 255);
            slot->putByte("Count", (char)255);
            slot->putShort("Damage", (short)(link < 0 ? -1 : link - Inventory::HOTBAR + LINKS));
            inv->add(slot);
        }
        for (int s = 0; s < Inventory::SURVIVAL_SLOTS; s++) {
            ItemInstance* it = g_level.player->inventory->gridItem(s);
            if (!it || it->isNull()) continue;
            CompoundTag* slot = new CompoundTag();
            slot->putByte("Slot", (char)(s + LINKS));
            slot->putShort("id", it->id);
            slot->putByte("Count", (char)it->count);
            slot->putShort("Damage", it->data);
            inv->add(slot);
        }
    }
    p->put("Inventory", inv);

    {
        ListTag* ar = new ListTag();
        for (int i = 0; i < Player::NUM_ARMOR; i++) {
            ItemInstance& it = g_level.player->armor[i];
            CompoundTag* slot = new CompoundTag();
            slot->putShort("id", it.id);
            slot->putByte("Count", (char)it.count);
            slot->putShort("Damage", it.data);
            ar->add(slot);
        }
        p->put("Armor", ar);
    }
    return p;
}

static bool saveLevelDat(World* w, const char* absDir, long seed, int gameType, const char* levelName) {
    CompoundTag root;
    root.putLong("RandomSeed", (long long)seed);
    root.putInt("GameType", gameType);

    root.putInt("SpawnX", g_level.spawnX);
    root.putInt("SpawnY", g_level.spawnY);
    root.putInt("SpawnZ", g_level.spawnZ);
    root.putLong("Time", (long long)w->dayTime);
    root.putLong("SizeOnDisk", 0);
    root.putLong("LastPlayed", 0);
    root.putString("LevelName", levelName ? levelName : "World");
    root.putInt("StorageVersion", STORAGE_VERSION);
    root.putInt("Platform", 2);
    root.putCompound("Player", buildPlayerTag(w));

    MemWriter mw;
    NbtIo::write(&root, &mw);
    root.deleteChildren();

    unsigned char head[8];
    int version = STORAGE_VERSION, size = (int)mw.buf.size();
    memcpy(head, &version, 4);
    memcpy(head + 4, &size, 4);
    return writeDatFile(absDir, "level.dat", head, 8, mw);
}

struct SavedSlot { short id; short damage; short count; bool used; };
static SavedSlot g_loadedHotbar[Inventory::HOTBAR];
static SavedSlot g_loadedStorage[Inventory::SURVIVAL_SLOTS];
static int       g_loadedLinks[Inventory::HOTBAR];
static bool      g_loadedSurvival = false;

static bool      g_loadedPlayerPos = false;

static void clearLoadedHotbar() {
    for (int i = 0; i < Inventory::HOTBAR; i++) { g_loadedHotbar[i].used = false; g_loadedLinks[i] = -1; }
    for (int i = 0; i < Inventory::SURVIVAL_SLOTS; i++) g_loadedStorage[i].used = false;
    g_loadedSurvival = false;
    g_loadedPlayerPos = false;
}

static void loadLevelDat(World* w, const char* absDir, long* outSeed, int* outGameType) {
    CompoundTag* tag = readDatFile(absDir, "level.dat", 8);
    if (tag) {
        if (outSeed)     *outSeed = (long)tag->getLong("RandomSeed");
        if (outGameType) *outGameType = tag->getInt("GameType");
        w->dayTime = (long)tag->getLong("Time");

        if (tag->contains("SpawnY")) {
            g_level.setSpawnPos(tag->getInt("SpawnX"),
                                tag->getInt("SpawnY"),
                                tag->getInt("SpawnZ"));
        }
        CompoundTag* p = tag->getCompound("Player");
        if (p) {
            ListTag* pos = p->getList("Pos");
            ListTag* rot = p->getList("Rotation");
            if (pos->size() >= 3) {
                float px = pos->getFloat(0), py = pos->getFloat(1), pz = pos->getFloat(2);

                if (px == px && py == py && pz == pz && py >= 0.0f) {
                    g_level.player->x = px; g_level.player->y = py; g_level.player->z = pz;
                    g_loadedPlayerPos = true;
                }
            }
            if (rot->size() >= 2) { g_level.player->yRot = rot->getFloat(0); g_level.player->xRot = rot->getFloat(1); }
            if (p->contains("Health")) g_level.player->health = p->getShort("Health");

            if (p->getBoolean("Sleeping")) {
                g_level.player->sleeping = true;
                g_level.player->sleepCounter = p->getShort("SleepTimer");
                g_level.player->bedX = p->getInt("BedPositionX");
                g_level.player->bedY = p->getInt("BedPositionY");
                g_level.player->bedZ = p->getInt("BedPositionZ");
            }

            if (p->contains("SpawnY"))
                g_level.player->setRespawnPosition(p->getInt("SpawnX"),
                                                   p->getInt("SpawnY"),
                                                   p->getInt("SpawnZ"));

            g_loadedSurvival = (outGameType && *outGameType != 1);
            ListTag* inv = p->getList("Inventory");

            const int LINKS = 9;
            for (int i = 0; i < inv->size(); i++) {
                Tag* st = inv->get(i);
                if (!st || st->getId() != Tag::TAG_Compound) continue;
                CompoundTag* slot = (CompoundTag*)st;
                int si = (unsigned char)slot->getByte("Slot");
                if (g_loadedSurvival) {
                    short id  = slot->getShort("id");
                    int   cnt = (unsigned char)slot->getByte("Count");
                    if (si < LINKS) {
                        if (id == 255 && cnt == 255 && si < Inventory::HOTBAR) {
                            int link = (short)slot->getShort("Damage");
                            g_loadedLinks[si] = (link < 0) ? -1 : link - LINKS;
                        }
                        continue;
                    }
                    si -= LINKS;
                    if (si < 0 || si >= Inventory::SURVIVAL_SLOTS) continue;
                    g_loadedStorage[si].id     = id;
                    g_loadedStorage[si].damage = slot->getShort("Damage");
                    g_loadedStorage[si].count  = cnt;
                    g_loadedStorage[si].used   = true;
                } else {
                    if (si < 0 || si >= Inventory::HOTBAR) continue;
                    g_loadedHotbar[si].id     = slot->getShort("id");
                    g_loadedHotbar[si].damage = slot->getShort("Damage");

                    g_loadedHotbar[si].count  = (unsigned char)slot->getByte("Count");
                    g_loadedHotbar[si].used   = true;
                }
            }

            if (p->contains("Armor")) {
                ListTag* ar = p->getList("Armor");
                int na = ar->size(); if (na > Player::NUM_ARMOR) na = Player::NUM_ARMOR;
                for (int i = 0; i < na; i++) {
                    Tag* st = ar->get(i);
                    if (!st || st->getId() != Tag::TAG_Compound) continue;
                    CompoundTag* slot = (CompoundTag*)st;
                    g_level.player->armor[i] = ItemInstance(
                        slot->getShort("id"),
                        (short)(unsigned char)slot->getByte("Count"),
                        slot->getShort("Damage"));
                }
            }
        }
        tag->deleteChildren();
        delete tag;
    }
}

static const char* tileEntityName(int type) {
    if (type == TE_SIGN)    return "Sign";
    if (type == TE_CHEST)   return "Chest";
    if (type == TE_FURNACE) return "Furnace";
    if (type == TE_REACTOR) return "NetherReactor";
    return "";
}

static void saveEntities(World* w, const char* absDir) {
    ListTag* ents = new ListTag();
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        CompoundTag* t = new CompoundTag();
        if (g_level.entities[i]->save(t)) ents->add(t);
        else delete t;
    }
    ListTag* tes = new ListTag();
    for (size_t i = 0; i < g_level.tileEntities.size(); i++) {
        TileEntity* te = g_level.tileEntities[i];
        if (te->removed || !te->shouldSave()) continue;
        CompoundTag* t = new CompoundTag();
        if (te->save(t)) {
            t->putString("id", tileEntityName(te->type));
            tes->add(t);
        } else delete t;
    }

    for (size_t i = 0; i < w->preservedTileEntities.size(); i++) {
        std::vector<unsigned char>& blob = w->preservedTileEntities[i];
        if (blob.empty()) continue;
        MemReader mr(&blob[0], (int)blob.size());
        if (CompoundTag* c = NbtIo::read(&mr)) tes->add(c);
    }
    CompoundTag root;
    root.put("Entities", ents);
    root.put("TileEntities", tes);

    MemWriter mw;
    NbtIo::write(&root, &mw);
    root.deleteChildren();

    unsigned char head[12];
    int version = 1, numBytes = (int)mw.buf.size();
    memcpy(head, "ENT\0", 4);
    memcpy(head + 4, &version, 4);
    memcpy(head + 8, &numBytes, 4);
    writeDatFile(absDir, "entities.dat", head, 12, mw);
}

static TileEntity* createTileEntityByName(const std::string& id) {
    if (id == "Sign")    return new SignTileEntity();
    if (id == "Chest")   return new ChestTileEntity();
    if (id == "Furnace") return new FurnaceTileEntity();
    if (id == "NetherReactor") return new ReactorTileEntity();
    return NULL;
}

static void loadEntities(World* w, const char* absDir) {
    CompoundTag* root = readDatFile(absDir, "entities.dat", 12);
    if (root) {
        if (root->contains("Entities", Tag::TAG_List)) {
            ListTag* list = root->getList("Entities");
            for (int i = 0; i < list->size(); i++) {
                Tag* et = list->get(i);
                if (!et || et->getId() != Tag::TAG_Compound) continue;
                if (Entity* e = EntityFactory::loadEntity((CompoundTag*)et, &g_level))
                    g_level.addEntity(e);
            }
        }
        if (root->contains("TileEntities", Tag::TAG_List)) {
            ListTag* list = root->getList("TileEntities");
            for (int i = 0; i < list->size(); i++) {
                Tag* et = list->get(i);
                if (!et || et->getId() != Tag::TAG_Compound) continue;
                CompoundTag* c = (CompoundTag*)et;
                TileEntity* te = createTileEntityByName(c->getString("id"));
                if (!te) {

                    MemWriter mw;
                    NbtIo::write(c, &mw);
                    if (!mw.buf.empty()) w->preservedTileEntities.push_back(mw.buf);
                    continue;
                }
                te->level = &g_level;
                te->load(c);
                g_level.setTileEntity(te->x, te->y, te->z, te);
            }
        }
        root->deleteChildren();
        delete root;
    }
}

extern int g_lowMemHeap;

namespace LevelStorage {

bool hasSave(const char* absDir) {
    return chunkStorageHasSave(absDir);
}

bool save(World* w, const char* absDir, long seed, int gameType, const char* levelName,
          bool fullSave) {

    if (g_lowMemHeap) {
        for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
            chunkFreeMesh(&w->chunks[i]);
            for (int si = 0; si < N_SECTIONS; si++) w->chunks[i].sec[si].dirty = true;
        }
    }
    chunkStorageInit(absDir);
    saveChunks(w, !fullSave);
    bool ok = saveLevelDat(w, absDir, seed, gameType, levelName);
    saveEntities(w, absDir);
    return ok;
}

bool loadedValidPlayerPos() { return g_loadedPlayerPos; }

void applyLoadedHotbar() {
    if (!g_level.player->inventory->isCreative()) {

        for (int s = 0; s < Inventory::SURVIVAL_SLOTS; s++) {

            if (!g_loadedStorage[s].used ||
                g_loadedStorage[s].count <= 0 || g_loadedStorage[s].id == 0) continue;

            ItemInstance stack(g_loadedStorage[s].id,
                               g_loadedStorage[s].count,
                               g_loadedStorage[s].damage);
            g_level.player->inventory->setItem(
                s + g_level.player->inventory->firstGridSlot(), &stack);
        }
        for (int i = 0; i < Inventory::HOTBAR; i++)
            if (g_loadedLinks[i] >= 0) g_level.player->inventory->linkSlot(i, g_loadedLinks[i] + g_level.player->inventory->firstGridSlot());
        return;
    }

    for (int i = 0; i < Inventory::HOTBAR; i++) {
        if (!g_loadedHotbar[i].used) continue;
        if (g_loadedHotbar[i].id == 0) continue;
        int slot = i + g_level.player->inventory->firstGridSlot();
        int n = g_loadedHotbar[i].count;
        if (n <= 0) n = 1;
        ItemInstance stack(g_loadedHotbar[i].id, (short)n, g_loadedHotbar[i].damage);
        g_level.player->inventory->setItem(slot, &stack);
        g_level.player->inventory->linkSlot(i, slot);
    }
}

bool load(World* w, const char* absDir, long* outSeed, int* outGameType) {
    if (!hasSave(absDir)) return false;
    if (!worldAllocArrays(w)) return false;
    clearLoadedHotbar();
    g_level.removeAllEntities();
    g_level.removeAllTileEntities();

    loadLevelDat(w, absDir, outSeed, outGameType);

    if (levelSourceFor(s_activeWorldType).supportsGenFeatures())
        worldGenInit(outSeed ? *outSeed : 0, s_activeGenMask);

    chunkStorageInit(absDir);

    loadChunks(w, Mth::floor(g_level.player->x) >> 4, Mth::floor(g_level.player->z) >> 4);
    g_terrainProgress = 60;

    worldScheduleLoadedTicks(w);

    lightCompactAll(w);
    g_terrainProgress = 100;
    w->lightReady = true;

    worldUpdateSkyDarken(w);
    loadEntities(w, absDir);
    return true;
}

bool readInfo(const char* absDir, char* nameOut, int nameCap, int* outGameType, long* outSeed) {
    bool ok = false;
    CompoundTag* tag = readDatFile(absDir, "level.dat", 8);
    if (tag) {
        if (nameOut && nameCap > 0) {
            std::string nm = tag->getString("LevelName");
            strncpy(nameOut, nm.c_str(), nameCap - 1);
            nameOut[nameCap - 1] = '\0';
        }
        if (outGameType) *outGameType = tag->getInt("GameType");
        if (outSeed)     *outSeed = (long)tag->getLong("RandomSeed");
        ok = true;
        tag->deleteChildren();
        delete tag;
    }
    return ok;
}

void setActiveWorld(const char* absDir, long seed, int gameType, const char* levelName,
                    int worldType, int genMask) {
    if (absDir) strncpy(s_activeDir, absDir, sizeof(s_activeDir) - 1);
    s_activeDir[sizeof(s_activeDir) - 1] = '\0';
    if (levelName) strncpy(s_activeName, levelName, sizeof(s_activeName) - 1);
    s_activeName[sizeof(s_activeName) - 1] = '\0';
    s_activeSeed = seed;
    s_activeGameType = gameType;
    s_activeWorldType = worldType;
    s_activeGenMask = genMask;
}

const char* getActiveDir() { return s_activeDir; }
long getActiveSeed() { return s_activeSeed; }
int getActiveGameType() { return s_activeGameType; }
int getActiveWorldType() { return s_activeWorldType; }
int getActiveGenMask() { return s_activeGenMask; }
const char* getActiveName() { return s_activeName; }

}
