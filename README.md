<img width="768" height="256" alt="SPLASH" src="https://github.com/user-attachments/assets/c58041a0-fcc8-4499-b131-c75cea22dc7b" />

This repository contains a port of **Minecraft Pocket Edition** for the Sony PlayStation®Portable.

It runs on **every PSP™ model, including the 32 MB PSP™-1000** — see [Hardware](#hardware) for what the 1000 gives up.  
The port is based on **MCPE v0.6.1** — this means all *World Logic*, *Save Format*, *Block Interaction*, *Movement* etc.  
are ***exactly*** as in Pocket Edition on your phones[^1].  
This doesn't mean however it isn't improved upon, the current feature set sits somewhere around **v0.7.6**[^2]  
### This includes:  
* world generation,
* gamemodes,
* mobs,
* crafting,
* furnaces,
* chests,
* armor,
* TNT,
* the Nether Reactor,
* buckets,
* fire,
* signs,
* paintings,
* day light cycle,
* the tripod camera,
* audio,
* and saving.

> [!Note]
> Join the Discord for build help, bug reports and updates on the port:
> **https://discord.gg/uQddmU7Vra**

## About the port

### This is a **source-port, not the source code itself**!
The gameplay and world logic were re-interpreted line by line from the original code,   
and fully re-written for the PSP™ where the code needed it the most.  

The biggest difference is how the map is kept in memory.  
Originally MCPE holds the world data as a cache of separate chunk objects, each carrying its own block, data and light arrays.  

Instead, here the whole world[^3] is present at once,  
so all three of the bellow had to get much smaller than a byte per block.  

- `blocks` — block IDs are stored in **16×16×16 chunks**, same scheme Minecraft's console edition uses.  
  A single ID section such as ***air above the surface***, or ***stone below the surface**,
  get stored with one ID and costs nothing. Anything else gets a page:
  **4 bits per block** indexing a 16-entry palette, if there are more than 16 block types per chunk
  a full byte is used, resulting in a practically limitless palette per chunk where
  you are actively building. TLDR, 8 MB of World Data become roughly 3 MB.
- `data` — metadata is stored separately in **4 bits per block**: one 64-byte page per column, allocated on the first non-zero write.  
  Real world test prove that ~95% of columns never hold any metadata at all, so 4 MB is compressed to about 0.6 MB.
- `light` — sky and block light is stored in **16×16 horizontal planes** with a sentinel index,  
  also from the console editions. About 95% of sky planes and 80% of block-light planes are uniform,  
  and cost one index entry instead of a page, so 8 MB of lighting data becomes 1 MB.

Together that is roughly 20 MB of world data compressed down to about 4 MB,
which is what makes the whole map fit on a 32 MB PSP™ alongside the meshes.

The world is generated once at load around the spawn point, and the rest builds
lazily as you walk toward it. Only the mesh columns near the camera are drawn.
So it is the same *fixed* MCPE world, just held and streamed differently.

## Building

Make sure you have the [PSPDEV](https://github.com/pspdev/pspdev) toolchain on
your `PATH`, then:

```
make clean && make
```

This produces an `EBOOT.PBP`. To get a ready-to-copy folder instead:

```
make dist
```

Header dependencies are tracked, so editing a `.h` rebuilds everything that
includes it — a plain `make` is enough after the first build.

## Running

**On a PSP** — copy `MCPSP` into the memory stick's `PSP/GAME` directory

and launch it from the Game menu. Worlds are saved into a `saves/` folder created
next to the EBOOT.

**In PPSSPP** — just open the `EBOOT.PBP`.

Keep `EBOOT.PBP` and `data/` together; textures and sounds load from `data/`
next to the EBOOT.

## DualShock 3/Sixaxis

Options > Controls > Control Scheme > **Layout 4** puts the camera on the right analogue stick
and gives every action its own button
* place and break on L2/R2,
* the hotbar on L1/R1,
* crafting and inventory on Square/Triangle,
* third person and sneak on L3 and R3 respectivley.  
>[!IMPORTANT]
>A Dualshock 3/Sixaxis controller needs to be paired to the console,  
>and the required plugin below needs to be installed:

**[VanillaDS3Remapper](https://github.com/rereprep/VanillaDS3Remapper)** —
Total_Noob's DS3Remapper. Not shipped with this port: download `DS3Remapper.prx`
from there, and install it in `seplugins/` on the memory stick and enable it in
(`ms0:/seplugins/game.txt`, with the line `ms0:/seplugins/DS3Remapper.prx 1`).

Without it the extra inputs do not exist as far as any game is concerned. Read
through plain `sceCtrlReadBufferPositive` a DualShock is aliased onto a PSP pad:
L1 and L2 both return `0x100`, and the second analogue stick and L3/R3 do not output a signal.  
The extra buttons live in ctrl.prx's EXTENDED report,
which that call never asks for; the plugin hooks the service and asks for it.

The port does not require the plugin and never refuses to run without one: with
no Dualshock/Sixaxis in sight Layout 4 is greyed out in the Controls page,
and is quietly reported as Layout 1, however it is automatically enabled the moment the pad turns up.
A saved Layout 4 survives a boot on a plain PSP.

## Hardware

Every PSP model can run this port, however the models with less memory[^4]
have half of the available memory compared to other models,
and as such the world and it's meshes still take most of the heap.
The game measures the machine's user memory at boot — heap + what the kernel still holds,
so a fragmented heap cannot make a 3000 look like a 1000.
### The limitations on 32MB RAM models:

- **View distance** — it is limited to Tiny and Short out of the normally 4 available modes,
  Short is acceptable in most worlds, however.
  heavy caves and lava, run the heap to it's limit,
  where distant sections simply stop building until you get closer.
- **Sound** — a half-rate pack[^5] is used to free up the memory.
  It is audibly grainier, but saves 0.8 MB of Random Access Memory.

Otherwise, there are the same world sizes, same generation, same gameplay, and the same save files.
All later models[^4] + Emulators (depending on their configuration) get the full-resolution sound pack and all four view distances.

## Compatibility

Worlds use the real MCPE 0.6.1 on-disk format (`chunks.dat`, `level.dat`, `entities.dat`).  
All worlds made on the PSP™ open in MCPE v0.6.1, and vice versa.

## Disclaimer

This is unofficial homebrew not associated with Mojang AB, Sony Group Corporation or Microsoft Corporation.  
Run at your own risk! It comes with **no warranty of any kind** — see the licenses below.

I am not responsible for anything that happens to your console, your memory
stick, your save files or anything else while running it. That covers a console
that stops working, data loss, and a warranty you void by running homebrew.  
Running unsigned software on the PlayStation®Portable is something you choose to do,  
and any side effects that come with it are yours to deal with.

Practical version, because most of the risk here is boring and avoidable:

- **Back up your worlds.** They are plain files in the `saves/` directory next to the EBOOT.  
  copy them off the stick before updating. This port is still being worked on and bugs may corrupt a save file.
- **Keep a personal Archive.** Keep the current version archive you installed from in a safe location,
  a potentially bad update can corrupt the game, having a Plan B is safer than sorry.
- Nothing here writes outside its own folder and `ms0:/PSP/PHOTO`, and nothing touches the original firmware, **however**,  
  that is what the code does as of the date of writing this READ ME file,  
  any un-noted changes, and experimental builds may carry the risks of corruption.  
>[!WARNING]
>HERE BE DRAGONS!

If something does go wrong, open an issue with what happened and on which PSP™ model;
that is worth more than a warning nobody reads.

## Credits and Special Thanks

- Gameplay and world logic ported from the Minecraft Pocket Edition sources
  (0.6.1 as the base, later versions for the newer features).
- [**MCPE-0.8.1**](https://github.com/oldminecraftcommunity/MCPE-0.8.1) — the
  0.8.1 decompilation, used as a source for the features 0.6.1 never had before.
- [**Oreo**](https://github.com/Oreo80) — for help with the porting.
- [**CODINGBOTSTUDIO**](https://github.com/CODINGBOTSTUDIO) — contributed the
  code the 3D clouds are based on, and found the leak in `CompoundTag`'s put*,
  which dropped the tag already under a key instead of freeing it. Also spotted
  that exiting the app (HOME/START) skipped the world/player teardown that
  quitting to the menu does, that `worldFree` never cleared
  `preservedTileEntities`, and that world teardown terminated the chunk
  generation worker instead of waiting for it, which can strand the allocator
  lock.
- [**CYEVV**](https://github.com/CYEVV) — for helpibng fix in-game buttons that were not rendering with the 4444 texture format.
- [**SzyZET777**](https://github.com/SzyZET777) — for pointing out that the
  player-edit fast lane in `dirty.cpp` was open-coding the same shift loop at
  four call sites, each keeping the membership array in sync by hand. The queue
  stayed a fixed array (it is pushed to from the light cascade, which can run
  while the worldgen worker is inside the allocator — under `-fno-exceptions` a
  container that has to grow there corrupts the heap silently), but the index
  arithmetic moved into `editQueueRemoveAt` / `editQueuePushFront` /
  `editQueueFind`. Reading it properly is also what turned up `evict()` not
  clearing the queue: entries key on the resident *slot*, so one outliving its
  chunk blocked the next chunk's edits from the fast lane.
  not rendering with the 4444 texture format.
- [**Stann**](https://github.com/ThatStann) — for drawing the delete-world **X** button
  (both states, in `touchgui.png`), the controller drawings the Controls page
  labels (`data/images/gui/controls/psp.png`, `go.png`), and the button-icon
  sheet the in-game hint row is built from (`data/images/gui/tooltips.png`) —
  every button in a pressed and a released state, the DualShock's shoulders,
  sticks and PlayStation® Start/Select included. Also spotted the uneven border
  widths on the world-type pills that turned out to be a fractional nine-patch
  corner.
- **Total_Noob** — **DS3Remapper**, for the plugin that makes a DualShock's extra
  buttons and its right stick reach a game at all. Not bundled here; it lives at
  [rereprep/VanillaDS3Remapper](https://github.com/rereprep/VanillaDS3Remapper)
  under its own GPLv3, and Layout 4 needs it (see [DualShock 3](#dualshock-3)).

### other PSP™ projects this port has build of off

- [**DaedalusX64**](https://github.com/DaedalusX64/daedalus) — the N64 emulator
  for PSP™, and the sharpest PSP™ renderer to read. `src/util/fast_memcpy.cpp` is
  it's `memcpy_vfpu` (`Source/SysPSP/Utility/FastMemcpyPSP.cpp`, © 2009 Raphael,
  modified by Corn) copied essentially verbatim; Daedalus is
  GPL-2.0-**or-later**, so it is carried here under GPLv3. Two of its practices
  were adopted rather than copied: composing simple model matrices by hand
  instead of paying the `sceGum*` stack for them
  (`Source/SysPSP/HLEGraphics/RendererPSP.cpp` uses no `sceGum*` at all), and
  writing D-cache ranges back **without** invalidating them when the reader is
  the GE (`NativeTexturePSP.cpp` writes back for textures and keeps
  invalidate for the audio path, where the CPU is the one reading).

## License

The original engine code written for this port — the world storage, the PSP™
renderer and mesher, the GU/graphics layer, and everything else authored here
for the PSP™ — is released under the **GNU General Public License v3.0**
(see [LICENSE](LICENSE)). In short: use it, study it, fork it — but if you
distribute a modified version or a binary built from it, you have to release
its complete source under the same license. This covers **every version of the
project, including the earlier ones** — there is no MIT branch of it still on
offer. (Copies someone already received under the old MIT terms keep those
rights; that part is not up to anyone.)

**What the GPL does not cover:** the gameplay and world logic in this project is
ported from the Minecraft Pocket Edition 0.6.1 sources, and Minecraft is the
intellectual property of Mojang / Microsoft. That copyright, and the
"Minecraft" trademark, are theirs — the GPL grant applies only to the original
PSP™ engine work, not to anything derived from Mojang's code.

This is a non-commercial, educational project and is not affiliated with,
endorsed by, or associated with Mojang or Microsoft. The game assets bundled
under `data/` (textures such as `terrain.png`, sounds, the font, mob and GUI
art) are the property of Mojang / Microsoft and are not covered by the GPL
above; they are included only to make this educational port runnable.
If you are a rights holder and want anything removed, open an issue.

[^1]: If your phones still hoists the same .APK/.IPA from circa 2013 :P
[^2]: https://minecraft.wiki/w/Pocket_Edition_v0.7.0_alpha
[^3]: [Each world is 256x128x256 blocks deep, tall and wide.](https://minecraft.wiki/w/World_boundary#Bedrock_Edition_2)
[^4]: https://consolemods.org/wiki/PSP:Model_Differences
[^5]: 11 kHz instead of the usual 22 kHz
