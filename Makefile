TARGET = mcpsp-menu
OBJS = \
	src/main.o \
	src/client/player/physics.o \
	src/client/gamemode/gamemode.o \
	src/client/player/player.o \
	src/world/entity/mob.o \
	src/world/entity/player.o \
	src/world/entity/local_player.o \
	src/world/level/tile/tile.o \
	src/client/renderer/render.o \
	src/client/renderer/particle.o \
	src/client/renderer/water_anim.o \
	src/client/renderer/item_hand.o \
	src/client/renderer/item_model.o \
	src/client/gui/gen_screen.o \
	src/client/gui/hud.o \
	src/client/gui/inventory_ui.o \
	src/client/gui/screens/menu_common.o \
	src/client/gui/screens/screen_title.o \
	src/client/gui/screens/screen_worlds.o \
	src/client/gui/screens/screen_delete.o \
	src/client/gui/screens/screen_create.o \
	src/client/gui/screens/screen_join.o \
	src/client/gui/screens/screen_joinip.o \
	src/client/gui/screens/screen_options.o \
	src/client/gui/screens/screen_pause.o \
	src/client/gui/screens/screen_dead.o \
	src/client/gui/screens/screen_inbed.o \
	src/world/entity/tripod_camera.o \
	src/client/renderer/entity/tripod_camera_renderer.o \
	src/world/item/crafting/recipe.o \
	src/world/item/crafting/recipes.o \
	src/client/gui/screens/screen_craft.o \
	src/client/gui/screens/screen_armor.o \
	src/client/gui/screens/screen_furnace.o \
	src/client/gui/screens/screen_chest.o \
	src/world/level/tile/entity/furnace_tile_entity.o \
	src/world/level/tile/entity/reactor_tile_entity.o \
	src/world/level/tile/nether_reactor_pattern.o \
	src/client/gui/screens/screen_sign.o \
	src/gpu/gu.o \
	src/gpu/texture.o \
	src/gpu/sprite.o \
	src/gpu/font.o \
	src/gpu/widgets.o \
	src/platform/path.o \
	src/platform/png_loader.o \
	src/platform/malloc_lock.o \
	src/platform/audio/sound.o \
	src/world/level/storage/worldlist.o \
	src/world/level/storage/region_file.o \
	src/world/level/storage/level_storage.o \
	src/util/data_io.o \
	src/nbt/tag.o \
	src/world/level/chunk/chunk.o \
	src/world/level/chunk/chunk_build.o \
	src/world/level/chunk/chunk_draw.o \
	src/world/level/tile/tile_glass.o \
	src/world/level/tile/tile_wool.o \
	src/world/level/tile/tile_leaves.o \
	src/world/level/tile/tile_furnace_chest.o \
	src/world/level/tile/tile_quartz_sandstone.o \
	src/world/level/tile/tile_reactor.o \
	src/world/level/tile/tile_stonecutter_crafting.o \
	src/world/level/tile/entity/tile_entity.o \
	src/world/level/tile/entity/sign_tile_entity.o \
	src/world/level/tile/entity/chest_tile_entity.o \
	src/world/level/tile/entity/tile_entity_factory.o \
	src/world/item/item.o \
	src/world/item/item_instance.o \
	src/world/item/tile_item.o \
	src/world/item/hoe_item.o \
	src/world/item/seed_item.o \
	src/world/item/hanging_entity_item.o \
	src/world/item/sign_item.o \
	src/world/item/bonemeal_item.o \
	src/world/item/spawn_egg_item.o \
	src/world/item/armor_item.o \
	src/world/level/level.o \
	src/world/entity/entity.o \
	src/world/entity/hanging_entity.o \
	src/world/entity/motive.o \
	src/world/entity/painting.o \
	src/world/entity/arrow.o \
	src/world/entity/throwable.o \
	src/world/entity/falling_tile.o \
	src/world/entity/primed_tnt.o \
	src/world/entity/item_entity.o \
	src/world/entity/entity_factory.o \
	src/world/entity/path_finder_mob.o \
	src/world/entity/mob_factory.o \
	src/world/level/mob_spawner.o \
	src/world/entity/animal/animal.o \
	src/world/entity/animal/pig.o \
	src/world/entity/animal/cow.o \
	src/world/entity/animal/chicken.o \
	src/world/entity/animal/sheep.o \
	src/world/entity/monster/monster.o \
	src/world/entity/monster/zombie.o \
	src/world/entity/monster/skeleton.o \
	src/world/entity/monster/creeper.o \
	src/world/entity/monster/spider.o \
	src/world/entity/monster/pig_zombie.o \
	src/world/level/pathfinder/path.o \
	src/world/level/pathfinder/path_finder.o \
	src/client/renderer/entity/entity_render_dispatcher.o \
	src/client/renderer/entity/entity_renderer.o \
	src/client/renderer/entity/painting_renderer.o \
	src/client/renderer/entity/arrow_renderer.o \
	src/client/renderer/entity/falling_tile_renderer.o \
	src/client/renderer/entity/primed_tnt_renderer.o \
	src/client/renderer/entity/item_renderer.o \
	src/client/renderer/entity/throwable_renderer.o \
	src/client/renderer/entity/mob_model.o \
	src/client/renderer/entity/pig_renderer.o \
	src/client/renderer/entity/cow_renderer.o \
	src/client/renderer/entity/chicken_renderer.o \
	src/client/renderer/entity/sheep_renderer.o \
	src/client/renderer/entity/humanoid_renderer.o \
	src/client/renderer/entity/creeper_renderer.o \
	src/client/renderer/entity/spider_renderer.o \
	src/client/renderer/entity/player_model.o \
	src/client/renderer/tileentity/sign_renderer.o \
	src/world/inventory/filling_container.o \
	src/world/inventory/inventory.o \
	src/client/renderer/tile/mesh_block.o \
	src/client/renderer/tile/mesh_partial.o \
	src/client/renderer/tile/mesh_liquid.o \
	src/client/renderer/level/frustum.o \
	src/world/level/world.o \
	src/world/level/dirty.o \
	src/world/level/raycast.o \
	src/world/level/explosion.o \
	src/world/level/light.o \
	src/client/renderer/level/worldrender.o \
	src/world/level/liquid.o \
	src/world/level/leafdecay.o \
	src/world/level/tile/tile_behavior.o \
	src/world/level/tile/tile_bush.o \
	src/world/level/tile/tile_reed_cactus.o \
	src/world/level/tile/tile_farmland.o \
	src/world/level/tile/tile_support.o \
	src/world/level/tile/redstone_ore.o \
	src/world/level/tile/tile_drops.o \
	src/world/level/levelgen/Synth.o \
	src/world/level/levelgen/ImprovedNoise.o \
	src/world/level/levelgen/PerlinNoise.o \
	src/world/level/levelgen/biome.o \
	src/world/level/levelgen/mcpegen.o \
	src/world/level/levelgen/features_common.o \
	src/world/level/levelgen/feature_tree_oak.o \
	src/world/level/levelgen/feature_tree_birch.o \
	src/world/level/levelgen/feature_tree_spruce.o \
	src/world/level/levelgen/feature_tree_pine.o \
	src/world/level/levelgen/feature_clay.o \
	src/world/level/levelgen/feature_flower.o \
	src/world/level/levelgen/feature_mushroom.o \
	src/world/level/levelgen/feature_cactus.o \
	src/world/level/levelgen/feature_reeds.o \
	src/world/level/levelgen/feature_ore.o \
	src/world/level/levelgen/feature_spring.o \
	src/world/level/levelgen/feature_lake.o \
	src/world/level/levelgen/feature_snow.o \
	src/world/level/levelgen/caves.o

INCDIR = src
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl -lpsppower -lpspaudio -lpng -lz -lm -lstdc++

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Minecraft Pocket-Edition

PRESENT = presentation
PSP_EBOOT_ICON  = $(if $(wildcard $(PRESENT)/ICON0.PNG),$(PRESENT)/ICON0.PNG,NULL)
PSP_EBOOT_ICON1 = $(if $(wildcard $(PRESENT)/ICON1.PMF),$(PRESENT)/ICON1.PMF,NULL)
PSP_EBOOT_PIC0  = $(if $(wildcard $(PRESENT)/PIC0.PNG),$(PRESENT)/PIC0.PNG,NULL)
PSP_EBOOT_PIC1  = $(if $(wildcard $(PRESENT)/PIC1.PNG),$(PRESENT)/PIC1.PNG,NULL)
PSP_EBOOT_SND0  = $(if $(wildcard $(PRESENT)/SND0.AT3),$(PRESENT)/SND0.AT3,NULL)

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

EBOOT_ASSETS := $(filter-out NULL,$(PSP_EBOOT_ICON) $(PSP_EBOOT_ICON1) \
                                  $(PSP_EBOOT_PIC0) $(PSP_EBOOT_PIC1) \
                                  $(PSP_EBOOT_SND0))
EBOOT.PBP: $(EBOOT_ASSETS)

dist: EBOOT.PBP
	@rm -rf build && mkdir -p build/data
	@cp EBOOT.PBP build/
	@cp -r data/. build/data/
	@rm -rf build/data/sound/aac
	@printf 'MCPSP - Minecraft PSP (test build)\n\n=== Run on a real PSP ===\n1. On the memory stick make a folder:  PSP/GAME/MCPSP\n2. Put EBOOT.PBP and the data/ folder inside it, so you have:\n     PSP/GAME/MCPSP/EBOOT.PBP\n     PSP/GAME/MCPSP/data/\n3. Launch it from the PSP Game menu.\n\n=== PPSSPP ===\nJust open EBOOT.PBP.\n\nKeep EBOOT.PBP and data/ together - textures load from data/ next to\nthe EBOOT, and worlds save into a saves/ folder created beside it.\n' > build/README.txt
	@echo "Packaged -> build/  (copy its contents into ms0:/PSP/GAME/MCPSP/)"
