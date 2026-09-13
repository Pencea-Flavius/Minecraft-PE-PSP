#ifndef MCPSP_WORLD_ITEM_H
#define MCPSP_WORLD_ITEM_H

#include "world/level/chunk/chunk.h"
#include "world/item/item_instance.h"

enum {

    ITEM_SHOVEL_IRON    = 256,
    ITEM_PICKAXE_IRON   = 257,
    ITEM_HATCHET_IRON   = 258,
    ITEM_FLINT_AND_STEEL= 259,
    ITEM_APPLE          = 260,
    ITEM_BOW            = 261,
    ITEM_ARROW          = 262,
    ITEM_COAL           = 263,
    ITEM_DIAMOND        = 264,
    ITEM_IRON_INGOT     = 265,
    ITEM_GOLD_INGOT     = 266,
    ITEM_SWORD_IRON     = 267,
    ITEM_SWORD_WOOD     = 268,
    ITEM_SHOVEL_WOOD    = 269,
    ITEM_PICKAXE_WOOD   = 270,
    ITEM_HATCHET_WOOD   = 271,
    ITEM_SWORD_STONE    = 272,
    ITEM_SHOVEL_STONE   = 273,
    ITEM_PICKAXE_STONE  = 274,
    ITEM_HATCHET_STONE  = 275,
    ITEM_SWORD_DIAMOND  = 276,
    ITEM_SHOVEL_DIAMOND = 277,
    ITEM_PICKAXE_DIAMOND= 278,
    ITEM_HATCHET_DIAMOND= 279,
    ITEM_STICK          = 280,
    ITEM_BOWL           = 281,
    ITEM_MUSHROOM_STEW  = 282,
    ITEM_SWORD_GOLD     = 283,
    ITEM_SHOVEL_GOLD    = 284,
    ITEM_PICKAXE_GOLD   = 285,
    ITEM_HATCHET_GOLD   = 286,
    ITEM_STRING         = 287,
    ITEM_FEATHER        = 288,
    ITEM_GUNPOWDER      = 289,
    ITEM_HOE_WOOD       = 290,
    ITEM_HOE_STONE      = 291,
    ITEM_HOE_IRON       = 292,
    ITEM_HOE_DIAMOND    = 293,
    ITEM_HOE_GOLD       = 294,
    ITEM_SEEDS_WHEAT    = 295,
    ITEM_WHEAT          = 296,
    ITEM_BREAD          = 297,

    ITEM_HELMET_CLOTH   = 298, ITEM_CHESTPLATE_CLOTH   = 299, ITEM_LEGGINGS_CLOTH   = 300, ITEM_BOOTS_CLOTH   = 301,
    ITEM_HELMET_CHAIN   = 302, ITEM_CHESTPLATE_CHAIN   = 303, ITEM_LEGGINGS_CHAIN   = 304, ITEM_BOOTS_CHAIN   = 305,
    ITEM_HELMET_IRON    = 306, ITEM_CHESTPLATE_IRON    = 307, ITEM_LEGGINGS_IRON    = 308, ITEM_BOOTS_IRON    = 309,
    ITEM_HELMET_DIAMOND = 310, ITEM_CHESTPLATE_DIAMOND = 311, ITEM_LEGGINGS_DIAMOND = 312, ITEM_BOOTS_DIAMOND = 313,
    ITEM_HELMET_GOLD    = 314, ITEM_CHESTPLATE_GOLD    = 315, ITEM_LEGGINGS_GOLD    = 316, ITEM_BOOTS_GOLD    = 317,

    ITEM_FLINT          = 318,
    ITEM_PORKCHOP_RAW   = 319,
    ITEM_PORKCHOP_COOKED= 320,
    ITEM_PAINTING       = 321,
    ITEM_SIGN           = 323,
    ITEM_DOOR_WOOD_ITEM = 324,
    ITEM_DOOR_IRON_ITEM = 330,
    ITEM_SNOWBALL       = 332,
    ITEM_LEATHER        = 334,
    ITEM_BRICK          = 336,
    ITEM_CLAY           = 337,
    ITEM_REEDS          = 338,
    ITEM_PAPER          = 339,
    ITEM_BOOK           = 340,
    ITEM_SLIMEBALL      = 341,
    ITEM_EGG            = 344,
    ITEM_SPAWN_EGG      = 383,
    ITEM_MINECART       = 328,
    ITEM_COMPASS        = 345,
    ITEM_BUCKET         = 325,
    ITEM_CLOCK          = 347,
    ITEM_REDSTONE       = 331,
    ITEM_GLOWSTONE_DUST = 348,
    ITEM_BONEMEAL       = 351,
    ITEM_BONE           = 352,
    ITEM_SUGAR          = 353,
    ITEM_CAKE           = 354,
    ITEM_BED_ITEM       = 355,
    ITEM_SHEARS         = 359,
    ITEM_MELON          = 360,
    ITEM_SEEDS_PUMPKIN  = 361,
    ITEM_SEEDS_MELON    = 362,
    ITEM_BEEF_RAW       = 363,
    ITEM_BEEF_COOKED    = 364,
    ITEM_CHICKEN_RAW    = 365,
    ITEM_CHICKEN_COOKED = 366,

    ITEM_CARROT         = 391,
    ITEM_POTATO         = 392,
    ITEM_POTATO_BAKED   = 393,
    ITEM_PUMPKIN_PIE    = 400,
    ITEM_NETHER_BRICK   = 405,
    ITEM_NETHER_QUARTZ  = 406,
    ITEM_CAMERA         = 456,

    ITEM_BEETROOT       = 457,
    ITEM_SEEDS_BEETROOT = 458,
    ITEM_BEETROOT_SOUP  = 459
};

enum { DYE_WHITE = 15 };

class ItemCategory {
public:
    static const int Structures  = 1;
    static const int Tools       = 2;
    static const int FoodArmor   = 4;
    static const int Decorations = 8;
    static const int Mechanisms  = 16;
};

struct World;
class Player;
class Mob;
class Entity;

class Item {
public:

    class Tier {
    public:
        const int   level;
        const int   uses;
        const float speed;
        const int   damage;
        Tier(int level, int uses, float speed, int damage)
            : level(level), uses(uses), speed(speed), damage(damage) {}
        int   getUses() const { return uses; }
        float getSpeed() const { return speed; }
        int   getAttackDamageBonus() const { return damage; }
        int   getLevel() const { return level; }
        static const Tier WOOD, STONE, IRON, EMERALD, GOLD;
    };

    const short id;
    int   maxStackSize;
    short maxDamage;
    int   category;

    int   creativeTab;

    Item(short id);
    virtual ~Item() {}

    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ) {
        return false;
    }
    virtual void use(ItemInstance* item, Player* player, World* world) {}

    virtual int  getMaxUseDuration() const { return 0; }

    virtual void useTimeDepleted(ItemInstance* item, Player* player) {}

    virtual int  getUseAnimation() const { return 0; }

    virtual void releaseUsing(ItemInstance* item, Player* player, int duration) {}

    virtual int getMaxStackSize(const ItemInstance* item) const { return maxStackSize; }

    virtual bool isLiquidClipItem(short data) const { return false; }

    virtual bool placesTile() const { return false; }

    virtual short placedTileId() const { return 0; }

    virtual short plantedTileId() const { return 0; }

    virtual bool isHoe() const { return false; }

    virtual bool mineBlock(ItemInstance* item, World* world, int blockId, int x, int y, int z, Player* player) {
        return false;
    }
    virtual void hurtEnemy(ItemInstance* item, Mob* target, Player* attacker) {}

    virtual bool isTool() const { return maxDamage > 0; }
    virtual bool isHandEquipped() const { return false; }

    virtual float getDestroySpeed(int blockId) const { return 1.0f; }

    virtual int   getAttackDamage(Entity* target) const { return 1; }
    virtual bool  canDestroySpecial(int blockId) const { return false; }
    virtual bool  isFood() const { return false; }

    virtual bool  isSeed() const { return false; }
    virtual bool  isArmor() const { return false; }

    virtual int getIcon(short data) const { return -1; }

    virtual int iconFrameCount() const { return 0; }
    virtual int getAnimationFrameFor(const Player*) const { return 0; }

    static Item* items[4096];
    static void initItems();
};

#endif
