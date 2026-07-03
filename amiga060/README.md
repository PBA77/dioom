# DIOOM Amiga 060 RTG Tech Demo

This is a separate AmigaOS 3.x / 68060 RTG experiment. It is not a port of the full SDL2 game yet.

The demo is intentionally small:

- `160x100` CPU-side chunky framebuffer, scaled 2x2 into a `320x200x8` P96 RTG CLUT screen.
- Direct 8-bit chunky presentation through `Picasso96API.library` and `p96WritePixelArray`.
- Fixed-point raycaster with 8x8 wall textures downsampled from `assets/textures.ppm`, flat floor/ceiling bands, billboards, pickups, shooting, and a minimal HUD.
- Keyboard movement and FPS logging.
- No audio, assets, PBR, bloom, volumetric fog, generated levels, saves, or SDL2 code.

## Build

```sh
make -C amiga060
```

The Makefile requires a real AmigaOS m68k C toolchain:

- `m68k-amigaos-gcc`, or
- vbcc `vc` with an AmigaOS m68k target.

It deliberately refuses to use `m68k-elf-gcc` or unrelated tools. On this Mac, `/opt/homebrew/bin/vc` is Vercel CLI, not vbcc.

Current local setup uses `~/.local/bin/m68k-amigaos-gcc`, a Docker wrapper around:

```text
amigadev/m68k-amigaos-gcc:latest
```

The default build currently uses `-m68020-60 -msoft-float`, matching the 060 emulator profile while still avoiding hardware floating-point instructions in the demo code.

The P96 development headers required by this port are vendored under `p96include/` from Aminet `P96Develop.lha`; the runtime still requires `Picasso96API.library` on the Amiga side.

Expected output after a successful build:

```text
amiga060/build/dioom060
```

## WinUAE Test

Use the local config:

```sh
/Applications/WinUAE.app/Contents/MacOS/WinUAE -f amiga060/winuae/dioom060.uae
```

The config is based on the existing local WinUAE setup and mounts this folder as:

```text
DIOOM060:
```

After booting Workbench, run:

```text
DIOOM060:build/dioom060
```

Controls:

- Arrow keys: move/turn
- `W` / `S`: move forward/back
- `A` / `D`: turn left/right
- `Esc`: exit

## Current Limitations

- The demo now requires a working P96/Picasso96 RTG setup with an 8-bit CLUT mode. If `320x200x8` mode selection fails, it prints a clear error and exits.
- Wall rendering runs at 160x100 internally, then scales to a 320x200 8-bit chunky staging buffer before `p96WritePixelArray`.
- Wall textures are compiled into `wall_textures.h`; the Amiga executable does not load `assets/textures.ppm` at runtime.
- The palette is 256 entries. The current renderer uses the first 64 entries for base colors plus wall shade ramps; entries 64-255 are initialized as a simple RGB cube for later texture expansion.
- The render path is a tech-demo baseline for profiling, not visual parity with the current desktop game.
