# Minecraft PE for PSP

This repository contains a port of **Minecraft Pocket Edition 0.6.1** to the
Sony PlayStation Portable (PSP-2000 and later).

It has everything MCPE 0.6.1 has — world generation, survival and creative,
mobs, crafting, furnaces, chests, armor, TNT, the Nether Reactor, day/night,
saving — with the same look and behaviour. There are probably still some bugs.

## About the port

This is a **source-based port, not the source code itself.** The gameplay and
world logic are ported piece by piece from the original MCPE 0.6.1 sources and
adapted for the PSP, but the engine underneath is different where the hardware
needs it to be.

The biggest difference is how the map is kept in memory. MCPE holds the world
as a cache of separate chunk objects, each carrying its own block, data and
light arrays. On the PSP we instead hold the whole fixed 256×128×256 world in
a few flat contiguous arrays:

- `blocks` — one 8 MB array of block IDs
- `data` — block metadata, packed to **4 bits per block** (4 MB) instead of a
  full byte, so it fits the PSP's small heap
- `light` — sky and block light packed into one byte per block (8 MB)

The world is generated once at load around the spawn point, and the rest builds
lazily as you walk toward it. Only the mesh columns near the camera are drawn.
So it is the same *fixed* MCPE world, just held and streamed differently.

## Building

Make sure you have the [PSPDEV](https://github.com/pspdev/pspdev) toolchain on
your `PATH`, then:

```
make clean && make
```

This produces `EBOOT.PBP`. To get a ready-to-copy folder instead:

```
make dist
```

> The Makefile does not track header dependencies — after editing any `.h`, run
> `make clean && make`, or a stale object file will crash on hardware.

## Running

**On a PSP** — copy onto the memory stick so you have:

```
PSP/GAME/MCPSP/EBOOT.PBP
PSP/GAME/MCPSP/data/
```

and launch it from the Game menu. Worlds save into a `saves/` folder created
next to the EBOOT.

**In PPSSPP** — just open `EBOOT.PBP`.

Keep `EBOOT.PBP` and `data/` together; textures and sounds load from `data/`
next to the EBOOT.

## Compatibility

Worlds use the real MCPE 0.6.1 on-disk format (`chunks.dat`, `level.dat`,
`entities.dat`). A world made on the PSP opens in MCPE 0.6.1, and a world copied
off a phone opens on the PSP.

## Credits

- Gameplay and world logic ported from the Minecraft Pocket Edition 0.6.1
  sources. Not affiliated with Mojang or Microsoft.
- [**Oreo**](https://github.com/Oreo80) — helped with the porting.
- [**CYEVV**](https://github.com/CYEVV) — helped fix in-game buttons that were
  not rendering with the 4444 texture format.
