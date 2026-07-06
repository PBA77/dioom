# Dioom

Dioom is a minimal Wolfenstein-style raycaster written in C. The whole scene is rendered in software into a `uint32_t` framebuffer, while SDL2 is used only for the window, input, audio, and presenting the finished texture.

Wall, floor, and ceiling textures are loaded from `assets/textures.ppm`: a 4x2 atlas with eight 64x64 tiles. The current set mixes a dark gothic dungeon with gloomy forest tiles: dirt, moss, roots, bark, and a black tree canopy. A missing or invalid atlas terminates the program with an error.

Monsters are loaded from `assets/monsters.ppm`: a 4x24 atlas, six enemy types with four directions and four 64x64 animation frames each. In each type block, the rows are front, right side, back, and left side. The front row uses idle/windup/attack/gesture frames, while the other directions treat the columns as walking or movement variants. Magenta `#ff00ff` is treated as transparent.

The giant skeleton and boss are loaded from separate 4x4 atlases: `assets/giant_skeleton.ppm` and `assets/boss.ppm`, with four directions and four 128x128 animation frames. Magenta `#ff00ff` is transparent. A missing atlas terminates the program with an error.

Forest trees are loaded from `assets/trees.ppm`: a 4x1 atlas with four 64x64 billboard variants. Magenta `#ff00ff` is transparent. A missing atlas terminates the program with an error.

Forest cemetery shrines are loaded from `assets/houses.ppm`: a 2x2 atlas with 64x64 tiles for the front, sides, back, and roof. Shrine interiors use simple textured boxes for furniture and textured cylinders for barrels. Their surfaces are loaded from `assets/furniture.ppm`: an 8x1 atlas with eight 64x64 tiles. A missing atlas terminates the program with an error.

Cult relics are loaded from `assets/relics.ppm`: a 4x1 atlas with four 32x32 pickup sprites. Magenta `#ff00ff` is transparent. A missing atlas terminates the program with an error.

Ten monsters patrol the level. Enemies detect the player through line of sight and FOV, remember the last known position, chase, keep distance, and react to a nearby monster that can see the player. The skeleton shoots fire projectiles, while the other types, including the ghost, mostly attack in melee.

The player starts with a short-range knife. The hitscan pistol and fireball with separate ammo are unlocked through pickups. Fireballs travel as projectiles, explode on walls or enemies, and deal area damage. Hit monsters get a short pain flash, and death produces a procedural orange pop. The background music uses an experimental software FM synthesizer; the forest uses a quiet, grim drone, and each relic crypt selects a different MIDI track from `assets/music/`.

The game starts in a dark forest with only the knife; the pistol and fireball must be found as pickups. Levels are generated at runtime: rooms, corridors, doors, secrets, torches or campfires, items, and monsters are created from the run seed. There are four generator modes: a classic rooms-and-corridors layout, a tight one-tile maze with an entrance and distant exit, a boss level with a maze and boss chamber, and a dark forest as a large fenced area with trees, campfires, cemetery shrines, monsters, items, and dungeon entrances. The forest has four relic dungeon entrances and a separate boss gate. Shrines block movement, and pressing `F` at their front doors enters a separate interior with furniture, caches, and one-time loot. Entering saves the current forest state, and exiting a dungeon or shrine restores the same forest and camera position near the entrance. Restarting with `R` creates the next seed in the current mode. `make dump` uses a fixed test seed so rendering and validations stay deterministic.

The map contains items: medkits, ammo, rapid fire, damage boost, keys, fireball ammo/unlock, gold, shrines, and four cult relics. Pickups are also placed in side branches so exploration provides resources for combat. Monsters drop small gold piles, shrines give a one-time Diablo-style effect, and bone piles provide atmosphere without being pickup items. There are normal doors, locked key doors, and secret walls opened through interaction. The top-right corner has a fog-of-war minimap that reveals terrain around the player, and `Tab` shows a larger automap.

Collecting all four relics unlocks the boss gate in the forest. The boss is much larger and tougher than normal enemies, has a stronger melee attack, and fires a slower, painful ranged projectile. Victory happens only after killing the boss. Combat includes light knockback on monster collisions, sound samples for shots/pickups/explosions, pause, restart, and fullscreen. Difficulty levels are easy, normal, hard, and nightmare; higher levels increase enemy HP and damage.

## Renderer

The renderer is fully software-based and composes the final image in a `uint32_t framebuffer[640x480]`. SDL2 creates the window, receives input, handles audio, and displays the finished texture; raycasting, sprites, lights, effects, and UI are computed on the CPU.

Main renderer features:

- Column-based wall raycaster with DDA, distance correction, PPM atlas texturing, and support for normal, locked, and opening doors.
- Separate floor and ceiling pass with perspective texture mapping, distance lighting, and fog.
- Forest mode with a procedural night sky, stars, moon, moon glow, map-cached moon visibility, and cool moonlight mixed into the ground and walls.
- Two quality modes: `fast` uses simpler shading, while `pbr` adds a lightweight PBR-like model with material parameters, roughness/metallic/wetness/specular, Fresnel, texel brightness, and bump/normal lighting derived from textures.
- Depth buffers: a column `z_buffer` for classic sprites and a full `depth_buffer` for screen-space passes, fog, 3D props, and post-processing effects.
- Back-to-front billboard sorting: monsters, pickups, projectiles, torches/campfires, particles, portals, and trees are occluded by walls through `z_buffer`.
- Multi-direction animated monster sprites, separate larger atlases for the giant skeleton and boss, boss/giant scaling, pain flash, attack windup glow, and hit rim.
- Billboards for trees, portals, relics, shrines, gold, bone piles, projectiles, explosions, fireballs, and bolts with magenta `#ff00ff` transparency.
- Dungeon torches and forest campfires rendered as procedural flame sprites with flicker, glow, `glow_buffer` contribution, and local influence on walls, floors, ceilings, and fog.
- Dynamic player and weapon lighting: player torch, muzzle flash, hitscan shot trace, and glow/light from fireballs, explosions, relics, and shrines.
- Experimental volumetric fog: distance fog on walls, floors, ceilings, and sprites, plus a separate animated cloudy fog/scattering pass over the world buffer. Fog is heavier near the ground and avoids directly painting over forest walls.
- Dynamic shadows under sprites and world objects.
- Floor decals from `assets/decals.ppm`, including scorch marks after explosions, projected onto the perspective floor and mixed with fog.
- Wall decals from `assets/wall_decals.ppm`, indexed per tile/wall side and applied while rendering wall columns.
- Forest shrines rendered as simple solids using ray/box intersection, textured walls, separately drawn roofs and gables, and fog.
- Shrine interiors contain simple 3D props: textured boxes and cylinders, triangulated in software, with basic surface lighting and `depth_buffer` testing.
- Soft particles for smoke/sparks with soft fade at wall intersections based on `z_buffer`.
- Forest weather overlay: subtle rainy haze and vertical rain streaks over the scene.
- Optional light buffer/pseudo-deferred composite, threshold bloom with a two-stage blur buffer, and three post-processing intensity presets.
- Optional selective edge antialiasing based on luminance contrast, applied only to the game world.
- Optional color grading with contrast, brightness, tint, LUT-like channel correction, vignette, and separate preset variants; the forest has its own cooler grade.
- Feedback effects: screen shake through camera jitter, red hit flash on screen edges, directional damage indicator, hit marker, recoil/bob, and animated weapon sprites.
- HUD and UI rendered by the same software raster: crosshair, weapon, shot trace, HP/ammo bars, relics, gold, fog-of-war minimap, full automap, interaction prompt, menu, shop, pause, victory screen, and FPS/timing overlays.
- Renderer pass profiling under `F4` and `--profile-dump`: total, floor, wall, sprite, fog, bloom, and post.
- Headless render dumps: `make dump`, `--dump`, `--dump-house`, `--dump-quality`, and `--profile-dump` write deterministic PPM frames for regression checks.

Heavier post-processing passes can be changed with `--effects full`, `--effects preset2`, `--effects preset3`, or from the settings menu. Render quality can be set with `--quality pbr|fast` or from the menu. Native SDL2 builds use a small render worker pool by default; force a specific count with `--render-threads N` from 1 to 16. WASM stays single-threaded.

## Build

```sh
make
./dioom
```

The Makefile uses Homebrew SDL2 by default:

```sh
SDL2_PREFIX=/opt/homebrew/opt/sdl2 make
```

## Web/WASM

The committed static web bundle lives in `web/`:

- `web/index.html`
- `web/index.js`
- `web/index.wasm`
- `web/index.data`

`web/index.data` contains the preloaded runtime assets and `web/dioom.ini`. The source shell is `web/shell.html`; rebuild the bundle after C, asset, shell, or web settings changes:

```sh
make wasm
```

On this machine Emscripten needs the explicit Python path:

```sh
make wasm EMSDK_PYTHON=/opt/homebrew/opt/python@3.14/bin/python3.14
```

The `web/` directory is deployable as static output, for example:

```sh
vercel deploy web --prod --yes
```

## Controls

- `W/S` or up/down arrows: move forward/backward
- `A/D` or left/right arrows: turn
- `Q/E`: strafe
- `1/2/3`: knife / pistol after pickup / fireball after pickup
- `Space` or left mouse button: fire the selected weapon
- `F`: interact with doors, secrets, dungeon/shrine entrances and exits, and lootable furniture
- `H`: show/hide the objective hint
- `Tab`: full automap
- `P`: pause
- `R`: restart the run
- `F3/F4`: FPS / timing
- `F8/F9`: save/load menu with 8 slots, `dioom_slot1.sav` through `dioom_slot8.sav`
- `F11`: fullscreen
- `Esc`: game menu
- Menu: `W/S` or arrows select an item, `Enter`/`Space` confirms, and `Esc` goes one level back. Settings are in a separate submenu where `A/D` changes difficulty, quality, post-processing, FX/music volume sliders, and fullscreen. The main menu contains start/resume, restart run, save, load, settings, and exit.

Menu settings are saved to `dioom.ini` on exit and after option changes. Supported keys are `difficulty=easy|normal|hard|nightmare`, `quality=pbr|fast`, `post_process=full|off`, `fullscreen=0|1`, `sfx_volume=0..8`, `music_volume=0..8`, and hidden `trainer=0|1`. Trainer blocks player damage, prevents ammo and fireball ammo consumption, and starts with all four relics.

## Render test

You can save a single frame without opening a window:

```sh
make dump
open frame.ppm
```

To inspect a shrine interior, render a separate frame:

```sh
./dioom --dump-house /tmp/house.ppm
```

To profile a deterministic frame with a specific render thread count:

```sh
./dioom --profile-dump pbr forest /tmp/forest.ppm --render-threads 6
```
