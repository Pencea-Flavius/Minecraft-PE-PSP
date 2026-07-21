#include "world/level/storage/level_storage.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/storage/region_file.h"

#include "world/level/world.h"
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
#define LOGI printf
#include <cstring>
#include <string>

static const int CH_BLOCKS = 16 * 16 * 128;
static const int CH_NIBBLE = CH_BLOCKS / 2;
static const int CH_COLS   = 256;
static const int CH_PAYLOAD = CH_BLOCKS + CH_NIBBLE * 3 + CH_COLS;

static const int OFF_DATA = CH_BLOCKS;
static const int OFF_SKY  = OFF_DATA + CH_NIBBLE;
static const int OFF_BLK  = OFF_SKY  + CH_NIBBLE;
static const int OFF_UPD  = OFF_BLK  + CH_NIBBLE;

static const int STORAGE_VERSION = 3;

static inline int chunkIdx(int lx, int lz, int y) { return (lx << 11) | (lz << 7) | y; }
static inline void nibSet(unsigned char* base, int idx, int v) {
    unsigned char& b = base[idx >> 1];
    if (idx & 1) b = (b & 0x0F) | ((v & 0x0F) << 4);
    else         b = (b & 0xF0) | (v & 0x0F);
}
static inline int nibGet(const unsigned char* base, int idx) {
    unsigned char b = base[idx >> 1];
    return (idx & 1) ? (b >> 4) & 0x0F : b & 0x0F;
}

static std::string join(const char* dir, const char* name) {
    std::string s = dir; s += "/"; s += name; return s;
}

static void saveOneChunk(World* w, RegionFile& rf, unsigned char* payload, int cx, int cz);
extern bool g_saveShowProgress;

static void saveChunks(World* w, const char* absDir, bool onlyDirty) {
    RegionFile rf(absDir);
    if (!rf.open()) { LOGI("LevelStorage: can't open chunks.dat for write\n"); return; }

    unsigned char* payload = new unsigned char[CH_PAYLOAD];
    for (int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
        for (int cx = 0; cx < WORLD_CHUNKS_X; cx++) {
            if (onlyDirty && !w->unsaved[cz * WORLD_CHUNKS_X + cx]) continue;
            saveOneChunk(w, rf, payload, cx, cz);
        }

        if (g_saveShowProgress) g_terrainProgress = (cz + 1) * 100 / WORLD_CHUNKS_Z;
    }
    delete[] payload;
}

static void saveOneChunk(World* w, RegionFile& rf, unsigned char* payload, int cx, int cz) {
    memset(payload, 0, CH_PAYLOAD);
    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            int gx = cx * 16 + lx, gz = cz * 16 + lz;
            int srcBase = worldIndex(gx, 0, gz);
            int dstBase = chunkIdx(lx, lz, 0);

            memcpy(payload + dstBase, w->blocks + srcBase, 128);
            for (int y = 0; y < 128; y++) {
                int idx = dstBase + y;
                unsigned char lv = w->light[srcBase + y];

                nibSet(payload + OFF_DATA, idx, nibGet(w->data, srcBase + y));
                nibSet(payload + OFF_SKY,  idx, lv >> 4);
                nibSet(payload + OFF_BLK,  idx, lv & 0x0F);
            }
        }
    }
    rf.writeChunk(cx, cz, payload, CH_PAYLOAD);
    w->unsaved[cz * WORLD_CHUNKS_X + cx] = false;
}

static bool loadChunks(World* w, const char* absDir, bool* outGotLight) {
    RegionFile rf(absDir);
    if (!rf.open()) return false;

    bool any = false;
    *outGotLight = true;
    for (int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
        for (int cx = 0; cx < WORLD_CHUNKS_X; cx++) {
            unsigned char* payload = NULL;
            int len = 0;
            if (!rf.readChunk(cx, cz, &payload, &len)) continue;
            if (len < OFF_DATA + CH_NIBBLE) { delete[] payload; continue; }
            any = true;

            bool chunkHasLight = (len >= OFF_UPD);
            if (!chunkHasLight) *outGotLight = false;
            for (int lx = 0; lx < 16; lx++) {
                for (int lz = 0; lz < 16; lz++) {
                    int gx = cx * 16 + lx, gz = cz * 16 + lz;
                    int srcBase = worldIndex(gx, 0, gz);
                    int dstBase = chunkIdx(lx, lz, 0);

                    memcpy(w->blocks + srcBase, payload + dstBase, 128);
                    for (int y = 0; y < 128; y++) {
                        nibSet(w->data, srcBase + y, nibGet(payload + OFF_DATA, dstBase + y));
                        if (chunkHasLight)
                            w->light[srcBase + y] = (unsigned char)
                                ((nibGet(payload + OFF_SKY, dstBase + y) << 4) |
                                  nibGet(payload + OFF_BLK, dstBase + y));
                    }
                }
            }
            delete[] payload;
        }
        g_terrainProgress = (cz + 1) * 60 / WORLD_CHUNKS_Z;
    }
    return any;
}

int g_autosave = 18000;

bool g_saveShowProgress = true;

static bool fileExists(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f) { fclose(f); return true; }
    return false;
}

static CompoundTag* buildPlayerTag(World* w) {
    (void)w;
    CompoundTag* p = new CompoundTag();

    float px = g_level.player->x, py = g_level.player->y, pz = g_level.player->z;
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

    ListTag* inv = new ListTag();
    if (g_inv.isCreative()) {
        for (int i = 0; i < Inventory::HOTBAR; i++) {
            ItemInstance* it = g_inv.getLinked(i);
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
            int link = (i < Inventory::HOTBAR) ? g_inv.linkedSlots[i].inventorySlot : -1;
            CompoundTag* slot = new CompoundTag();
            slot->putByte("Slot", (char)i);
            slot->putShort("id", 255);
            slot->putByte("Count", (char)255);
            slot->putShort("Damage", (short)(link < 0 ? -1 : link + LINKS));
            inv->add(slot);
        }
        for (int s = 0; s < Inventory::SURVIVAL_SLOTS; s++) {
            ItemInstance* it = g_inv.getItem(s);
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

    if (g_bedSpawnY >= 0) {
        root.putInt("SpawnX", g_bedSpawnX);
        root.putInt("SpawnY", g_bedSpawnY);
        root.putInt("SpawnZ", g_bedSpawnZ);
    } else {
        int cx = WORLD_W / 2, cz = WORLD_D / 2;
        root.putInt("SpawnX", cx);
        root.putInt("SpawnY", (int)w->heightmap[cx * WORLD_D + cz]);
        root.putInt("SpawnZ", cz);
    }
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

    std::string dat = join(absDir, "level.dat");
    std::string tmp = join(absDir, "level.dat_new");
    std::string old = join(absDir, "level.dat_old");

    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    int version = STORAGE_VERSION;
    int size = (int)mw.buf.size();
    fwrite(&version, sizeof(int), 1, f);
    fwrite(&size, sizeof(int), 1, f);
    if (size > 0) fwrite(&mw.buf[0], 1, size, f);
    fclose(f);

    remove(old.c_str());
    if (fileExists(dat)) rename(dat.c_str(), old.c_str());
    rename(tmp.c_str(), dat.c_str());
    return true;
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
    std::string dat = join(absDir, "level.dat");
    FILE* f = fopen(dat.c_str(), "rb");
    if (!f) { f = fopen(join(absDir, "level.dat_old").c_str(), "rb"); }
    if (!f) return;

    int version = 0, size = 0;
    if (fread(&version, sizeof(int), 1, f) == 1 &&
        fread(&size, sizeof(int), 1, f) == 1 && size > 0 && version >= 2) {
        unsigned char* buf = new unsigned char[size];
        if ((int)fread(buf, 1, size, f) == size) {
            MemReader mr(buf, size);
            CompoundTag* tag = NbtIo::read(&mr);
            if (tag) {
                if (outSeed)     *outSeed = (long)tag->getLong("RandomSeed");
                if (outGameType) *outGameType = tag->getInt("GameType");
                w->dayTime = (long)tag->getLong("Time");

                if (tag->contains("SpawnY")) {
                    g_bedSpawnX = tag->getInt("SpawnX");
                    g_bedSpawnY = tag->getInt("SpawnY");
                    g_bedSpawnZ = tag->getInt("SpawnZ");
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

                    g_loadedSurvival = (outGameType && *outGameType != 1);
                    ListTag* inv = p->getList("Inventory");

                    const int LINKS = 9;
                    bool oldFormat = g_loadedSurvival && p->contains("Hotbar");
                    for (int i = 0; i < inv->size(); i++) {
                        CompoundTag* slot = (CompoundTag*)inv->get(i);
                        if (!slot) continue;
                        int si = (unsigned char)slot->getByte("Slot");
                        if (g_loadedSurvival) {
                            short id  = slot->getShort("id");
                            int   cnt = (unsigned char)slot->getByte("Count");
                            if (!oldFormat) {
                                if (si < LINKS) {
                                    if (id == 255 && cnt == 255 && si < Inventory::HOTBAR) {
                                        int link = (short)slot->getShort("Damage");
                                        g_loadedLinks[si] = (link < 0) ? -1 : link - LINKS;
                                    }
                                    continue;
                                }
                                si -= LINKS;
                            }
                            if (si < 0 || si >= Inventory::SURVIVAL_SLOTS) continue;
                            g_loadedStorage[si].id     = id;
                            g_loadedStorage[si].damage = slot->getShort("Damage");
                            g_loadedStorage[si].count  = cnt;
                            g_loadedStorage[si].used   = true;
                        } else {
                            if (si < 0 || si >= Inventory::HOTBAR) continue;
                            g_loadedHotbar[si].id     = slot->getShort("id");
                            g_loadedHotbar[si].damage = slot->getShort("Damage");
                            g_loadedHotbar[si].used   = true;
                        }
                    }

                    if (p->contains("Armor")) {
                        ListTag* ar = p->getList("Armor");
                        int na = ar->size(); if (na > Player::NUM_ARMOR) na = Player::NUM_ARMOR;
                        for (int i = 0; i < na; i++) {
                            CompoundTag* slot = (CompoundTag*)ar->get(i);
                            if (!slot) continue;
                            g_level.player->armor[i] = ItemInstance(
                                slot->getShort("id"),
                                (short)(unsigned char)slot->getByte("Count"),
                                slot->getShort("Damage"));
                        }
                    }
                    if (g_loadedSurvival) {
                        ListTag* hb = p->getList("Hotbar");
                        for (int i = 0; i < hb->size(); i++) {
                            CompoundTag* l = (CompoundTag*)hb->get(i);
                            if (!l) continue;
                            int hi = (unsigned char)l->getByte("Slot");
                            if (hi < 0 || hi >= Inventory::HOTBAR) continue;
                            g_loadedLinks[hi] = l->getShort("Link");
                        }
                    }
                }
                tag->deleteChildren();
                delete tag;
            }
        }
        delete[] buf;
    }
    fclose(f);
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
        if (te->removed) continue;
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

    FILE* f = fopen(join(absDir, "entities.dat").c_str(), "wb");
    if (f) {
        int version = 1, numBytes = (int)mw.buf.size();
        fwrite("ENT\0", 1, 4, f);
        fwrite(&version, sizeof(int), 1, f);
        fwrite(&numBytes, sizeof(int), 1, f);
        if (numBytes > 0) fwrite(&mw.buf[0], 1, numBytes, f);
        fclose(f);
    }
}

static TileEntity* createTileEntityByName(const std::string& id) {
    if (id == "Sign")    return new SignTileEntity();
    if (id == "Chest")   return new ChestTileEntity();
    if (id == "Furnace") return new FurnaceTileEntity();
    if (id == "NetherReactor") return new ReactorTileEntity();
    return NULL;
}

static void loadEntities(World* w, const char* absDir) {
    FILE* f = fopen(join(absDir, "entities.dat").c_str(), "rb");
    if (!f) return;
    char header[4]; int version = 0, numBytes = 0;
    if (fread(header, 1, 4, f) == 4 &&
        fread(&version, sizeof(int), 1, f) == 1 &&
        fread(&numBytes, sizeof(int), 1, f) == 1 &&
        numBytes > 0 && memcmp(header, "ENT", 3) == 0) {
        unsigned char* buf = new unsigned char[numBytes];
        if ((int)fread(buf, 1, numBytes, f) == numBytes) {
            MemReader mr(buf, numBytes);
            CompoundTag* root = NbtIo::read(&mr);
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
        delete[] buf;
    }
    fclose(f);
}

namespace LevelStorage {

bool hasSave(const char* absDir) {
    return fileExists(join(absDir, "chunks.dat"));
}

bool save(World* w, const char* absDir, long seed, int gameType, const char* levelName,
          bool fullSave) {
    saveChunks(w, absDir, !fullSave);
    bool ok = saveLevelDat(w, absDir, seed, gameType, levelName);
    saveEntities(w, absDir);
    return ok;
}

bool loadedValidPlayerPos() { return g_loadedPlayerPos; }

void applyLoadedHotbar() {
    if (!g_inv.isCreative()) {

        for (int s = 0; s < Inventory::SURVIVAL_SLOTS; s++) {
            if (!g_loadedStorage[s].used) continue;
            g_inv.setItem(s, new ItemInstance(g_loadedStorage[s].id,
                                              g_loadedStorage[s].count,
                                              g_loadedStorage[s].damage));
        }
        for (int i = 0; i < Inventory::HOTBAR; i++)
            if (g_loadedLinks[i] >= 0) g_inv.linkSlot(i, g_loadedLinks[i]);
        return;
    }
    for (int i = 0; i < Inventory::HOTBAR; i++) {
        if (!g_loadedHotbar[i].used) continue;

        g_inv.linkHotbarTo(i, g_loadedHotbar[i].id, (unsigned char)g_loadedHotbar[i].damage);
    }
}

bool load(World* w, const char* absDir, long* outSeed, int* outGameType) {
    if (!hasSave(absDir)) return false;
    if (!worldAllocArrays(w)) return false;
    clearLoadedHotbar();
    bool gotLight = false;
    loadChunks(w, absDir, &gotLight);
    g_terrainProgress = 60;

    worldScheduleLoadedLiquids(w);

    if (gotLight) worldRecalcHeightmap(w);
    else          worldInitLight(w);
    g_terrainProgress = 100;
    w->lightReady = true;
    g_level.removeAllEntities();
    g_level.removeAllTileEntities();
    loadLevelDat(w, absDir, outSeed, outGameType);

    worldUpdateSkyDarken(w);
    loadEntities(w, absDir);
    return true;
}

bool readInfo(const char* absDir, char* nameOut, int nameCap, int* outGameType, long* outSeed) {
    std::string dat = join(absDir, "level.dat");
    FILE* f = fopen(dat.c_str(), "rb");
    if (!f) { f = fopen(join(absDir, "level.dat_old").c_str(), "rb"); }
    if (!f) return false;

    bool ok = false;
    int version = 0, size = 0;
    if (fread(&version, sizeof(int), 1, f) == 1 &&
        fread(&size, sizeof(int), 1, f) == 1 && size > 0 && version >= 2) {
        unsigned char* buf = new unsigned char[size];
        if ((int)fread(buf, 1, size, f) == size) {
            MemReader mr(buf, size);
            CompoundTag* tag = NbtIo::read(&mr);
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
        }
        delete[] buf;
    }
    fclose(f);
    return ok;
}

}
