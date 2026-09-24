#include "world/entity/player.h"
#include "world/entity/local_player.h"
#include "world/entity/animal/sheep.h"
#include "world/entity/entity_types.h"
#include "world/entity/item_entity.h"
#include "world/item/item.h"
#include "world/item/item_instance.h"
#include "world/inventory/inventory.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "nbt/compound_tag.h"
#include "client/gamemode/gamemode.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/panic_goal.h"
#include "world/entity/ai/goals/breed_goal.h"
#include "world/entity/ai/goals/tempt_goal.h"
#include "world/entity/ai/goals/follow_parent_goal.h"
#include "world/entity/ai/goals/eat_tile_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"
#include <math.h>
#include "world/level/pathfinder/path_navigation.h"

extern World g_world;

static const int SHEEP_FOODS[] = { ITEM_WHEAT };

enum { W_WHITE = 0, W_ORANGE = 1, W_MAGENTA = 2, W_LIGHT_BLUE = 3, W_YELLOW = 4,
       W_LIME = 5, W_PINK = 6, W_GRAY = 7, W_SILVER = 8, W_CYAN = 9,
       W_PURPLE = 10, W_BLUE = 11, W_BROWN = 12, W_GREEN = 13, W_RED = 14,
       W_BLACK = 15 };

struct WoolMix { unsigned char a, b, out; };
static const WoolMix WOOL_MIXES[] = {
    { W_RED,    W_YELLOW, W_ORANGE     },
    { W_RED,    W_WHITE,  W_PINK       },
    { W_GREEN,  W_WHITE,  W_LIME       },
    { W_BLUE,   W_WHITE,  W_LIGHT_BLUE },
    { W_BLUE,   W_GREEN,  W_CYAN       },
    { W_BLUE,   W_RED,    W_PURPLE     },
    { W_BLACK,  W_WHITE,  W_GRAY       },
    { W_GRAY,   W_WHITE,  W_SILVER     },
    { W_PURPLE, W_PINK,   W_MAGENTA    },
    { W_BLACK,  W_ORANGE, W_BROWN      },
};

static int mixWoolColors(int a, int b) {
    for (unsigned int i = 0; i < sizeof(WOOL_MIXES) / sizeof(WOOL_MIXES[0]); i++) {
        const WoolMix& m = WOOL_MIXES[i];
        if ((m.a == a && m.b == b) || (m.a == b && m.b == a)) return m.out;
    }
    return -1;
}

static const float MTH_PI = 3.14159265f;

Sheep::Sheep(Level* level) : Animal(level), woolColor(0), sheared(false) {
    setSize(0.9f, 1.3f);
    heightOffset = 0.0f;
    runSpeed = 0.25f;

    getNavigation()->setAvoidWater(true);
    entityRendererId = ER_SHEEP_RENDERER;
    health = getMaxHealth();
    woolColor = getSheepColor(sharedRandom);

    eatTileGoal = new EatTileGoal(this);
    goalSelector.addGoal(0, new FloatGoal(this));

    goalSelector.addGoal(1, new PanicGoal(this, 0.38f));
    goalSelector.addGoal(2, new BreedGoal(this, 0.23f));
    goalSelector.addGoal(3, new TemptGoal(this, 0.25f, SHEEP_FOODS, 1));
    goalSelector.addGoal(4, new FollowParentGoal(this, 0.25f));
    goalSelector.addGoal(5, eatTileGoal, false);
    goalSelector.addGoal(6, new RandomStrollGoal(this, 0.23f));
    goalSelector.addGoal(7, new LookAtPlayerGoal(this, 6.0f));
    goalSelector.addGoal(8, new RandomLookAroundGoal(this));
}

Sheep::~Sheep() { delete eatTileGoal; }

bool Sheep::isFood(ItemInstance* item) { return item && item->id == ITEM_WHEAT; }

Animal* Sheep::getBreedOffspring(Animal* partner) {
    Sheep* lamb = new Sheep(level);
    int other = ((Sheep*)partner)->getColor();
    int mixed = mixWoolColors(woolColor, other);

    lamb->setColor(mixed >= 0 ? mixed
                              : (sharedRandom.nextInt(2) ? woolColor : other));
    return lamb;
}

void Sheep::serverAiMobStep() { eatAnimationTick = eatTileGoal->getEatAnimationTick(); }

void Sheep::ate() {
    setSheared(false);
    if (isBaby()) {
        int na = getAge() + TicksPerSecond * 60;
        setAge(na > 0 ? 0 : na);
    }
}

int Sheep::getEntityTypeId() const { return EntityTypes::IdSheep; }

int Sheep::getSheepColor(Random& random) {
    int r = random.nextInt(100);
    if (r < 5)  return 15;
    if (r < 10) return 7;
    if (r < 15) return 8;
    if (r < 18) return 12;
    if (random.nextInt(500) == 0) return 6;
    return 0;
}

static void dropWool(Level* level, float x, float y, float z, int color) {
    ItemEntity* ie = new ItemEntity(level, x, y, z, ItemInstance(BLOCK_WOOL, 1, (short)color));
    level->addEntity(ie);
}

void Sheep::dropDeathLoot() {
    if (!sheared) dropWool(level, x, y, z, woolColor);
}

bool Sheep::playerInteract() {
    ItemInstance* sel = g_level.player->inventory->getSelected();
    if (!sel || sel->isNull()) return false;

    if (sel->id == ITEM_BONEMEAL && !isSheared()) {
        int newColor = 15 - sel->data;
        if (!level->isClientSide && getColor() != newColor) {
            setColor(newColor);

            g_level.player->inventory->consumeSelected();
        }
        return true;
    }

    if (sel->id != ITEM_SHEARS) return Animal::playerInteract();
    if (sheared || isBaby()) return Animal::playerInteract();
    if (!level->isClientSide) {
        sheared = true;
        int count = 1 + sharedRandom.nextInt(3);
        for (int i = 0; i < count; i++) dropWool(level, x, y, z, woolColor);
    }

    g_level.player->inventory->hurtSelected(1);
    return true;

}

void Sheep::addAdditonalSaveData(CompoundTag* tag) {
    Animal::addAdditonalSaveData(tag);
    tag->putBoolean("Sheared", sheared);
    tag->putByte("Color", (char)woolColor);
}

void Sheep::readAdditionalSaveData(CompoundTag* tag) {
    Animal::readAdditionalSaveData(tag);
    sheared = tag->getBoolean("Sheared");
    woolColor = tag->getByte("Color") & 0x0f;
}

float Sheep::getHeadEatPositionScale(float a) const {
    if (eatAnimationTick <= 0) return 0.0f;
    if (eatAnimationTick >= 4 && eatAnimationTick <= EAT_ANIMATION_TICKS - 4) return 1.0f;
    if (eatAnimationTick < 4) return ((float)eatAnimationTick - a) / 4.0f;
    return -((float)(eatAnimationTick - EAT_ANIMATION_TICKS) - a) / 4.0f;
}

float Sheep::getHeadEatAngleScale(float a) const {
    if (eatAnimationTick > 4 && eatAnimationTick <= EAT_ANIMATION_TICKS - 4) {
        float s = ((float)(eatAnimationTick - 4) - a) / (float)(EAT_ANIMATION_TICKS - 8);
        return MTH_PI * 0.20f + MTH_PI * 0.07f * sinf(s * 28.7f);
    }
    if (eatAnimationTick > 0) return MTH_PI * 0.20f;
    return 0.0f;
}
