# Project Instructions

## What This Project Is

This repository is a small C/SDL2 software raycaster inspired by Wolfenstein-style rendering, with dark dungeon/forest mood and arcade combat. The main implementation is intentionally kept in one C file:

- `src/main.c`: game loop, software renderer, level generation, AI, combat, pickups, HUD, audio, validation, and dump mode.
- `assets/`: required runtime assets. PPM atlases are loaded by the C game; PNG source/preview files are kept alongside them. WAV files in `assets/sfx/` are required for audio.
- `Makefile`: builds `raycaster` and provides `make dump` for deterministic validation/render output.

The renderer writes into a `uint32_t` framebuffer and uses SDL2 only for the window, input, audio device, and presenting the final texture. Most visual/gameplay assets are procedural or atlas-based: walls/floors/ceilings, monsters, trees, relics, projectiles, fog, light, particles, decals, HUD, and item sprites.

## Gameplay Shape

The game starts in a dark forest with a knife. Pistol and fireball are unlocked through pickups. The forest contains dungeon entrances, relic progression, a boss gate, fog-of-war minimap, monsters, items, trees, campfires, and Diablo-like loot/props such as gold, shrines, and bone piles. Dungeons and boss levels are generated at runtime.

## Build And Test

Use the existing Makefile:

```sh
make
./raycaster
make dump
```

`make dump` is the main regression check. It runs deterministic validations and writes `frame.ppm`. Run it after gameplay, renderer, generator, asset-loading, AI, combat, or input changes.

SDL2 is expected from Homebrew by default:

```sh
SDL2_PREFIX=/opt/homebrew/opt/sdl2 make
```

## Required Assets And No Fallbacks

- Do not add default fallbacks for missing tools, files, assets, services, APIs, configuration, or failed operations.
- If a required dependency or artifact is missing, fail clearly with an actionable error instead of silently substituting different behavior.
- Missing texture, monster, tree, relic, or sound assets should remain hard errors.
- Do not silently replace missing generated/image assets with procedural placeholders unless the user explicitly asks for that behavior.
- New visual gameplay assets such as monsters, items, weapons, portals, decals, and textures should be generated or edited as bitmap image assets, not drawn procedurally in code, unless the user explicitly asks for procedural art.

## Development Notes

- Prefer small, direct edits that match the current single-file architecture.
- Keep generated gameplay deterministic where validation depends on `LEVEL_TEST_SEED`.
- Preserve existing asset formats and atlas conventions unless the task explicitly changes them.
- Do not introduce a new engine, framework, asset pipeline, or project structure unless the user explicitly asks for it.
- Avoid committing generated dumps such as `frame.ppm`, `frame.png`, forest forward frames, or local editor/cache artifacts.
