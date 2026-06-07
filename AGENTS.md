# AGENTS.md

This file provides guidance to the AI agent when working with code in this repository.

## Project

PvZ-Portable is a cross-platform community reimplementation of **Plants vs. Zombies: Game of the Year Edition** (v1.2.0.1073). C++20, SDL2 + OpenGL ES 2.0, CMake. The repo contains **no copyrighted game assets** — users supply their own `main.pak` and `properties/`.

## Architecture

4-layer top-down dependency structure. See `docs/architecture.md` for the full map.

```
Layer 3: Lawn/              — Game logic (Board, Plant, Zombie, Projectile, UI, save system)
Layer 2: Sexy.TodLib/       — Engine lib (Reanimator, TodParticle, EffectSystem, Definition parser)
Layer 1: SexyAppFramework/  — Framework (SexyAppBase main loop, Graphics, Widget UI, Sound, PakInterface)
Layer 0: platform/          — Platform adapters (SDL window, input, OpenGL context)
```

`Board` (~290KB) is the core battlefield and main game loop. `ConstEnums.h` is the domain glossary (~1400 lines of enums).

## Build

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Prerequisites: CMake, Ninja, C++20 compiler, SDL2, libpng, libjpeg-turbo, zlib, libogg, libvorbis, libopenmpt, mpg123.

## Code style

- **Variable naming:** `mMember`, `theParameter`, `aLocal` (SexyAppFramework convention). Public member access preferred over getters/setters.
- **Header guards:** `#ifndef __MODULE_H__` / `#define __MODULE_H__` / `#endif` (not `#pragma once`).
- **License header:** Every `.h` and `.cpp` must have the standard LGPL-3.0-or-later header block (see existing files).
- **`/*inline*/` comments** mark functions intended to be inlined — do not remove them.

## Critical rules

- **Do not subclass plants or zombies.** Game entities use enum-branching in `Update()` methods (switch on `SeedType` / `ZombieType`), not class hierarchies. Follow the existing pattern exactly.
- **Do not reorder `ConstEnums.h` values.** Integer values are serialized in save files and must remain stable. New values go before the `NUM_*` sentinel.
- **Do not add `TodDrawTriangle.cpp` or `TodDrawTriangleInc.cpp` to the build.** They are compile-time-included files, not standalone translation units.
- **Use TLV serialization (`.v4`) for save data**, not raw memory dumps. See `DataSync` and `scripts/pvzp-v4-converter.py`.

## Testing

**No automated test suite.** Manual gameplay testing is essential across affected GameModes (Adventure, Survival, Minigames). Test save/load roundtrips if modifying `DataSync` or game state. Test on a non-x86 platform if possible (WASM via Emscripten is easiest).

Inspect `.v4` save files:
```bash
python scripts/pvzp-v4-converter.py info <savefile.v4>
python scripts/pvzp-v4-converter.py export <savefile.v4> dump.yaml
```

## Custom CLI flags

The binary accepts modding flags beyond the original game. Defined in `LawnApp.cpp` arg parsing:

- `--zombie-multiplier=N` — multiplies zombie spawn points budget (default 1 = vanilla)
- `--zombie-hp-multiplier=N` — multiplies zombie HP (default 1 = vanilla)
- `-resdir=PATH` — override resource directory for `main.pak` and `properties/`

## More context

- Architecture map: `docs/architecture.md`
- Architecture Decision Records: `docs/adr/`
- Community wiki (game data): https://wiki.pvz1.com
- Memory reference: https://pvz.tools/memory/
