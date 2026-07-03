/*
 * DIOOM Amiga 060 RTG 8-bit chunky tech demo.
 *
 * This file is intentionally independent from ../src/main.c. It ports the
 * core raycaster/game loop shape to a small fixed-point AmigaOS program.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/intuitionbase.h>
#include <libraries/dos.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/Picasso96.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wall_textures.h"

#define SCREEN_W 320
#define SCREEN_H 200
#define RENDER_SCALE 2
#define RENDER_W (SCREEN_W / RENDER_SCALE)
#define RENDER_H (SCREEN_H / RENDER_SCALE)
#define FRAME_PIXELS (RENDER_W * RENDER_H)
#define FIX_SHIFT 16
#define FIX_ONE (1L << FIX_SHIFT)
#define MOVE_STEP 0x0800L
#define TURN_STEP 1
#define WALL_HEIGHT_SCALE 60
#define MAP_W 24
#define MAP_H 24
#define FOV_SCALE 220
#define RENDER_X_STEP 1
#define RAY_COUNT (RENDER_W / RENDER_X_STEP)
#define MAX_RAY_DELTA (FIX_ONE * 64)
#define DIST_LUT_SHIFT 12
#define DIST_LUT_MAX (MAX_RAY_DELTA >> DIST_LUT_SHIFT)
#define WALL_SHADE_COUNT 4
#define MAX_MONSTERS 8
#define MAX_ITEMS 8
#define MAX_SPRITES (MAX_MONSTERS + MAX_ITEMS)
#define PLAYER_MAX_HEALTH 100
#define START_AMMO 24
#ifndef AUTO_EXIT_FRAMES
#define AUTO_EXIT_FRAMES 0
#endif

#define RAWKEY_ESC 0x45
#define RAWKEY_W 0x11
#define RAWKEY_A 0x20
#define RAWKEY_S 0x21
#define RAWKEY_D 0x22
#define RAWKEY_SPACE 0x40
#define RAWKEY_UP 0x4c
#define RAWKEY_DOWN 0x4d
#define RAWKEY_RIGHT 0x4e
#define RAWKEY_LEFT 0x4f

typedef int32_t fix;

typedef struct {
    fix x;
    fix y;
} Vec2Fix;

typedef struct {
    Vec2Fix pos;
    UBYTE active;
    UBYTE hp;
    UBYTE flash;
} Monster;

typedef struct {
    Vec2Fix pos;
    UBYTE active;
    UBYTE type;
} Item;

typedef struct {
    Monster monsters[MAX_MONSTERS];
    Item items[MAX_ITEMS];
    UBYTE health;
    UBYTE ammo;
    UBYTE relics;
    UBYTE shoot_cooldown;
    UBYTE hit_flash;
} GameState;

typedef struct {
    fix depth;
    int screen_x;
    int height;
    UBYTE color;
    UBYTE active;
} RenderSprite;

typedef struct {
    fix ray_x;
    fix ray_y;
    fix delta_x;
    fix delta_y;
    BYTE step_x;
    BYTE step_y;
} RaySetup;

struct Library *P96Base = NULL;

static UBYTE framebuffer[FRAME_PIXELS];
static RaySetup ray_table[64][RAY_COUNT];
static UBYTE ray_table_ready = 0;
static UBYTE wall_textures_shaded[WALL_SHADE_COUNT][WALL_TEX_COUNT][WALL_TEX_SIZE * WALL_TEX_SIZE];
static fix wall_tex_step[RENDER_H * 2 + 1];
static UWORD wall_height_lut[DIST_LUT_MAX + 1];
static UBYTE wall_texture_tables_ready = 0;
static WORD p96_pens[] = {~0};
static UWORD rtg_screen_w = SCREEN_W;
static UWORD rtg_screen_h = SCREEN_H;
static UWORD rtg_dest_x = 0;
static UWORD rtg_dest_y = 0;
static UBYTE rtg_use_direct = 0;
static UBYTE rtg_no_direct = 0;
static UBYTE rtg_use_vsync = 0;
static UBYTE rtg_use_prof = 1;
static UBYTE rtg_no_present = 0;
static UBYTE rtg_present_1x = 0;
static UBYTE render_half_rays = 1;
static UBYTE rtg_color_mode = 0;
static RGBFTYPE rtg_rgb_format = RGBFB_CLUT;
static UBYTE rtg_direct_logged = 0;
static UBYTE rtg_fallback_logged = 0;
static UWORD rtg_dupword[256];
static ULONG rtg_duppair[65536];
static UBYTE rtg_palette_rgb[256][3];
#ifdef __GNUC__
static UBYTE rtg_framebuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(16)));
static UBYTE rtg_framebuffer16[SCREEN_W * SCREEN_H * 2] __attribute__((aligned(16)));
static UBYTE rtg_framebuffer24[SCREEN_W * SCREEN_H * 3] __attribute__((aligned(16)));
static UBYTE rtg_framebuffer32[SCREEN_W * SCREEN_H * 4] __attribute__((aligned(16)));
#else
static UBYTE rtg_framebuffer[SCREEN_W * SCREEN_H];
static UBYTE rtg_framebuffer16[SCREEN_W * SCREEN_H * 2];
static UBYTE rtg_framebuffer24[SCREEN_W * SCREEN_H * 3];
static UBYTE rtg_framebuffer32[SCREEN_W * SCREEN_H * 4];
#endif

static const UBYTE map_data[MAP_H][MAP_W + 1] = {
    "111111111111111111111111",
    "100000000000000000000001",
    "102000011111000111100001",
    "100000010001000100100001",
    "100111010001000100100001",
    "100100000001000000100001",
    "100101111101111110100001",
    "100100000000000010100001",
    "100001111111110010000001",
    "100001000000010011111001",
    "100001000330010000001001",
    "100001000000011111101001",
    "100000000000000000001001",
    "100111110111111101111001",
    "100100010100000101000001",
    "100100010100000101000001",
    "100100000100000001000001",
    "100111111111011111011101",
    "100000000000010000000001",
    "100011111111010111110001",
    "100010000000000100010001",
    "100010222000000100010001",
    "100000000000000000000001",
    "111111111111111111111111",
};

static const WORD sin_lut[64] = {
    0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
    23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609,
    32767, 32609, 32137, 31356, 30273, 28898, 27245, 25329,
    23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539, -9512, -6393, -3212
};

static fix to_fix(int v)
{
    return (fix)(v << FIX_SHIFT);
}

static fix mul_fix(fix a, fix b)
{
    return (fix)(((int64_t)a * (int64_t)b) >> FIX_SHIFT);
}

static fix mul_fix_fast(fix a, fix b)
{
    return (fix)(((a >> 4) * (b >> 4)) >> 8);
}

static fix mul_frac_delta(fix frac, fix delta)
{
    return (fix)(((frac >> 4) * (delta >> 4)) >> 8);
}

static fix div_fix(fix a, fix b)
{
    int64_t q;

    if (b == 0) {
        return 0x7fffffffL;
    }
    q = ((int64_t)a << FIX_SHIFT) / b;
    if (q > 0x7fffffffLL) {
        return 0x7fffffffL;
    }
    if (q < -0x7fffffffLL) {
        return -0x7fffffffL;
    }
    return (fix)q;
}

static fix sin_fix(int angle)
{
    return ((fix)sin_lut[angle & 63]) << 1;
}

static fix cos_fix(int angle)
{
    return sin_fix(angle + 16);
}

static int map_at(int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
        return 1;
    }
    return map_data[y][x] - '0';
}

static void init_ray_table(void)
{
    int angle;
    int ray;

    if (ray_table_ready) {
        return;
    }

    for (angle = 0; angle < 64; ++angle) {
        fix dir_x = cos_fix(angle);
        fix dir_y = sin_fix(angle);
        fix plane_x = -mul_fix(dir_y, FOV_SCALE << 8);
        fix plane_y = mul_fix(dir_x, FOV_SCALE << 8);

        for (ray = 0; ray < RAY_COUNT; ++ray) {
            int screen_x = ray * RENDER_X_STEP + (RENDER_X_STEP / 2);
            fix camera = (fix)(((int32_t)((screen_x * 2) - RENDER_W) * FIX_ONE) / RENDER_W);
            fix ray_x = dir_x + mul_fix(plane_x, camera);
            fix ray_y = dir_y + mul_fix(plane_y, camera);
            RaySetup *setup = &ray_table[angle][ray];

            setup->ray_x = ray_x;
            setup->ray_y = ray_y;
            setup->delta_x = ray_x == 0 ? 0x7fffffffL : div_fix(FIX_ONE, ray_x < 0 ? -ray_x : ray_x);
            setup->delta_y = ray_y == 0 ? 0x7fffffffL : div_fix(FIX_ONE, ray_y < 0 ? -ray_y : ray_y);
            if (setup->delta_x > MAX_RAY_DELTA) setup->delta_x = MAX_RAY_DELTA;
            if (setup->delta_y > MAX_RAY_DELTA) setup->delta_y = MAX_RAY_DELTA;
            setup->step_x = ray_x < 0 ? -1 : 1;
            setup->step_y = ray_y < 0 ? -1 : 1;
        }
    }

    ray_table_ready = 1;
}

static fix abs_fix(fix v)
{
    return v < 0 ? -v : v;
}

static void init_game(GameState *game)
{
    static const struct {
        int x;
        int y;
        UBYTE hp;
    } monsters[MAX_MONSTERS] = {
        {8, 5, 24}, {15, 4, 24}, {11, 10, 30}, {19, 10, 24},
        {5, 16, 24}, {16, 16, 30}, {8, 21, 24}, {20, 21, 40}
    };
    static const struct {
        int x;
        int y;
        UBYTE type;
    } items[MAX_ITEMS] = {
        {6, 3, 0}, {13, 3, 1}, {20, 5, 0}, {3, 12, 1},
        {13, 13, 0}, {21, 14, 0}, {6, 20, 1}, {18, 22, 0}
    };
    int i;

    memset(game, 0, sizeof(*game));
    game->health = PLAYER_MAX_HEALTH;
    game->ammo = START_AMMO;

    for (i = 0; i < MAX_MONSTERS; ++i) {
        game->monsters[i].pos.x = to_fix(monsters[i].x) + FIX_ONE / 2;
        game->monsters[i].pos.y = to_fix(monsters[i].y) + FIX_ONE / 2;
        game->monsters[i].hp = monsters[i].hp;
        game->monsters[i].active = 1;
    }

    for (i = 0; i < MAX_ITEMS; ++i) {
        game->items[i].pos.x = to_fix(items[i].x) + FIX_ONE / 2;
        game->items[i].pos.y = to_fix(items[i].y) + FIX_ONE / 2;
        game->items[i].type = items[i].type;
        game->items[i].active = 1;
    }
}

static void clear_frame(void)
{
    UBYTE *p = framebuffer;
    int i;

    for (i = 0; i < RENDER_W * (RENDER_H / 2); ++i) {
        *p++ = 8;
    }
    for (; i < FRAME_PIXELS; ++i) {
        *p++ = 10;
    }
}

static void fill_rect(int x0, int y0, int x1, int y1, UBYTE color)
{
    int x;
    int y;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= RENDER_W) x1 = RENDER_W - 1;
    if (y1 >= RENDER_H) y1 = RENDER_H - 1;
    if (x0 > x1 || y0 > y1) return;

    for (y = y0; y <= y1; ++y) {
        UBYTE *row = framebuffer + y * RENDER_W;
        for (x = x0; x <= x1; ++x) {
            row[x] = color;
        }
    }
}

static UBYTE shade_wall_texel(UBYTE color, int shade)
{
    if (color == 0) {
        return 0;
    }
    return (UBYTE)(color + shade * 16);
}

static void init_wall_texture_tables(void)
{
    int shade;
    int tex;
    int idx;
    int h;

    if (wall_texture_tables_ready) {
        return;
    }

    for (shade = 0; shade < WALL_SHADE_COUNT; ++shade) {
        for (tex = 0; tex < WALL_TEX_COUNT; ++tex) {
            for (idx = 0; idx < WALL_TEX_SIZE * WALL_TEX_SIZE; ++idx) {
                wall_textures_shaded[shade][tex][idx] =
                    shade_wall_texel(wall_textures[tex][idx], shade);
            }
        }
    }

    wall_tex_step[0] = 0;
    for (h = 1; h <= RENDER_H * 2; ++h) {
        wall_tex_step[h] = (WALL_TEX_SIZE << FIX_SHIFT) / h;
    }

    wall_height_lut[0] = RENDER_H * 2;
    for (idx = 1; idx <= DIST_LUT_MAX; ++idx) {
        fix dist = (fix)idx << DIST_LUT_SHIFT;
        int height;

        if (dist < (FIX_ONE / 8)) {
            dist = FIX_ONE / 8;
        }
        height = (int)((WALL_HEIGHT_SCALE * FIX_ONE) / dist);
        if (height > RENDER_H * 2) {
            height = RENDER_H * 2;
        }
        if (height < 1) {
            height = 1;
        }
        wall_height_lut[idx] = (UWORD)height;
    }

    wall_texture_tables_ready = 1;
}

static void draw_textured_column(int x, int span, int y0, int y1, int raw_y0,
                                 int line_h, int tex_idx, int tex_x, int shade)
{
    int y;
    fix tex_step;
    fix tex_pos;
    const UBYTE *tex;

    if (x < 0 || x >= RENDER_W) return;
    if (x + span > RENDER_W) span = RENDER_W - x;
    if (y0 < 0) y0 = 0;
    if (y1 >= RENDER_H) y1 = RENDER_H - 1;
    if (y0 > y1 || line_h <= 0) return;

    tex = wall_textures_shaded[shade][tex_idx];
    tex_step = wall_tex_step[line_h];
    tex_pos = (y0 - raw_y0) * tex_step;

    for (y = y0; y <= y1; ++y) {
        int tex_y = (int)(tex_pos >> FIX_SHIFT) & (WALL_TEX_SIZE - 1);
        UBYTE color = tex[tex_y * WALL_TEX_SIZE + (tex_x & (WALL_TEX_SIZE - 1))];
        UBYTE *dst = framebuffer + y * RENDER_W + x;
        dst[0] = color;
        if (span > 1) {
            dst[1] = color;
        }
        tex_pos += tex_step;
    }
}

static void draw_entity_sprite(fix cam_x, fix cam_y, int cam_angle, Vec2Fix pos, UBYTE color)
{
    fix dx = pos.x - cam_x;
    fix dy = pos.y - cam_y;
    fix dir_x = cos_fix(cam_angle);
    fix dir_y = sin_fix(cam_angle);
    fix plane_x = -mul_fix(dir_y, FOV_SCALE << 8);
    fix plane_y = mul_fix(dir_x, FOV_SCALE << 8);
    fix depth = mul_fix(dx, dir_x) + mul_fix(dy, dir_y);
    fix side = mul_fix(dx, plane_x) + mul_fix(dy, plane_y);
    int sx;
    int h;
    int x0;
    int x1;
    int y0;
    int y1;
    int x;
    int y;

    if (depth <= (FIX_ONE / 4)) {
        return;
    }

    sx = RENDER_W / 2 + (int)((side * (RENDER_W / 2)) / depth);
    h = (int)((WALL_HEIGHT_SCALE * FIX_ONE) / depth);
    if (h < 3) h = 3;
    if (h > 36) h = 36;

    x0 = sx - h / 4;
    x1 = sx + h / 4;
    y0 = RENDER_H / 2 - h / 2;
    y1 = RENDER_H / 2 + h / 2;

    if (x0 < 0) x0 = 0;
    if (x1 >= RENDER_W) x1 = RENDER_W - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= RENDER_H) y1 = RENDER_H - 1;

    for (y = y0; y <= y1; ++y) {
        for (x = x0; x <= x1; ++x) {
            int cx = x - sx;
            int cy = y - (y0 + y1) / 2;
            if (cx * cx * 4 + cy * cy < h * h / 4) {
                framebuffer[y * RENDER_W + x] = color;
            }
        }
    }
}

static void render_hud(const GameState *game)
{
    int hp_w = (int)game->health;
    int ammo_w = (int)game->ammo;
    int relic_w = (int)game->relics * 7;

    hp_w /= 2;
    if (hp_w > 50) hp_w = 50;
    if (ammo_w > 40) ammo_w = 40;
    if (relic_w > 40) relic_w = 40;

    fill_rect(0, RENDER_H - 18, RENDER_W - 1, RENDER_H - 1, 0);
    fill_rect(4, RENDER_H - 13, 55, RENDER_H - 9, 3);
    fill_rect(4, RENDER_H - 13, 4 + hp_w, RENDER_H - 9, game->hit_flash ? 15 : 12);
    fill_rect(64, RENDER_H - 13, 105, RENDER_H - 9, 3);
    fill_rect(64, RENDER_H - 13, 64 + ammo_w, RENDER_H - 9, 14);
    fill_rect(116, RENDER_H - 13, 157, RENDER_H - 9, 3);
    fill_rect(116, RENDER_H - 13, 116 + relic_w, RENDER_H - 9, 10);
    fill_rect(RENDER_W / 2 - 2, RENDER_H / 2, RENDER_W / 2 + 2, RENDER_H / 2, 15);
    fill_rect(RENDER_W / 2, RENDER_H / 2 - 2, RENDER_W / 2, RENDER_H / 2 + 2, 15);
}

static void render_frame(const GameState *game, fix cam_x, fix cam_y, int cam_angle)
{
    int ray;
    int i;
    int ray_stride = render_half_rays ? 2 : 1;
    int column_span = render_half_rays ? 2 : 1;
    const RaySetup *rays;

    clear_frame();
    init_ray_table();
    init_wall_texture_tables();
    rays = ray_table[cam_angle & 63];

    for (ray = 0; ray < RAY_COUNT; ray += ray_stride) {
        const RaySetup *setup = &rays[ray];
        int x = ray * RENDER_X_STEP;
        int map_x = (int)(cam_x >> FIX_SHIFT);
        int map_y = (int)(cam_y >> FIX_SHIFT);
        fix delta_x = setup->delta_x;
        fix delta_y = setup->delta_y;
        fix side_x;
        fix side_y;
        int step_x = setup->step_x;
        int step_y = setup->step_y;
        int side = 0;
        int hit = 0;
        int wall = 1;
        int guard = 0;
        fix dist;
        fix hit_coord;
        int dist_idx;
        int line_h;
        int draw_start;
        int draw_end;
        int raw_draw_start;
        int tex_idx;
        int tex_x;
        int shade = 0;

        if (step_x < 0) {
            side_x = mul_frac_delta(cam_x - to_fix(map_x), delta_x);
        } else {
            side_x = mul_frac_delta(to_fix(map_x + 1) - cam_x, delta_x);
        }

        if (step_y < 0) {
            side_y = mul_frac_delta(cam_y - to_fix(map_y), delta_y);
        } else {
            side_y = mul_frac_delta(to_fix(map_y + 1) - cam_y, delta_y);
        }

        while (!hit && guard++ < 64) {
            if (side_x < side_y) {
                side_x += delta_x;
                map_x += step_x;
                side = 0;
            } else {
                side_y += delta_y;
                map_y += step_y;
                side = 1;
            }

            wall = map_at(map_x, map_y);
            if (wall > 0) {
                hit = 1;
            }
        }

        dist = side == 0 ? side_x - delta_x : side_y - delta_y;
        if (dist < (FIX_ONE / 8)) {
            dist = FIX_ONE / 8;
        }

        dist_idx = (int)(dist >> DIST_LUT_SHIFT);
        if (dist_idx > DIST_LUT_MAX) {
            dist_idx = DIST_LUT_MAX;
        }
        line_h = wall_height_lut[dist_idx];
        raw_draw_start = RENDER_H / 2 - line_h / 2;
        draw_start = raw_draw_start;
        draw_end = RENDER_H / 2 + line_h / 2;

        tex_idx = wall - 1;
        if (tex_idx < 0) {
            tex_idx = 0;
        }
        tex_idx %= WALL_TEX_COUNT;
        hit_coord = side == 0
            ? cam_y + mul_fix_fast(dist, setup->ray_y)
            : cam_x + mul_fix_fast(dist, setup->ray_x);
        tex_x = (int)(hit_coord >> (FIX_SHIFT - 3)) & (WALL_TEX_SIZE - 1);
        if ((side == 0 && setup->ray_x > 0) || (side == 1 && setup->ray_y < 0)) {
            tex_x = (WALL_TEX_SIZE - 1) - tex_x;
        }
        if (dist > (FIX_ONE * 6)) {
            shade = 3;
        } else if (dist > (FIX_ONE * 3)) {
            shade = 2;
        } else if (side) {
            shade = 1;
        }

        draw_textured_column(x, column_span, draw_start, draw_end, raw_draw_start, line_h,
                             tex_idx, tex_x, shade);
    }

    for (i = 0; i < MAX_ITEMS; ++i) {
        if (game->items[i].active) {
            draw_entity_sprite(cam_x, cam_y, cam_angle, game->items[i].pos,
                               game->items[i].type ? 13 : 14);
        }
    }

    for (i = 0; i < MAX_MONSTERS; ++i) {
        if (game->monsters[i].active) {
            draw_entity_sprite(cam_x, cam_y, cam_angle, game->monsters[i].pos,
                               game->monsters[i].flash ? 15 : 12);
        }
    }

    render_hud(game);
}

static void setup_palette(struct Screen *screen)
{
    struct ViewPort *vp = &screen->ViewPort;
    int i;
    int shade;

    static const UBYTE palette[16][3] = {
        {0, 0, 0},
        {12, 16, 28},
        {28, 34, 52},
        {46, 46, 58},
        {32, 22, 14},
        {54, 34, 18},
        {78, 48, 20},
        {108, 66, 26},
        {22, 38, 30},
        {34, 58, 42},
        {52, 82, 58},
        {78, 110, 80},
        {92, 40, 28},
        {138, 64, 34},
        {186, 100, 48},
        {230, 178, 92},
    };
    static const UBYTE shade_mul[WALL_SHADE_COUNT] = {255, 190, 128, 78};

    for (shade = 0; shade < WALL_SHADE_COUNT; ++shade) {
        for (i = 0; i < 16; ++i) {
            ULONG r = ((ULONG)palette[i][0] * shade_mul[shade]) >> 8;
            ULONG g = ((ULONG)palette[i][1] * shade_mul[shade]) >> 8;
            ULONG b = ((ULONG)palette[i][2] * shade_mul[shade]) >> 8;
            int idx = shade * 16 + i;
            rtg_palette_rgb[idx][0] = (UBYTE)r;
            rtg_palette_rgb[idx][1] = (UBYTE)g;
            rtg_palette_rgb[idx][2] = (UBYTE)b;
            if (rtg_rgb_format == RGBFB_CLUT) {
                SetRGB32(vp, (ULONG)idx,
                         r * 0x01010101UL,
                         g * 0x01010101UL,
                         b * 0x01010101UL);
            }
        }
    }

    for (i = 64; i < 256; ++i) {
        ULONG r = ((ULONG)((i >> 5) & 7) * 255UL) / 7UL;
        ULONG g = ((ULONG)((i >> 2) & 7) * 255UL) / 7UL;
        ULONG b = ((ULONG)(i & 3) * 255UL) / 3UL;
        rtg_palette_rgb[i][0] = (UBYTE)r;
        rtg_palette_rgb[i][1] = (UBYTE)g;
        rtg_palette_rgb[i][2] = (UBYTE)b;
        if (rtg_rgb_format == RGBFB_CLUT) {
            SetRGB32(vp, (ULONG)i, r * 0x01010101UL, g * 0x01010101UL, b * 0x01010101UL);
        }
    }
}

static void init_rtg_tables(void)
{
    int i;

    for (i = 0; i < 256; ++i) {
        rtg_dupword[i] = (UWORD)(((UWORD)i << 8) | (UWORD)i);
    }
    for (i = 0; i < 65536; ++i) {
        ULONG a = (ULONG)((i >> 8) & 255);
        ULONG b = (ULONG)(i & 255);
        rtg_duppair[i] = (a << 24) | (a << 16) | (b << 8) | b;
    }
}

static void expand_frame_to_buffer(UBYTE *dst_base, WORD bytes_per_row)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        UBYTE *dst0 = dst_base + (y * RENDER_SCALE) * bytes_per_row;
        UBYTE *dst1 = dst0 + bytes_per_row;
        int x;

        for (x = 0; x < RENDER_W; ++x) {
            UBYTE c = src[x];
            int sx = x * RENDER_SCALE;
            dst0[sx] = c;
            dst0[sx + 1] = c;
            dst1[sx] = c;
            dst1[sx + 1] = c;
        }
    }
}

static void expand_frame_to_aligned_words(UBYTE *dst_base, WORD bytes_per_row)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        UWORD *dst0 = (UWORD *)(dst_base + (y * RENDER_SCALE) * bytes_per_row);
        UWORD *dst1 = (UWORD *)((UBYTE *)dst0 + bytes_per_row);
        int x;

        for (x = 0; x < RENDER_W; ++x) {
            UWORD cc = rtg_dupword[src[x]];
            dst0[x] = cc;
            dst1[x] = cc;
        }
    }
}

static void expand_frame_to_aligned_longs(UBYTE *dst_base, WORD bytes_per_row)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        ULONG *dst0 = (ULONG *)(dst_base + (y * RENDER_SCALE) * bytes_per_row);
        ULONG *dst1 = (ULONG *)((UBYTE *)dst0 + bytes_per_row);
        int x;

        for (x = 0; x < RENDER_W; x += 2) {
            dst0[x >> 1] = rtg_duppair[((UWORD)src[x] << 8) | src[x + 1]];
        }
        memcpy(dst1, dst0, SCREEN_W);
    }
}

static void expand_frame_to_staging(void)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        ULONG *dst0 = (ULONG *)(rtg_framebuffer + (y * RENDER_SCALE) * SCREEN_W);
        UBYTE *dst1 = (UBYTE *)dst0 + SCREEN_W;
        int x;

        for (x = 0; x < RENDER_W; x += 2) {
            dst0[x >> 1] = rtg_duppair[((UWORD)src[x] << 8) | src[x + 1]];
        }
        memcpy(dst1, dst0, SCREEN_W);
    }
}

static void expand_frame_to_rgb24(void)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        UBYTE *dst0 = rtg_framebuffer24 + (y * RENDER_SCALE) * SCREEN_W * 3;
        UBYTE *dst1 = dst0 + SCREEN_W * 3;
        int x;

        for (x = 0; x < RENDER_W; ++x) {
            const UBYTE *rgb = rtg_palette_rgb[src[x]];
            UBYTE *d = dst0 + x * 6;
            if (rtg_rgb_format == RGBFB_B8G8R8) {
                d[0] = rgb[2];
                d[1] = rgb[1];
                d[2] = rgb[0];
                d[3] = rgb[2];
                d[4] = rgb[1];
                d[5] = rgb[0];
            } else {
                d[0] = rgb[0];
                d[1] = rgb[1];
                d[2] = rgb[2];
                d[3] = rgb[0];
                d[4] = rgb[1];
                d[5] = rgb[2];
            }
        }
        memcpy(dst1, dst0, SCREEN_W * 3);
    }
}

static void expand_frame_to_rgb16(void)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        UBYTE *dst0 = rtg_framebuffer16 + (y * RENDER_SCALE) * SCREEN_W * 2;
        UBYTE *dst1 = dst0 + SCREEN_W * 2;
        int x;

        for (x = 0; x < RENDER_W; ++x) {
            const UBYTE *rgb = rtg_palette_rgb[src[x]];
            UWORD value;
            int sx = x * RENDER_SCALE;
            UBYTE *d = dst0 + sx * 2;
            UBYTE r = rgb[0];
            UBYTE g = rgb[1];
            UBYTE b = rgb[2];
            int pc_order = 0;

            switch (rtg_rgb_format) {
                case RGBFB_R5G5B5:
                    value = (UWORD)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
                    break;
                case RGBFB_R5G5B5PC:
                    value = (UWORD)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
                    pc_order = 1;
                    break;
                case RGBFB_B5G6R5PC:
                    value = (UWORD)(((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3));
                    pc_order = 1;
                    break;
                case RGBFB_B5G5R5PC:
                    value = (UWORD)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
                    pc_order = 1;
                    break;
                case RGBFB_R5G6B5PC:
                    value = (UWORD)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                    pc_order = 1;
                    break;
                case RGBFB_R5G6B5:
                default:
                    value = (UWORD)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                    break;
            }

            if (pc_order) {
                d[0] = (UBYTE)value;
                d[1] = (UBYTE)(value >> 8);
            } else {
                d[0] = (UBYTE)(value >> 8);
                d[1] = (UBYTE)value;
            }

            d[2] = d[0];
            d[3] = d[1];
        }
        memcpy(dst1, dst0, SCREEN_W * 2);
    }
}

static void expand_frame_to_rgb32(void)
{
    int y;

    for (y = 0; y < RENDER_H; ++y) {
        const UBYTE *src = framebuffer + y * RENDER_W;
        UBYTE *dst0 = rtg_framebuffer32 + (y * RENDER_SCALE) * SCREEN_W * 4;
        UBYTE *dst1 = dst0 + SCREEN_W * 4;
        int x;

        for (x = 0; x < RENDER_W; ++x) {
            const UBYTE *rgb = rtg_palette_rgb[src[x]];
            int sx = x * RENDER_SCALE;
            UBYTE *d = dst0 + sx * 4;

            switch (rtg_rgb_format) {
                case RGBFB_A8B8G8R8:
                    d[0] = 0;
                    d[1] = rgb[2];
                    d[2] = rgb[1];
                    d[3] = rgb[0];
                    break;
                case RGBFB_R8G8B8A8:
                    d[0] = rgb[0];
                    d[1] = rgb[1];
                    d[2] = rgb[2];
                    d[3] = 0;
                    break;
                case RGBFB_B8G8R8A8:
                    d[0] = rgb[2];
                    d[1] = rgb[1];
                    d[2] = rgb[0];
                    d[3] = 0;
                    break;
                case RGBFB_A8R8G8B8:
                default:
                    d[0] = 0;
                    d[1] = rgb[0];
                    d[2] = rgb[1];
                    d[3] = rgb[2];
                    break;
            }

            d[4] = d[0];
            d[5] = d[1];
            d[6] = d[2];
            d[7] = d[3];
        }
        memcpy(dst1, dst0, SCREEN_W * 4);
    }
}

static void present_rtg_frame(struct Screen *screen, struct RastPort *fallback_rp)
{
    struct RenderInfo ri;
    LONG lock;

    if (rtg_use_direct && rtg_color_mode == 0) {
        lock = p96LockBitMap(screen->RastPort.BitMap, (UBYTE *)&ri, sizeof(ri));
        if (lock) {
            if (ri.Memory &&
                ri.RGBFormat == RGBFB_CLUT &&
                ri.BytesPerRow >= (WORD)(rtg_dest_x + SCREEN_W)) {
                UBYTE *dst = (UBYTE *)ri.Memory + (rtg_dest_y * ri.BytesPerRow) + rtg_dest_x;

                if ((((ULONG)dst | (ULONG)ri.BytesPerRow) & 3UL) == 0) {
                    expand_frame_to_aligned_longs(dst, ri.BytesPerRow);
                } else if ((((ULONG)dst | (ULONG)ri.BytesPerRow) & 1UL) == 0) {
                    expand_frame_to_aligned_words(dst, ri.BytesPerRow);
                } else {
                    expand_frame_to_buffer(dst, ri.BytesPerRow);
                }
                if (!rtg_direct_logged) {
                    printf("RTG direct locked bitmap path: bpr=%ld format=%ld\n",
                           (long)ri.BytesPerRow,
                           (long)ri.RGBFormat);
                    rtg_direct_logged = 1;
                }
                p96UnlockBitMap(screen->RastPort.BitMap, lock);
                return;
            }

            if (!rtg_fallback_logged) {
                printf("warning: RTG direct path unavailable: mem=%lx bpr=%ld format=%ld, using p96WritePixelArray fallback.\n",
                       (unsigned long)ri.Memory,
                       (long)ri.BytesPerRow,
                       (long)ri.RGBFormat);
                rtg_fallback_logged = 1;
            }
            p96UnlockBitMap(screen->RastPort.BitMap, lock);
        } else if (!rtg_fallback_logged) {
            printf("warning: p96LockBitMap failed, using p96WritePixelArray fallback.\n");
            rtg_fallback_logged = 1;
        }
    }

    if (rtg_present_1x) {
        ri.Memory = framebuffer;
        ri.BytesPerRow = RENDER_W;
        ri.pad = 0;
        ri.RGBFormat = RGBFB_CLUT;
        p96WritePixelArray(&ri, 0, 0, fallback_rp,
                           rtg_dest_x + ((SCREEN_W - RENDER_W) / 2),
                           rtg_dest_y + ((SCREEN_H - RENDER_H) / 2),
                           RENDER_W,
                           RENDER_H);
        return;
    }

    if (rtg_color_mode == 1) {
        expand_frame_to_rgb24();
        ri.Memory = rtg_framebuffer24;
        ri.BytesPerRow = SCREEN_W * 3;
        ri.pad = 0;
        ri.RGBFormat = rtg_rgb_format;
        p96WritePixelArray(&ri, 0, 0, fallback_rp, rtg_dest_x, rtg_dest_y, SCREEN_W, SCREEN_H);
        return;
    }

    if (rtg_color_mode == 3) {
        expand_frame_to_rgb16();
        ri.Memory = rtg_framebuffer16;
        ri.BytesPerRow = SCREEN_W * 2;
        ri.pad = 0;
        ri.RGBFormat = rtg_rgb_format;
        p96WritePixelArray(&ri, 0, 0, fallback_rp, rtg_dest_x, rtg_dest_y, SCREEN_W, SCREEN_H);
        return;
    }

    if (rtg_color_mode == 2) {
        expand_frame_to_rgb32();
        ri.Memory = rtg_framebuffer32;
        ri.BytesPerRow = SCREEN_W * 4;
        ri.pad = 0;
        ri.RGBFormat = rtg_rgb_format;
        p96WritePixelArray(&ri, 0, 0, fallback_rp, rtg_dest_x, rtg_dest_y, SCREEN_W, SCREEN_H);
        return;
    }

    expand_frame_to_staging();

    ri.Memory = rtg_framebuffer;
    ri.BytesPerRow = SCREEN_W;
    ri.pad = 0;
    ri.RGBFormat = RGBFB_CLUT;
    p96WritePixelArray(&ri, 0, 0, fallback_rp, rtg_dest_x, rtg_dest_y, SCREEN_W, SCREEN_H);
}

static ULONG micros_since(ULONG sec0, ULONG micro0, ULONG sec1, ULONG micro1)
{
    ULONG ds = sec1 - sec0;

    if (micro1 >= micro0) {
        return ds * 1000000UL + (micro1 - micro0);
    }

    return (ds - 1) * 1000000UL + (1000000UL - micro0 + micro1);
}

static const char *rtg_format_family_name(ULONG mask)
{
    if (mask == RGBFF_CLUT) {
        return "CLUT";
    }
    if (mask == RGBFF_TRUECOLOR) {
        return "TRUECOLOR24";
    }
    if (mask == RGBFF_TRUEALPHA) {
        return "TRUEALPHA32";
    }
    if (mask == RGBFF_HICOLOR) {
        return "HICOLOR";
    }
    return "custom";
}

static void probe_rtg_modes(void)
{
    static const UWORD probe_depths[] = {8, 15, 16, 24, 32};
    static const ULONG probe_masks[] = {
        RGBFF_CLUT,
        RGBFF_HICOLOR,
        RGBFF_TRUECOLOR,
        RGBFF_TRUEALPHA
    };
    int d;
    int m;

    printf("RTG mode probe for 320x200 and 640x480:\n");
    for (m = 0; m < (int)(sizeof(probe_masks) / sizeof(probe_masks[0])); ++m) {
        for (d = 0; d < (int)(sizeof(probe_depths) / sizeof(probe_depths[0])); ++d) {
            ULONG id320 = p96BestModeIDTags(
                P96BIDTAG_NominalWidth, 320,
                P96BIDTAG_NominalHeight, 200,
                P96BIDTAG_Depth, probe_depths[d],
                P96BIDTAG_FormatsAllowed, probe_masks[m],
                TAG_DONE);
            ULONG id640 = p96BestModeIDTags(
                P96BIDTAG_NominalWidth, 640,
                P96BIDTAG_NominalHeight, 480,
                P96BIDTAG_Depth, probe_depths[d],
                P96BIDTAG_FormatsAllowed, probe_masks[m],
                TAG_DONE);

            if (id320 != (ULONG)INVALID_ID || id640 != (ULONG)INVALID_ID) {
                printf("  depth=%u family=%s mask=0x%08lx id320=0x%08lx id640=0x%08lx\n",
                       probe_depths[d],
                       rtg_format_family_name(probe_masks[m]),
                       (unsigned long)probe_masks[m],
                       (unsigned long)id320,
                       (unsigned long)id640);
            }
        }
    }
}

static void try_move(fix *cam_x, fix *cam_y, int angle, int dir)
{
    fix dx = mul_fix(cos_fix(angle), MOVE_STEP * dir);
    fix dy = mul_fix(sin_fix(angle), MOVE_STEP * dir);
    fix nx = *cam_x + dx;
    fix ny = *cam_y + dy;

    if (!map_at((int)(nx >> FIX_SHIFT), (int)(*cam_y >> FIX_SHIFT))) {
        *cam_x = nx;
    }
    if (!map_at((int)(*cam_x >> FIX_SHIFT), (int)(ny >> FIX_SHIFT))) {
        *cam_y = ny;
    }
}

static void update_game(GameState *game, fix cam_x, fix cam_y)
{
    int i;

    if (game->shoot_cooldown) {
        --game->shoot_cooldown;
    }
    if (game->hit_flash) {
        --game->hit_flash;
    }

    for (i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }
        if (abs_fix(item->pos.x - cam_x) < FIX_ONE / 2 &&
            abs_fix(item->pos.y - cam_y) < FIX_ONE / 2) {
            item->active = 0;
            if (item->type) {
                UWORD hp = game->health + 24;
                game->health = hp > PLAYER_MAX_HEALTH ? PLAYER_MAX_HEALTH : (UBYTE)hp;
            } else {
                UWORD ammo = game->ammo + 8;
                game->ammo = ammo > 40 ? 40 : (UBYTE)ammo;
            }
        }
    }

    for (i = 0; i < MAX_MONSTERS; ++i) {
        Monster *monster = &game->monsters[i];
        fix dx;
        fix dy;
        fix step_x = 0;
        fix step_y = 0;
        fix nx;
        fix ny;

        if (!monster->active) {
            continue;
        }
        if (monster->flash) {
            --monster->flash;
        }

        dx = cam_x - monster->pos.x;
        dy = cam_y - monster->pos.y;
        if (abs_fix(dx) < FIX_ONE && abs_fix(dy) < FIX_ONE) {
            if (!game->hit_flash && game->health > 0) {
                game->health = game->health > 5 ? game->health - 5 : 0;
                game->hit_flash = 12;
            }
            continue;
        }

        if (abs_fix(dx) < FIX_ONE * 7 && abs_fix(dy) < FIX_ONE * 7) {
            if (abs_fix(dx) > abs_fix(dy)) {
                step_x = dx > 0 ? 0x0200L : -0x0200L;
            } else {
                step_y = dy > 0 ? 0x0200L : -0x0200L;
            }
            nx = monster->pos.x + step_x;
            ny = monster->pos.y + step_y;
            if (!map_at((int)(nx >> FIX_SHIFT), (int)(ny >> FIX_SHIFT))) {
                monster->pos.x = nx;
                monster->pos.y = ny;
            }
        }
    }
}

static void shoot(GameState *game, fix cam_x, fix cam_y, int cam_angle)
{
    int i;
    int best = -1;
    fix best_depth = FIX_ONE * 12;
    fix dir_x = cos_fix(cam_angle);
    fix dir_y = sin_fix(cam_angle);

    if (game->shoot_cooldown || game->ammo == 0) {
        return;
    }

    --game->ammo;
    game->shoot_cooldown = 10;

    for (i = 0; i < MAX_MONSTERS; ++i) {
        Monster *monster = &game->monsters[i];
        fix dx;
        fix dy;
        fix depth;
        fix side;

        if (!monster->active) {
            continue;
        }

        dx = monster->pos.x - cam_x;
        dy = monster->pos.y - cam_y;
        depth = mul_fix(dx, dir_x) + mul_fix(dy, dir_y);
        side = mul_fix(dx, -dir_y) + mul_fix(dy, dir_x);

        if (depth > FIX_ONE / 2 && depth < best_depth && abs_fix(side) < FIX_ONE / 2) {
            best = i;
            best_depth = depth;
        }
    }

    if (best >= 0) {
        Monster *monster = &game->monsters[best];
        if (monster->hp <= 12) {
            monster->active = 0;
            ++game->relics;
        } else {
            monster->hp -= 12;
            monster->flash = 5;
        }
    }
}

static int open_demo_screen(struct Screen **out_screen)
{
    static const UWORD mode_sizes[][2] = {
        {320, 200},
        {320, 240},
        {640, 480},
        {800, 600},
        {1024, 768}
    };
    UWORD depth_options[2];
    int depth_count = 1;
    ULONG display_id;
    ULONG formats_allowed = RGBFF_CLUT;
    UWORD depth = 8;
    RGBFTYPE rgb_format;
    const char *board_name;
    UWORD mode_w = 0;
    UWORD mode_h = 0;
    int i;

    if (rtg_color_mode == 1) {
        formats_allowed = RGBFF_TRUECOLOR;
        depth = 24;
    } else if (rtg_color_mode == 2) {
        formats_allowed = RGBFF_TRUEALPHA;
        depth = 32;
        depth_options[1] = 24;
        depth_count = 2;
    } else if (rtg_color_mode == 3) {
        formats_allowed = RGBFF_HICOLOR;
        depth = 16;
        depth_options[1] = 15;
        depth_count = 2;
    }
    depth_options[0] = depth;

    display_id = (ULONG)INVALID_ID;
    for (i = 0; i < depth_count; ++i) {
        int size_i;

        depth = depth_options[i];
        for (size_i = 0; size_i < (int)(sizeof(mode_sizes) / sizeof(mode_sizes[0])); ++size_i) {
            display_id = p96BestModeIDTags(
                P96BIDTAG_NominalWidth, mode_sizes[size_i][0],
                P96BIDTAG_NominalHeight, mode_sizes[size_i][1],
                P96BIDTAG_Depth, depth,
                P96BIDTAG_FormatsAllowed, formats_allowed,
                TAG_DONE);
            if (display_id != (ULONG)INVALID_ID) {
                mode_w = mode_sizes[size_i][0];
                mode_h = mode_sizes[size_i][1];
                break;
            }
        }
        if (display_id != (ULONG)INVALID_ID) {
            break;
        }
    }

    if (display_id == (ULONG)INVALID_ID) {
        printf("error: p96BestModeIDTags failed for requested RTG mode, format family %s mask 0x%08lx.\n",
               rtg_format_family_name(formats_allowed),
               (unsigned long)formats_allowed);
        probe_rtg_modes();
        return 0;
    }

    rtg_screen_w = mode_w;
    rtg_screen_h = mode_h;
    rtg_dest_x = mode_w > SCREEN_W ? (mode_w - SCREEN_W) / 2 : 0;
    rtg_dest_y = mode_h > SCREEN_H ? (mode_h - SCREEN_H) / 2 : 0;
    board_name = (const char *)p96GetModeIDAttr(display_id, P96IDA_BOARDNAME);
    if (board_name) {
        printf("P96 board name: %s\n", board_name);
    }
    rtg_rgb_format = (RGBFTYPE)p96GetModeIDAttr(display_id, P96IDA_RGBFORMAT);
    if (rtg_rgb_format == RGBFB_CLUT) {
        *out_screen = p96OpenScreenTags(
            P96SA_DisplayID, display_id,
            P96SA_Width, mode_w,
            P96SA_Height, mode_h,
            P96SA_Depth, depth,
            P96SA_RGBFormat, rtg_rgb_format,
            P96SA_Type, CUSTOMSCREEN,
            P96SA_Title, (ULONG)"DIOOM 060 RTG",
            P96SA_Quiet, TRUE,
            P96SA_AutoScroll, FALSE,
            P96SA_Pens, (ULONG)p96_pens,
            TAG_DONE);
    } else {
        *out_screen = p96OpenScreenTags(
            P96SA_DisplayID, display_id,
            P96SA_Width, mode_w,
            P96SA_Height, mode_h,
            P96SA_Depth, depth,
            P96SA_RGBFormat, rtg_rgb_format,
            P96SA_Type, CUSTOMSCREEN,
            P96SA_Title, (ULONG)"DIOOM 060 RTG",
            P96SA_Quiet, TRUE,
            P96SA_AutoScroll, FALSE,
            TAG_DONE);
    }

    if (!*out_screen) {
        printf("error: p96OpenScreenTags failed for %ux%ux%u RTG screen.\n", mode_w, mode_h, depth);
        return 0;
    }

    rgb_format = (RGBFTYPE)p96GetBitMapAttr((*out_screen)->RastPort.BitMap, P96BMA_RGBFORMAT);
    if (rgb_format != rtg_rgb_format) {
        printf("error: RTG screen opened with RGB format %ld, expected %ld.\n",
               (long)rgb_format,
               (long)rtg_rgb_format);
        p96CloseScreen(*out_screen);
        *out_screen = NULL;
        return 0;
    }

    printf("opened P96 RTG %ux%ux%u format=%ld display id 0x%08lx, render %dx%d at %u,%u\n",
           mode_w,
           mode_h,
           depth,
           (long)rtg_rgb_format,
           (unsigned long)display_id,
           SCREEN_W,
           SCREEN_H,
           rtg_dest_x,
           rtg_dest_y);
    return 1;
}

static struct Window *open_demo_window(struct Screen *screen)
{
    struct Window *window = OpenWindowTags(
        NULL,
        WA_CustomScreen, (ULONG)screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, rtg_screen_w,
        WA_Height, rtg_screen_h,
        WA_Title, (ULONG)"DIOOM 060",
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, FALSE,
        WA_Backdrop, TRUE,
        WA_Borderless, TRUE,
        WA_SizeGadget, FALSE,
        WA_Activate, TRUE,
        WA_RMBTrap, TRUE,
        WA_IDCMP, IDCMP_RAWKEY | IDCMP_CLOSEWINDOW,
        TAG_DONE);

    if (!window) {
        printf("error: OpenWindowTags failed for %ux%u demo window.\n", rtg_screen_w, rtg_screen_h);
    }

    return window;
}

static void update_key_state(UBYTE keys[128], UWORD raw_code, int *running)
{
    UBYTE code = raw_code & 0x7f;
    int released = (raw_code & 0x80) != 0;

    if (code == RAWKEY_ESC && !released) {
        *running = 0;
        return;
    }

    keys[code] = released ? 0 : 1;
}

int main(int argc, char **argv)
{
    struct Screen *screen = NULL;
    struct Window *window = NULL;
    fix cam_x = to_fix(3) + FIX_ONE / 2;
    fix cam_y = to_fix(3) + FIX_ONE / 2;
    int cam_angle = 0;
    GameState game;
    ULONG fps_sec;
    ULONG fps_micro;
    ULONG frames = 0;
    ULONG fps_frames = 0;
    ULONG prof_render_us = 0;
    ULONG prof_present_us = 0;
    UBYTE keys[128];
    int running = 1;
    int arg;

    printf("DIOOM 060 tech demo starting.\n");
    for (arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "DIRECT") == 0) {
            rtg_use_direct = 1;
        } else if (strcmp(argv[arg], "NODIRECT") == 0) {
            rtg_no_direct = 1;
            rtg_use_direct = 0;
        } else if (strcmp(argv[arg], "VSYNC") == 0) {
            rtg_use_vsync = 1;
        } else if (strcmp(argv[arg], "PROF") == 0) {
            rtg_use_prof = 1;
        } else if (strcmp(argv[arg], "NOPROF") == 0) {
            rtg_use_prof = 0;
        } else if (strcmp(argv[arg], "NOPRESENT") == 0) {
            rtg_no_present = 1;
        } else if (strcmp(argv[arg], "PRESENT1X") == 0) {
            rtg_present_1x = 1;
        } else if (strcmp(argv[arg], "RAYS80") == 0) {
            render_half_rays = 1;
        } else if (strcmp(argv[arg], "RAYS160") == 0) {
            render_half_rays = 0;
        } else if (strcmp(argv[arg], "RGB24") == 0) {
            rtg_color_mode = 1;
            rtg_present_1x = 0;
        } else if (strcmp(argv[arg], "RGB32") == 0) {
            rtg_color_mode = 2;
            rtg_present_1x = 0;
        } else if (strcmp(argv[arg], "RGB16") == 0) {
            rtg_color_mode = 3;
            rtg_present_1x = 0;
        }
    }
    if (rtg_use_direct) {
        printf("RTG direct locked bitmap mode enabled.\n");
    } else {
        printf("RTG safe mode: p96WritePixelArray. Run with DIRECT to test locked VRAM writes.\n");
    }
    if (rtg_use_vsync) {
        printf("RTG VSYNC mode enabled.\n");
    } else {
        printf("RTG uncapped mode: no WaitTOF. Run with VSYNC to cap to display refresh.\n");
    }
    if (rtg_use_prof) {
        printf("RTG profiling enabled.\n");
    }
    if (rtg_no_present) {
        printf("RTG present disabled for renderer-only profiling.\n");
    } else if (rtg_present_1x) {
        printf("RTG 1x present mode: copying 160x100 without 2x scale.\n");
    }
    if (rtg_color_mode == 1) {
        printf("RTG RGB24 mode: source follows selected P96 24-bit format.\n");
    } else if (rtg_color_mode == 2) {
        printf("RTG RGB32 mode: source follows selected P96 32-bit format.\n");
    } else if (rtg_color_mode == 3) {
        printf("RTG RGB16 mode: source follows selected P96 HiColor format.\n");
    } else {
        printf("RTG CLUT8 mode: source RGBFB_CLUT.\n");
    }
    if (render_half_rays) {
        printf("Render RAYS80 mode: 80 ray columns expanded to 160 logical columns.\n");
    }
    memset(keys, 0, sizeof(keys));
    init_rtg_tables();
    init_game(&game);

    P96Base = (struct Library *)OpenLibrary((CONST_STRPTR)P96NAME, 2);
    if (!P96Base) {
        printf("error: cannot open %s v2+. Install Picasso96/P96 RTG libraries.\n", P96NAME);
        return RETURN_FAIL;
    }

    if (!open_demo_screen(&screen)) {
        CloseLibrary(P96Base);
        return RETURN_FAIL;
    }
    setup_palette(screen);

    window = open_demo_window(screen);
    if (!window) {
        p96CloseScreen(screen);
        CloseLibrary(P96Base);
        return RETURN_FAIL;
    }
    printf("press Esc or wait for auto-exit after %d frames\n", AUTO_EXIT_FRAMES);

    CurrentTime(&fps_sec, &fps_micro);

    while (running) {
        ULONG now_sec;
        ULONG now_micro;
        ULONG render_start_sec = 0;
        ULONG render_start_micro = 0;
        ULONG present_start_sec = 0;
        ULONG present_start_micro = 0;
        ULONG present_end_sec = 0;
        ULONG present_end_micro = 0;
        struct IntuiMessage *msg;

        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)) != NULL) {
            ULONG cls = msg->Class;
            UWORD code = msg->Code;
            ReplyMsg((struct Message *)msg);

            if (cls == IDCMP_CLOSEWINDOW) {
                running = 0;
            } else if (cls == IDCMP_RAWKEY) {
                update_key_state(keys, code, &running);
            }
        }

        if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
            running = 0;
        }

        if (keys[RAWKEY_A] || keys[RAWKEY_LEFT]) {
            cam_angle = (cam_angle - TURN_STEP) & 63;
        }
        if (keys[RAWKEY_D] || keys[RAWKEY_RIGHT]) {
            cam_angle = (cam_angle + TURN_STEP) & 63;
        }
        if (keys[RAWKEY_W] || keys[RAWKEY_UP]) {
            try_move(&cam_x, &cam_y, cam_angle, 1);
        }
        if (keys[RAWKEY_S] || keys[RAWKEY_DOWN]) {
            try_move(&cam_x, &cam_y, cam_angle, -1);
        }
        if (keys[RAWKEY_SPACE]) {
            shoot(&game, cam_x, cam_y, cam_angle);
            keys[RAWKEY_SPACE] = 0;
        }

        if (rtg_use_prof) {
            CurrentTime(&render_start_sec, &render_start_micro);
        }
        update_game(&game, cam_x, cam_y);
        render_frame(&game, cam_x, cam_y, cam_angle);
        if (rtg_use_prof) {
            CurrentTime(&present_start_sec, &present_start_micro);
            prof_render_us += micros_since(render_start_sec, render_start_micro,
                                           present_start_sec, present_start_micro);
        }
        if (!rtg_no_present) {
            present_rtg_frame(screen, window->RPort);
        }
        if (rtg_use_prof) {
            CurrentTime(&present_end_sec, &present_end_micro);
            prof_present_us += micros_since(present_start_sec, present_start_micro,
                                            present_end_sec, present_end_micro);
        }
        if (rtg_use_vsync) {
            WaitTOF();
        }

        ++frames;
        ++fps_frames;
        CurrentTime(&now_sec, &now_micro);
        if (micros_since(fps_sec, fps_micro, now_sec, now_micro) >= 2000000UL) {
            ULONG elapsed = micros_since(fps_sec, fps_micro, now_sec, now_micro);
            ULONG fps = elapsed ? (fps_frames * 1000000UL) / elapsed : 0;
            if (rtg_use_prof && fps_frames) {
                printf("fps=%lu frames=%lu render_us=%lu present_us=%lu pos=%ld,%ld angle=%d\n",
                       fps,
                       frames,
                       prof_render_us / fps_frames,
                       prof_present_us / fps_frames,
                       (long)(cam_x >> FIX_SHIFT),
                       (long)(cam_y >> FIX_SHIFT),
                       cam_angle);
                prof_render_us = 0;
                prof_present_us = 0;
            } else {
                printf("fps=%lu frames=%lu pos=%ld,%ld angle=%d\n",
                       fps,
                       frames,
                       (long)(cam_x >> FIX_SHIFT),
                       (long)(cam_y >> FIX_SHIFT),
                       cam_angle);
            }
            fps_sec = now_sec;
            fps_micro = now_micro;
            fps_frames = 0;
        }

        if (AUTO_EXIT_FRAMES > 0 && frames > AUTO_EXIT_FRAMES) {
            running = 0;
        }
    }

    if (window) {
        CloseWindow(window);
    }
    if (screen) {
        p96CloseScreen(screen);
    }
    if (P96Base) {
        CloseLibrary(P96Base);
    }
    printf("DIOOM 060 tech demo stopped after %lu frames.\n", frames);
    return RETURN_OK;
}
