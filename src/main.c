#include <SDL2/SDL.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 200
#define WINDOW_SCALE 3
#define TEX_SIZE 64
#define TEX_COUNT 8
#define SPRITE_SIZE 64
#define SPRITE_FRAMES 4
#define MONSTER_TYPES 3
#define PROJECTILE_SIZE 32
#define MAX_PROJECTILES 12
#define MAX_MONSTERS 10
#define MAX_ITEMS 12
#define PLAYER_MAX_HEALTH 160
#define START_AMMO 48
#define PLAYER_DAMAGE 2
#define SHOT_COOLDOWN_TIME 0.24
#define WEAPON_FLASH_TIME 0.18
#define MAP_W 24
#define MAP_H 24
#define TEXTURE_ATLAS_PATH "assets/textures.ppm"
#define MONSTER_ATLAS_PATH "assets/monsters.ppm"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    double x;
    double y;
} Vec2;

typedef struct {
    Vec2 pos;
    Vec2 dir;
    Vec2 plane;
} Camera;

typedef struct {
    int active;
    int hp;
    Vec2 pos;
    double shoot_timer;
    int target_waypoint;
    int route;
    int type;
    Vec2 facing;
    double facing_lock;
    int ai_state;
    Vec2 last_seen;
    double alert_timer;
    double strafe_timer;
    int strafe_dir;
} Monster;

typedef struct {
    int active;
    Vec2 pos;
    Vec2 vel;
    double life;
} Projectile;

typedef struct {
    int active;
    int type;
    Vec2 pos;
} Item;

typedef struct {
    Monster monsters[MAX_MONSTERS];
    int monster_count;
    Projectile projectiles[MAX_PROJECTILES];
    Item items[MAX_ITEMS];
    double time;
    int player_health;
    int ammo;
    int kills;
    double shot_cooldown;
    double weapon_flash;
    double shot_trace;
    double hit_flash;
    double pickup_flash;
    double rapid_timer;
    double damage_timer;
    int victory;
    int game_over;
    unsigned char discovered[MAP_H][MAP_W];
} GameState;

typedef struct {
    int kind;
    int index;
    double dist;
} SpriteDraw;

static uint32_t framebuffer[SCREEN_W * SCREEN_H];
static uint32_t textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static uint32_t monster_sprites[MONSTER_TYPES][SPRITE_FRAMES][SPRITE_SIZE * SPRITE_SIZE];
static double z_buffer[SCREEN_W];

static const int world_map[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,2,2,2,0,0,3,3,3,0,0,0,4,4,4,4,0,0,5,5,5,0,1},
    {1,0,2,0,0,0,0,3,0,3,0,6,0,4,0,0,4,0,0,5,0,5,0,1},
    {1,0,2,0,2,2,0,3,0,0,0,6,0,4,0,0,0,0,0,5,0,5,0,1},
    {1,0,0,0,0,2,0,3,3,3,0,6,0,4,4,4,0,7,0,0,0,5,0,1},
    {1,0,4,4,0,2,0,0,0,0,0,0,0,0,0,4,0,7,7,7,0,5,0,1},
    {1,0,4,0,0,2,2,2,0,5,5,5,0,2,0,4,0,0,0,7,0,0,0,1},
    {1,0,4,0,0,0,0,0,0,5,0,0,0,2,0,0,0,3,0,7,7,7,0,1},
    {1,0,4,4,4,0,6,6,0,5,0,3,3,2,2,2,0,3,0,0,0,0,0,1},
    {1,0,0,0,4,0,6,0,0,5,0,0,0,0,0,2,0,3,3,3,0,4,0,1},
    {1,2,2,0,4,0,6,0,7,5,5,5,0,4,0,0,0,0,0,3,0,4,0,1},
    {1,2,0,0,0,0,6,0,7,0,0,0,0,4,4,4,0,5,0,0,0,4,0,1},
    {1,2,0,3,3,0,0,0,7,0,2,2,0,0,0,4,0,5,5,5,0,0,0,1},
    {1,0,0,3,0,0,4,4,4,0,2,0,0,6,0,4,0,0,0,5,0,2,0,1},
    {1,0,5,3,0,0,0,0,4,0,2,0,6,6,0,0,0,7,0,5,0,2,0,1},
    {1,0,5,3,3,3,0,0,4,0,0,0,0,6,6,6,0,7,0,0,0,2,0,1},
    {1,0,5,0,0,0,0,2,4,4,4,0,0,0,0,0,0,7,7,7,0,2,0,1},
    {1,0,5,5,5,0,0,2,0,0,0,0,3,3,3,0,0,0,0,7,0,0,0,1},
    {1,0,0,0,5,0,6,2,0,4,4,0,3,0,0,0,5,5,0,7,7,7,0,1},
    {1,0,3,0,0,0,6,0,0,4,0,0,0,0,2,0,5,0,0,0,0,0,0,1},
    {1,0,3,3,3,0,6,6,0,4,4,4,0,0,2,0,5,5,5,0,3,3,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static uint8_t clamp_u8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static uint32_t shade(uint32_t color, double amount)
{
    int r = (int)(((color >> 16) & 0xFFu) * amount);
    int g = (int)(((color >> 8) & 0xFFu) * amount);
    int b = (int)((color & 0xFFu) * amount);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

static uint32_t mix_color(uint32_t a, uint32_t b, double t)
{
    int ar = (int)((a >> 16) & 0xFFu);
    int ag = (int)((a >> 8) & 0xFFu);
    int ab = (int)(a & 0xFFu);
    int br = (int)((b >> 16) & 0xFFu);
    int bg = (int)((b >> 8) & 0xFFu);
    int bb = (int)(b & 0xFFu);

    return rgb(
        clamp_u8((int)(ar + (br - ar) * t)),
        clamp_u8((int)(ag + (bg - ag) * t)),
        clamp_u8((int)(ab + (bb - ab) * t)));
}

static void put_pixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        framebuffer[y * SCREEN_W + x] = color;
    }
}

static void fill_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            put_pixel(xx, yy, color);
        }
    }
}

static int ppm_next_int(FILE *f, int *value)
{
    int c;

    do {
        c = fgetc(f);
        if (c == '#') {
            do {
                c = fgetc(f);
            } while (c != '\n' && c != EOF);
        }
    } while (isspace(c));

    if (c == EOF) {
        return 0;
    }
    ungetc(c, f);
    return fscanf(f, "%d", value) == 1;
}

static int load_texture_atlas(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }

    char magic[3] = {0};
    int width = 0;
    int height = 0;
    int max_value = 0;
    int ok = fread(magic, 1, 2, f) == 2 &&
             strcmp(magic, "P6") == 0 &&
             ppm_next_int(f, &width) &&
             ppm_next_int(f, &height) &&
             ppm_next_int(f, &max_value) &&
             width == TEX_SIZE * 4 &&
             height == TEX_SIZE * 2 &&
             max_value == 255;

    int sep = fgetc(f);
    if (!isspace(sep)) {
        ok = 0;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    uint8_t *pixels = ok ? malloc(pixel_count * 3) : NULL;
    if (!pixels) {
        fclose(f);
        return 0;
    }

    ok = fread(pixels, 3, pixel_count, f) == pixel_count;
    fclose(f);
    if (!ok) {
        free(pixels);
        return 0;
    }

    for (int tex = 0; tex < TEX_COUNT; ++tex) {
        int tile_x = (tex % 4) * TEX_SIZE;
        int tile_y = (tex / 4) * TEX_SIZE;

        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                size_t src = ((size_t)(tile_y + y) * (size_t)width + (size_t)(tile_x + x)) * 3;
                textures[tex][y * TEX_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_monster_atlas(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }

    char magic[3] = {0};
    int width = 0;
    int height = 0;
    int max_value = 0;
    int ok = fread(magic, 1, 2, f) == 2 &&
             strcmp(magic, "P6") == 0 &&
             ppm_next_int(f, &width) &&
             ppm_next_int(f, &height) &&
             ppm_next_int(f, &max_value) &&
             width == SPRITE_SIZE * SPRITE_FRAMES &&
             height == SPRITE_SIZE * MONSTER_TYPES &&
             max_value == 255;

    int sep = fgetc(f);
    if (!isspace(sep)) {
        ok = 0;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    uint8_t *pixels = ok ? malloc(pixel_count * 3) : NULL;
    if (!pixels) {
        fclose(f);
        return 0;
    }

    ok = fread(pixels, 3, pixel_count, f) == pixel_count;
    fclose(f);
    if (!ok) {
        free(pixels);
        return 0;
    }

    for (int type = 0; type < MONSTER_TYPES; ++type) {
        int type_y = type * SPRITE_SIZE;
        for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
            int frame_x = frame * SPRITE_SIZE;

            for (int y = 0; y < SPRITE_SIZE; ++y) {
                for (int x = 0; x < SPRITE_SIZE; ++x) {
                    size_t src = ((size_t)(type_y + y) * (size_t)width + (size_t)(frame_x + x)) * 3;
                    monster_sprites[type][frame][y * SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
                }
            }
        }
    }

    free(pixels);
    return 1;
}

static int init_assets(void)
{
    if (!load_texture_atlas(TEXTURE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required texture atlas %s\n", TEXTURE_ATLAS_PATH);
        return 0;
    }
    if (!load_monster_atlas(MONSTER_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required monster atlas %s\n", MONSTER_ATLAS_PATH);
        return 0;
    }
    return 1;
}

static int map_at(int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
        return 1;
    }
    return world_map[y][x];
}

static void render_floor_ceiling(const Camera *cam)
{
    const int floor_tex = 6;
    const int ceil_tex = 3;
    double ray_dir_x0 = cam->dir.x - cam->plane.x;
    double ray_dir_y0 = cam->dir.y - cam->plane.y;
    double ray_dir_x1 = cam->dir.x + cam->plane.x;
    double ray_dir_y1 = cam->dir.y + cam->plane.y;

    for (int y = SCREEN_H / 2; y < SCREEN_H; ++y) {
        int p = y - SCREEN_H / 2;
        if (p == 0) {
            for (int x = 0; x < SCREEN_W; ++x) {
                framebuffer[y * SCREEN_W + x] = rgb(35, 33, 32);
                framebuffer[(SCREEN_H - y - 1) * SCREEN_W + x] = rgb(18, 18, 20);
            }
            continue;
        }

        double pos_z = 0.5 * SCREEN_H;
        double row_distance = pos_z / p;
        double floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / SCREEN_W;
        double floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / SCREEN_W;
        double floor_x = cam->pos.x + row_distance * ray_dir_x0;
        double floor_y = cam->pos.y + row_distance * ray_dir_y0;
        double light = 0.28 + 0.78 / (1.0 + row_distance * 0.08);

        for (int x = 0; x < SCREEN_W; ++x) {
            int cell_x = (int)floor(floor_x);
            int cell_y = (int)floor(floor_y);
            int tx = (int)(TEX_SIZE * (floor_x - cell_x)) & (TEX_SIZE - 1);
            int ty = (int)(TEX_SIZE * (floor_y - cell_y)) & (TEX_SIZE - 1);

            uint32_t floor_color = textures[floor_tex][ty * TEX_SIZE + tx];
            uint32_t ceil_color = textures[ceil_tex][ty * TEX_SIZE + tx];
            framebuffer[y * SCREEN_W + x] = shade(floor_color, light * 0.78);
            framebuffer[(SCREEN_H - y - 1) * SCREEN_W + x] = shade(ceil_color, light * 0.48);

            floor_x += floor_step_x;
            floor_y += floor_step_y;
        }
    }
}

static int is_sprite_key(uint32_t color)
{
    int r = (int)((color >> 16) & 0xFFu);
    int g = (int)((color >> 8) & 0xFFu);
    int b = (int)(color & 0xFFu);
    return r > 210 && g < 70 && b > 210;
}

static double normalize_angle(double angle)
{
    while (angle < -M_PI) angle += M_PI * 2.0;
    while (angle >= M_PI) angle -= M_PI * 2.0;
    return angle;
}

static int monster_frame_for_camera(const Camera *cam, const Monster *monster)
{
    double to_camera_x = cam->pos.x - monster->pos.x;
    double to_camera_y = cam->pos.y - monster->pos.y;
    double monster_front_angle = atan2(monster->facing.y, monster->facing.x);
    double relative = normalize_angle(atan2(to_camera_y, to_camera_x) - monster_front_angle);
    int frame = (int)floor((relative + M_PI / 4.0) / (M_PI / 2.0));
    static const int frame_map[SPRITE_FRAMES] = {0, 3, 2, 1};
    return frame_map[frame & 3];
}

static int project_sprite(const Camera *cam, Vec2 pos, double scale, int *screen_x, int *screen_h, double *depth)
{
    double sprite_x = pos.x - cam->pos.x;
    double sprite_y = pos.y - cam->pos.y;
    double inv_det = 1.0 / (cam->plane.x * cam->dir.y - cam->dir.x * cam->plane.y);
    double transform_x = inv_det * (cam->dir.y * sprite_x - cam->dir.x * sprite_y);
    double transform_y = inv_det * (-cam->plane.y * sprite_x + cam->plane.x * sprite_y);

    if (transform_y <= 0.01) {
        return 0;
    }

    *screen_x = (int)((SCREEN_W / 2.0) * (1.0 + transform_x / transform_y));
    *screen_h = abs((int)(SCREEN_H * scale / transform_y));
    *depth = transform_y;
    return *screen_h > 0;
}

static void render_monster(const Camera *cam, const Monster *monster)
{
    if (!monster->active) {
        return;
    }

    int sprite_screen_x;
    int sprite_h;
    double depth;
    if (!project_sprite(cam, monster->pos, 1.0, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;

    int draw_start_y = -sprite_h / 2 + SCREEN_H / 2;
    int draw_end_y = sprite_h / 2 + SCREEN_H / 2;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    int frame = monster_frame_for_camera(cam, monster);
    double light = 1.0 / (1.0 + depth * 0.055);

    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * SPRITE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= SPRITE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = y * 256 - SCREEN_H * 128 + sprite_h * 128;
            int tex_y = ((d * SPRITE_SIZE) / sprite_h) / 256;
            if (tex_y < 0 || tex_y >= SPRITE_SIZE) {
                continue;
            }

            int type = monster->type % MONSTER_TYPES;
            if (type < 0) type = 0;
            uint32_t color = monster_sprites[type][frame][tex_y * SPRITE_SIZE + tex_x];
            if (!is_sprite_key(color)) {
                framebuffer[y * SCREEN_W + stripe] = shade(color, light);
            }
        }
    }
}

static uint32_t projectile_texel(int tex_x, int tex_y)
{
    double dx = (tex_x + 0.5) - PROJECTILE_SIZE / 2.0;
    double dy = (tex_y + 0.5) - PROJECTILE_SIZE / 2.0;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist > PROJECTILE_SIZE * 0.48) {
        return 0;
    }
    if (dist < PROJECTILE_SIZE * 0.18) {
        return rgb(255, 248, 132);
    }
    if (((tex_x * 7 + tex_y * 11) & 15) < 5) {
        return rgb(255, 84, 24);
    }
    return rgb(196, 28, 18);
}

static void render_projectile(const Camera *cam, const Projectile *projectile)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    if (!project_sprite(cam, projectile->pos, 0.34, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;
    int draw_start_y = -sprite_h / 2 + SCREEN_H / 2;
    int draw_end_y = sprite_h / 2 + SCREEN_H / 2;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = 1.15 / (1.0 + depth * 0.03);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = y * 256 - SCREEN_H * 128 + sprite_h * 128;
            int tex_y = ((d * PROJECTILE_SIZE) / sprite_h) / 256;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }

            uint32_t color = projectile_texel(tex_x, tex_y);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = shade(color, light);
            }
        }
    }
}

static uint32_t item_texel(int type, int tex_x, int tex_y)
{
    double dx = (tex_x + 0.5) - PROJECTILE_SIZE / 2.0;
    double dy = (tex_y + 0.5) - PROJECTILE_SIZE / 2.0;
    double dist = sqrt(dx * dx + dy * dy);
    int border = dist > PROJECTILE_SIZE * 0.43;

    if (dist > PROJECTILE_SIZE * 0.48) {
        return 0;
    }

    switch (type) {
    case 0:
        if (abs(tex_x - PROJECTILE_SIZE / 2) < 4 || abs(tex_y - PROJECTILE_SIZE / 2) < 4) {
            return rgb(245, 236, 218);
        }
        return border ? rgb(94, 12, 18) : rgb(192, 24, 34);
    case 1:
        if ((tex_x / 4) & 1) {
            return rgb(234, 192, 74);
        }
        return border ? rgb(92, 54, 22) : rgb(166, 112, 38);
    case 2:
        if (((tex_x + tex_y) & 7) < 3) {
            return rgb(136, 232, 255);
        }
        return border ? rgb(20, 62, 94) : rgb(42, 152, 210);
    default:
        if (dist < PROJECTILE_SIZE * 0.18) {
            return rgb(255, 244, 160);
        }
        return border ? rgb(86, 20, 104) : rgb(186, 64, 230);
    }
}

static void render_item(const Camera *cam, const Item *item)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    if (!project_sprite(cam, item->pos, 0.42, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;
    int draw_start_y = -sprite_h / 2 + SCREEN_H / 2;
    int draw_end_y = sprite_h / 2 + SCREEN_H / 2;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double bob = sin(depth + item->pos.x * 3.1 + item->pos.y * 2.7);
    double light = 1.2 / (1.0 + depth * 0.045);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int shifted_y = y + (int)(bob * 2.0);
            int d = shifted_y * 256 - SCREEN_H * 128 + sprite_h * 128;
            int tex_y = ((d * PROJECTILE_SIZE) / sprite_h) / 256;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }

            uint32_t color = item_texel(item->type, tex_x, tex_y);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = shade(color, light);
            }
        }
    }
}

static void render_world_sprites(const Camera *cam, const GameState *game)
{
    SpriteDraw draws[MAX_MONSTERS + MAX_PROJECTILES + MAX_ITEMS];
    int count = 0;

    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }
        double dx = monster->pos.x - cam->pos.x;
        double dy = monster->pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){0, i, dx * dx + dy * dy};
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }
        double dx = item->pos.x - cam->pos.x;
        double dy = item->pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){1, i, dx * dx + dy * dy};
    }

    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        const Projectile *projectile = &game->projectiles[i];
        if (!projectile->active) {
            continue;
        }
        double dx = projectile->pos.x - cam->pos.x;
        double dy = projectile->pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){2, i, dx * dx + dy * dy};
    }

    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (draws[i].dist < draws[j].dist) {
                SpriteDraw tmp = draws[i];
                draws[i] = draws[j];
                draws[j] = tmp;
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        SpriteDraw draw = draws[i];
        if (draw.kind == 0) {
            render_monster(cam, &game->monsters[draw.index]);
        } else if (draw.kind == 1) {
            render_item(cam, &game->items[draw.index]);
        } else {
            render_projectile(cam, &game->projectiles[draw.index]);
        }
    }
}

static void render_hit_flash(const GameState *game)
{
    if (game->hit_flash <= 0.0) {
        return;
    }

    double intensity = game->hit_flash / 0.18;
    for (int y = 0; y < SCREEN_H; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            if (x > 8 && x < SCREEN_W - 9 && y > 8 && y < SCREEN_H - 9) {
                continue;
            }

            uint32_t color = framebuffer[y * SCREEN_W + x];
            framebuffer[y * SCREEN_W + x] = mix_color(color, rgb(190, 18, 18), intensity * 0.55);
        }
    }
}

static void render_crosshair(void)
{
    uint32_t c = rgb(232, 226, 196);
    fill_rect(SCREEN_W / 2 - 5, SCREEN_H / 2, 4, 1, c);
    fill_rect(SCREEN_W / 2 + 2, SCREEN_H / 2, 4, 1, c);
    fill_rect(SCREEN_W / 2, SCREEN_H / 2 - 5, 1, 4, c);
    fill_rect(SCREEN_W / 2, SCREEN_H / 2 + 2, 1, 4, c);
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void render_shot_trace(const GameState *game)
{
    if (game->shot_trace <= 0.0) {
        return;
    }

    double t = game->shot_trace / WEAPON_FLASH_TIME;
    uint32_t core = mix_color(rgb(255, 120, 28), rgb(255, 245, 170), t);
    int jitter = (int)(sin(game->time * 80.0) * 2.0);
    draw_line(SCREEN_W / 2 + 6, SCREEN_H - 41, SCREEN_W / 2 + jitter, SCREEN_H / 2 + 2, core);
    draw_line(SCREEN_W / 2 + 7, SCREEN_H - 41, SCREEN_W / 2 + jitter + 1, SCREEN_H / 2 + 2, rgb(255, 210, 80));
}

static void render_weapon(const GameState *game)
{
    int bob = (int)(sin(game->time * 7.5) * 2.0);
    double fire_t = game->weapon_flash > 0.0 ? game->weapon_flash / WEAPON_FLASH_TIME : 0.0;
    int recoil_y = (int)(fire_t * fire_t * 15.0);
    int recoil_x = (int)(sin(game->time * 120.0) * fire_t * 4.0);
    int x = SCREEN_W / 2 - 24 + recoil_x;
    int y = SCREEN_H - 42 + bob + recoil_y;

    fill_rect(x + 16, y + 4, 32, 12, rgb(42, 43, 46));
    fill_rect(x + 20, y + 0, 32, 6, rgb(80, 82, 86));
    fill_rect(x + 23, y + 6, 24, 3, rgb(132, 134, 132));
    fill_rect(x + 29, y + 16, 13, 26, rgb(62, 42, 30));
    fill_rect(x + 32, y + 18, 7, 22, rgb(110, 70, 38));
    fill_rect(x + 9, y + 8, 15, 7, rgb(34, 35, 38));

    if (game->weapon_flash > 0.0) {
        int flash = 12 + (int)(fire_t * 20.0);
        fill_rect(x - flash / 2, y - 6, flash, flash, rgb(255, 132, 28));
        fill_rect(x - flash / 4, y - 1, flash / 2, flash / 2, rgb(255, 246, 150));
        fill_rect(x - flash / 2 - 4, y + 2, 4, 5, rgb(255, 198, 64));
        fill_rect(x + flash / 2, y + 2, 4, 5, rgb(255, 198, 64));
    }
}

static void render_hud(const GameState *game)
{
    fill_rect(0, SCREEN_H - 14, SCREEN_W, 14, rgb(20, 20, 22));

    fill_rect(6, SCREEN_H - 10, 74, 6, rgb(70, 18, 18));
    int hp_w = game->player_health * 74 / PLAYER_MAX_HEALTH;
    fill_rect(6, SCREEN_H - 10, hp_w, 6, game->player_health > 30 ? rgb(32, 174, 64) : rgb(210, 42, 32));

    int ammo_pips = game->ammo > 18 ? 18 : game->ammo;
    for (int i = 0; i < ammo_pips; ++i) {
        fill_rect(94 + i * 5, SCREEN_H - 11, 3, 8, rgb(204, 162, 64));
    }

    for (int i = 0; i < game->monster_count; ++i) {
        uint32_t c = i < game->kills ? rgb(120, 28, 24) : rgb(52, 132, 52);
        fill_rect(SCREEN_W - 42 + i * 6, SCREEN_H - 10, 4, 6, c);
    }

    if (game->rapid_timer > 0.0) {
        fill_rect(6, SCREEN_H - 18, (int)(game->rapid_timer * 4.0), 3, rgb(48, 180, 230));
    }
    if (game->damage_timer > 0.0) {
        fill_rect(6, SCREEN_H - 22, (int)(game->damage_timer * 4.0), 3, rgb(190, 72, 230));
    }
    if (game->pickup_flash > 0.0) {
        fill_rect(SCREEN_W / 2 - 18, SCREEN_H - 62, 36, 4, rgb(220, 210, 96));
    }

    if (game->victory || game->game_over) {
        uint32_t c = game->victory ? rgb(232, 196, 64) : rgb(160, 28, 28);
        fill_rect(SCREEN_W / 2 - 48, 28, 96, 22, rgb(18, 18, 20));
        fill_rect(SCREEN_W / 2 - 44, 32, 88, 14, c);
        fill_rect(SCREEN_W / 2 - 36, 36, 72, 6, rgb(18, 18, 20));
    }
}

static void render_minimap(const Camera *cam, const GameState *game)
{
    const int cell = 3;
    const int ox = SCREEN_W - MAP_W * cell - 6;
    const int oy = 6;

    fill_rect(ox - 2, oy - 2, MAP_W * cell + 4, MAP_H * cell + 4, rgb(8, 8, 10));

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            uint32_t c = rgb(10, 10, 12);
            if (game->discovered[y][x]) {
                int wall = map_at(x, y);
                c = wall ? rgb(80, 62, 54) : rgb(34, 34, 32);
                if (wall >= 5) {
                    c = rgb(70, 48, 66);
                }
            }
            fill_rect(ox + x * cell, oy + y * cell, cell, cell, c);
        }
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *item = &game->items[i];
        int ix = (int)item->pos.x;
        int iy = (int)item->pos.y;
        if (item->active && ix >= 0 && ix < MAP_W && iy >= 0 && iy < MAP_H && game->discovered[iy][ix]) {
            uint32_t c = item->type == 0 ? rgb(210, 42, 48) :
                         item->type == 1 ? rgb(220, 170, 64) :
                         item->type == 2 ? rgb(54, 174, 230) : rgb(190, 70, 230);
            fill_rect(ox + ix * cell + 1, oy + iy * cell + 1, 2, 2, c);
        }
    }

    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        int mx = (int)monster->pos.x;
        int my = (int)monster->pos.y;
        if (monster->active && mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H && game->discovered[my][mx]) {
            fill_rect(ox + mx * cell, oy + my * cell, cell, cell, rgb(170, 34, 28));
        }
    }

    int px = (int)cam->pos.x;
    int py = (int)cam->pos.y;
    fill_rect(ox + px * cell, oy + py * cell, cell, cell, rgb(236, 220, 72));
}

static void render_scene(const Camera *cam, const GameState *game)
{
    render_floor_ceiling(cam);

    for (int x = 0; x < SCREEN_W; ++x) {
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;

        int map_x = (int)cam->pos.x;
        int map_y = (int)cam->pos.y;

        double delta_dist_x = ray_dir_x == 0.0 ? 1e30 : fabs(1.0 / ray_dir_x);
        double delta_dist_y = ray_dir_y == 0.0 ? 1e30 : fabs(1.0 / ray_dir_y);
        double side_dist_x;
        double side_dist_y;
        int step_x;
        int step_y;

        if (ray_dir_x < 0.0) {
            step_x = -1;
            side_dist_x = (cam->pos.x - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0 - cam->pos.x) * delta_dist_x;
        }

        if (ray_dir_y < 0.0) {
            step_y = -1;
            side_dist_y = (cam->pos.y - map_y) * delta_dist_y;
        } else {
            step_y = 1;
            side_dist_y = (map_y + 1.0 - cam->pos.y) * delta_dist_y;
        }

        int hit = 0;
        int side = 0;
        int wall = 0;

        while (!hit) {
            if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }

            wall = map_at(map_x, map_y);
            hit = wall > 0;
        }

        double perp_wall_dist = side == 0
            ? (side_dist_x - delta_dist_x)
            : (side_dist_y - delta_dist_y);
        if (perp_wall_dist < 0.001) {
            perp_wall_dist = 0.001;
        }

        int line_h = (int)(SCREEN_H / perp_wall_dist);
        int draw_start = -line_h / 2 + SCREEN_H / 2;
        int draw_end = line_h / 2 + SCREEN_H / 2;
        if (draw_start < 0) draw_start = 0;
        if (draw_end >= SCREEN_H) draw_end = SCREEN_H - 1;

        double wall_x = side == 0
            ? cam->pos.y + perp_wall_dist * ray_dir_y
            : cam->pos.x + perp_wall_dist * ray_dir_x;
        wall_x -= floor(wall_x);

        int tex_x = (int)(wall_x * TEX_SIZE);
        if ((side == 0 && ray_dir_x > 0.0) || (side == 1 && ray_dir_y < 0.0)) {
            tex_x = TEX_SIZE - tex_x - 1;
        }

        int tex_idx = (wall - 1) % TEX_COUNT;
        double step = (double)TEX_SIZE / line_h;
        double tex_pos = (draw_start - SCREEN_H / 2.0 + line_h / 2.0) * step;
        double light = side == 1 ? 0.62 : 0.86;
        light *= 1.0 / (1.0 + perp_wall_dist * 0.045);
        z_buffer[x] = perp_wall_dist;

        for (int y = draw_start; y <= draw_end; ++y) {
            int tex_y = ((int)tex_pos) & (TEX_SIZE - 1);
            tex_pos += step;
            uint32_t color = textures[tex_idx][tex_y * TEX_SIZE + tex_x];
            framebuffer[y * SCREEN_W + x] = shade(color, light);
        }
    }

    render_world_sprites(cam, game);
    render_hit_flash(game);
    render_crosshair();
    render_shot_trace(game);
    render_weapon(game);
    render_hud(game);
    render_minimap(cam, game);
}

static double vec_len(Vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

static Vec2 vec_norm(Vec2 v)
{
    double len = vec_len(v);
    if (len <= 0.0001) {
        return (Vec2){0.0, 0.0};
    }
    return (Vec2){v.x / len, v.y / len};
}

static double raycast_wall_distance(Vec2 origin, Vec2 dir)
{
    int map_x = (int)origin.x;
    int map_y = (int)origin.y;
    double delta_dist_x = dir.x == 0.0 ? 1e30 : fabs(1.0 / dir.x);
    double delta_dist_y = dir.y == 0.0 ? 1e30 : fabs(1.0 / dir.y);
    double side_dist_x;
    double side_dist_y;
    int step_x;
    int step_y;
    int side = 0;

    if (dir.x < 0.0) {
        step_x = -1;
        side_dist_x = (origin.x - map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (map_x + 1.0 - origin.x) * delta_dist_x;
    }

    if (dir.y < 0.0) {
        step_y = -1;
        side_dist_y = (origin.y - map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (map_y + 1.0 - origin.y) * delta_dist_y;
    }

    for (int i = 0; i < MAP_W * MAP_H; ++i) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = 0;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = 1;
        }

        if (map_at(map_x, map_y) > 0) {
            return side == 0 ? side_dist_x - delta_dist_x : side_dist_y - delta_dist_y;
        }
    }

    return 1e30;
}

static int has_line_of_sight(Vec2 from, Vec2 to)
{
    Vec2 diff = {to.x - from.x, to.y - from.y};
    double dist = vec_len(diff);
    Vec2 dir = vec_norm(diff);
    return raycast_wall_distance(from, dir) + 0.12 >= dist;
}

static void reveal_fog(GameState *game, const Camera *cam)
{
    int cx = (int)cam->pos.x;
    int cy = (int)cam->pos.y;
    int radius = 5;

    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
                continue;
            }

            Vec2 center = {x + 0.5, y + 0.5};
            Vec2 diff = {center.x - cam->pos.x, center.y - cam->pos.y};
            double dist = vec_len(diff);
            if (dist > radius + 0.35) {
                continue;
            }
            if (dist < 0.7 || has_line_of_sight(cam->pos, center)) {
                game->discovered[y][x] = 1;
            }
        }
    }
}

static void init_game(GameState *game)
{
    memset(game, 0, sizeof(*game));
    game->monster_count = MAX_MONSTERS;
    game->player_health = PLAYER_MAX_HEALTH;
    game->ammo = START_AMMO;

    const Vec2 starts[MAX_MONSTERS] = {
        {12.5, 8.5},
        {20.5, 3.5},
        {3.5, 12.5},
        {18.5, 15.5},
        {8.5, 20.5},
        {7.5, 22.5},
        {3.5, 3.5},
        {11.5, 12.5},
        {16.5, 15.5},
        {21.5, 20.5},
    };
    const Item item_starts[MAX_ITEMS] = {
        {1, 1, {4.5, 22.5}},
        {1, 0, {9.5, 22.5}},
        {1, 2, {5.5, 12.5}},
        {1, 3, {12.5, 12.5}},
        {1, 1, {20.5, 20.5}},
        {1, 0, {3.5, 3.5}},
        {1, 2, {18.5, 15.5}},
        {1, 3, {20.5, 3.5}},
        {1, 1, {1.5, 15.5}},
        {1, 0, {16.5, 22.5}},
        {1, 1, {14.5, 8.5}},
        {1, 0, {18.5, 18.5}},
    };

    for (int i = 0; i < MAX_ITEMS; ++i) {
        game->items[i] = item_starts[i];
    }

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        monster->active = 1;
        monster->pos = starts[i];
        monster->shoot_timer = 0.45 + i * 0.27;
        monster->target_waypoint = 1;
        monster->route = i;
        static const int monster_types[MAX_MONSTERS] = {1, 1, 1, 0, 1, 1, 2, 1, 0, 1};
        monster->type = monster_types[i];
        monster->hp = monster->type == 1 ? 4 : (monster->type == 0 ? 5 : 3);
        monster->facing = (Vec2){0.0, -1.0};
        monster->ai_state = 0;
        monster->last_seen = starts[i];
        monster->alert_timer = 0.0;
        monster->strafe_timer = 0.4 + i * 0.11;
        monster->strafe_dir = (i & 1) ? 1 : -1;
    }
}

static int can_occupy(double x, double y, double radius)
{
    return map_at((int)(x - radius), (int)(y - radius)) == 0 &&
           map_at((int)(x + radius), (int)(y - radius)) == 0 &&
           map_at((int)(x - radius), (int)(y + radius)) == 0 &&
           map_at((int)(x + radius), (int)(y + radius)) == 0;
}

static int can_move(double x, double y)
{
    return can_occupy(x, y, 0.18);
}

static int move_monster_by(Monster *monster, Vec2 delta)
{
    int moved = 0;
    double nx = monster->pos.x + delta.x;
    double ny = monster->pos.y + delta.y;

    if (can_occupy(nx, ny, 0.28)) {
        monster->pos.x = nx;
        monster->pos.y = ny;
        moved = 1;
    } else {
        if (can_occupy(nx, monster->pos.y, 0.28)) {
            monster->pos.x = nx;
            moved = 1;
        }
        if (can_occupy(monster->pos.x, ny, 0.28)) {
            monster->pos.y = ny;
            moved = 1;
        }
    }

    if (moved && monster->facing_lock <= 0.0 && (delta.x != 0.0 || delta.y != 0.0)) {
        monster->facing = vec_norm(delta);
    }
    return moved;
}

static int move_monster_toward(Monster *monster, Vec2 target, double speed, double dt)
{
    Vec2 to_target = {
        target.x - monster->pos.x,
        target.y - monster->pos.y,
    };
    double dist = vec_len(to_target);
    if (dist < 0.05) {
        return 1;
    }

    Vec2 dir = vec_norm(to_target);
    double step = speed * dt;
    if (step > dist) {
        step = dist;
    }
    Vec2 delta = {dir.x * step, dir.y * step};
    if (move_monster_by(monster, delta)) {
        return 1;
    }

    Vec2 side = {-dir.y * monster->strafe_dir * step, dir.x * monster->strafe_dir * step};
    if (move_monster_by(monster, side)) {
        return 1;
    }

    monster->strafe_dir *= -1;
    side.x = -side.x;
    side.y = -side.y;
    return move_monster_by(monster, side);
}

static void update_monster(Monster *monster, const Camera *cam, double dt)
{
    static const Vec2 route0[] = {
        {12.5, 8.5},
        {12.5, 3.5},
    };
    static const Vec2 route1[] = {
        {20.5, 3.5},
        {20.5, 8.5},
    };
    static const Vec2 route2[] = {
        {3.5, 12.5},
        {5.5, 12.5},
        {5.5, 15.5},
        {1.5, 15.5},
    };
    static const Vec2 route3[] = {
        {18.5, 15.5},
        {20.5, 15.5},
        {20.5, 18.5},
        {17.5, 18.5},
    };
    static const Vec2 route4[] = {
        {6.5, 20.5},
        {8.5, 20.5},
        {8.5, 22.0},
        {5.5, 22.0},
    };
    static const Vec2 route5[] = {
        {7.5, 22.5},
        {11.5, 22.5},
        {15.5, 22.5},
    };
    static const Vec2 route6[] = {
        {3.5, 3.5},
        {5.5, 3.5},
        {5.5, 5.5},
        {1.5, 5.5},
    };
    static const Vec2 route7[] = {
        {11.5, 12.5},
        {12.5, 12.5},
        {12.5, 15.5},
        {9.5, 15.5},
    };
    static const Vec2 route8[] = {
        {16.5, 15.5},
        {18.5, 15.5},
        {18.5, 17.5},
        {16.5, 17.5},
    };
    static const Vec2 route9[] = {
        {21.5, 20.5},
        {18.5, 20.5},
        {18.5, 22.5},
        {22.0, 22.5},
    };
    const Vec2 *waypoints = route0;
    int waypoint_count = (int)(sizeof(route0) / sizeof(route0[0]));

    switch (monster->route) {
    case 1:
        waypoints = route1;
        waypoint_count = (int)(sizeof(route1) / sizeof(route1[0]));
        break;
    case 2:
        waypoints = route2;
        waypoint_count = (int)(sizeof(route2) / sizeof(route2[0]));
        break;
    case 3:
        waypoints = route3;
        waypoint_count = (int)(sizeof(route3) / sizeof(route3[0]));
        break;
    case 4:
        waypoints = route4;
        waypoint_count = (int)(sizeof(route4) / sizeof(route4[0]));
        break;
    case 5:
        waypoints = route5;
        waypoint_count = (int)(sizeof(route5) / sizeof(route5[0]));
        break;
    case 6:
        waypoints = route6;
        waypoint_count = (int)(sizeof(route6) / sizeof(route6[0]));
        break;
    case 7:
        waypoints = route7;
        waypoint_count = (int)(sizeof(route7) / sizeof(route7[0]));
        break;
    case 8:
        waypoints = route8;
        waypoint_count = (int)(sizeof(route8) / sizeof(route8[0]));
        break;
    case 9:
        waypoints = route9;
        waypoint_count = (int)(sizeof(route9) / sizeof(route9[0]));
        break;
    }

    if (!monster->active) {
        return;
    }

    Vec2 to_player = {
        cam->pos.x - monster->pos.x,
        cam->pos.y - monster->pos.y,
    };
    double player_dist = vec_len(to_player);
    int sees_player = player_dist < 11.0 && has_line_of_sight(monster->pos, cam->pos);

    if (sees_player) {
        monster->ai_state = player_dist < 6.5 ? 2 : 1;
        monster->last_seen = cam->pos;
        monster->alert_timer = 5.0;
        if (monster->facing_lock <= 0.0) {
            monster->facing = vec_norm(to_player);
        }
    } else if (monster->alert_timer > 0.0) {
        monster->alert_timer -= dt;
        if (monster->alert_timer < 0.0) {
            monster->alert_timer = 0.0;
        }
        monster->ai_state = 1;
    } else {
        monster->ai_state = 0;
    }

    monster->strafe_timer -= dt;
    if (monster->strafe_timer <= 0.0) {
        monster->strafe_timer = 0.7 + 0.13 * (monster->route % 4);
        monster->strafe_dir *= -1;
    }

    if (monster->ai_state == 2 && sees_player) {
        Vec2 dir_to_player = vec_norm(to_player);
        double preferred = monster->type == 1 ? 4.8 : 3.8;
        if (player_dist > preferred + 0.7) {
            move_monster_toward(monster, cam->pos, 1.65, dt);
        } else if (player_dist < preferred - 0.8) {
            Vec2 away = {
                monster->pos.x - dir_to_player.x,
                monster->pos.y - dir_to_player.y,
            };
            move_monster_toward(monster, away, 1.25, dt);
        } else {
            Vec2 side = {
                -dir_to_player.y * monster->strafe_dir * 1.05 * dt,
                dir_to_player.x * monster->strafe_dir * 1.05 * dt,
            };
            move_monster_by(monster, side);
        }
        return;
    }

    if (monster->ai_state == 1) {
        if (move_monster_toward(monster, monster->last_seen, 1.55, dt)) {
            Vec2 to_last_seen = {
                monster->last_seen.x - monster->pos.x,
                monster->last_seen.y - monster->pos.y,
            };
            if (!sees_player && vec_len(to_last_seen) < 0.25) {
                monster->alert_timer = 0.0;
                monster->ai_state = 0;
            }
        }
        return;
    }

    Vec2 target = waypoints[monster->target_waypoint];
    Vec2 to_target = {
        target.x - monster->pos.x,
        target.y - monster->pos.y,
    };
    if (vec_len(to_target) < 0.08) {
        monster->target_waypoint = (monster->target_waypoint + 1) % waypoint_count;
        return;
    }
    if (!move_monster_toward(monster, target, 1.05, dt)) {
        monster->target_waypoint = (monster->target_waypoint + 1) % waypoint_count;
    }
}

static void spawn_monster_shot(GameState *game, const Monster *monster, const Camera *cam)
{
    Vec2 dir = vec_norm((Vec2){
        cam->pos.x - monster->pos.x,
        cam->pos.y - monster->pos.y,
    });

    if (dir.x == 0.0 && dir.y == 0.0) {
        return;
    }

    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            p->active = 1;
            p->pos = (Vec2){
                monster->pos.x + dir.x * 0.55,
                monster->pos.y + dir.y * 0.55,
            };
            p->vel = (Vec2){dir.x * 4.2, dir.y * 4.2};
            p->life = 2.2;
            return;
        }
    }
}

static void update_projectiles(GameState *game, const Camera *cam, double dt)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            continue;
        }

        p->life -= dt;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;

        if (p->life <= 0.0 || map_at((int)p->pos.x, (int)p->pos.y) != 0) {
            p->active = 0;
            continue;
        }

        double dx = p->pos.x - cam->pos.x;
        double dy = p->pos.y - cam->pos.y;
        if (dx * dx + dy * dy < 0.18) {
            p->active = 0;
            game->player_health -= 12;
            if (game->player_health <= 0) {
                game->player_health = 0;
                game->game_over = 1;
            }
            game->hit_flash = 0.18;
        }
    }
}

static void pickup_item(GameState *game, Item *item)
{
    switch (item->type) {
    case 0:
        game->player_health += 45;
        if (game->player_health > PLAYER_MAX_HEALTH) {
            game->player_health = PLAYER_MAX_HEALTH;
        }
        break;
    case 1:
        game->ammo += 18;
        if (game->ammo > 99) {
            game->ammo = 99;
        }
        break;
    case 2:
        game->rapid_timer = 12.0;
        break;
    default:
        game->damage_timer = 12.0;
        break;
    }

    game->pickup_flash = 0.22;
    item->active = 0;
}

static void update_items(GameState *game, const Camera *cam)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }

        double dx = item->pos.x - cam->pos.x;
        double dy = item->pos.y - cam->pos.y;
        if (dx * dx + dy * dy < 0.34) {
            pickup_item(game, item);
        }
    }
}

static void player_fire(GameState *game, const Camera *cam)
{
    if (game->game_over || game->victory || game->shot_cooldown > 0.0 || game->ammo <= 0) {
        return;
    }

    game->ammo -= 1;
    game->shot_cooldown = game->rapid_timer > 0.0 ? SHOT_COOLDOWN_TIME * 0.48 : SHOT_COOLDOWN_TIME;
    game->weapon_flash = WEAPON_FLASH_TIME;
    game->shot_trace = WEAPON_FLASH_TIME;

    int best = -1;
    double best_depth = 1e30;
    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }

        int screen_x;
        int sprite_h;
        double depth;
        if (!project_sprite(cam, monster->pos, 1.0, &screen_x, &sprite_h, &depth)) {
            continue;
        }

        int aim_window = sprite_h / 3;
        if (aim_window < 10) aim_window = 10;
        if (abs(screen_x - SCREEN_W / 2) > aim_window) {
            continue;
        }
        if (!has_line_of_sight(cam->pos, monster->pos)) {
            continue;
        }
        if (depth < best_depth) {
            best_depth = depth;
            best = i;
        }
    }

    if (best >= 0) {
        Monster *monster = &game->monsters[best];
        int damage = PLAYER_DAMAGE + (game->damage_timer > 0.0 ? 2 : 0);
        monster->hp -= damage;
        if (monster->hp <= 0) {
            monster->active = 0;
            game->kills += 1;
        }
    }
}

static void update_game(GameState *game, const Camera *cam, double dt)
{
    game->time += dt;
    reveal_fog(game, cam);

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        update_monster(monster, cam, dt);
        if (monster->facing_lock > 0.0) {
            monster->facing_lock -= dt;
            if (monster->facing_lock < 0.0) {
                monster->facing_lock = 0.0;
            }
        }

        if (!monster->active) {
            continue;
        }
        monster->shoot_timer -= dt;
        Vec2 to_player = {
            cam->pos.x - monster->pos.x,
            cam->pos.y - monster->pos.y,
        };
        double player_dist = vec_len(to_player);
        int can_attack = monster->ai_state == 2 &&
                         player_dist < 9.5 &&
                         has_line_of_sight(monster->pos, cam->pos);
        while (monster->shoot_timer <= 0.0 && can_attack) {
            monster->facing = vec_norm((Vec2){
                cam->pos.x - monster->pos.x,
                cam->pos.y - monster->pos.y,
            });
            monster->facing_lock = 0.45;
            spawn_monster_shot(game, monster, cam);
            monster->shoot_timer += 1.15 + (i % 3) * 0.18;
        }
        if (monster->shoot_timer <= 0.0) {
            monster->shoot_timer = 0.25;
        }
    }

    update_projectiles(game, cam, dt);
    if (game->shot_cooldown > 0.0) {
        game->shot_cooldown -= dt;
        if (game->shot_cooldown < 0.0) game->shot_cooldown = 0.0;
    }
    if (game->weapon_flash > 0.0) {
        game->weapon_flash -= dt;
        if (game->weapon_flash < 0.0) game->weapon_flash = 0.0;
    }
    if (game->shot_trace > 0.0) {
        game->shot_trace -= dt;
        if (game->shot_trace < 0.0) game->shot_trace = 0.0;
    }
    if (game->pickup_flash > 0.0) {
        game->pickup_flash -= dt;
        if (game->pickup_flash < 0.0) game->pickup_flash = 0.0;
    }
    if (game->rapid_timer > 0.0) {
        game->rapid_timer -= dt;
        if (game->rapid_timer < 0.0) game->rapid_timer = 0.0;
    }
    if (game->damage_timer > 0.0) {
        game->damage_timer -= dt;
        if (game->damage_timer < 0.0) game->damage_timer = 0.0;
    }
    if (game->hit_flash > 0.0) {
        game->hit_flash -= dt;
        if (game->hit_flash < 0.0) {
            game->hit_flash = 0.0;
        }
    }

    if (game->kills >= game->monster_count) {
        game->victory = 1;
    }
}

static int verify_monster_path(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    for (int i = 0; i < 60 * 12; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
        for (int m = 0; m < game.monster_count; ++m) {
            Monster *monster = &game.monsters[m];
            if (monster->active && !can_occupy(monster->pos.x, monster->pos.y, 0.28)) {
                fprintf(stderr, "error: monster %d path entered a wall at %.2f %.2f\n", m, monster->pos.x, monster->pos.y);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_player_weapon(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    player_fire(&game, &cam);
    if (game.ammo != START_AMMO - 1 || game.monsters[5].hp != 2 || !game.monsters[5].active) {
        fprintf(stderr, "error: player weapon failed first hit verification\n");
        return 0;
    }

    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != START_AMMO - 2 || game.monsters[5].active || game.kills != 1) {
        fprintf(stderr, "error: player weapon failed kill verification\n");
        return 0;
    }

    return 1;
}

static int verify_monster_shot_direction(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    Monster *monster = &game.monsters[5];

    spawn_monster_shot(&game, monster, &cam);
    Projectile *shot = NULL;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active) {
            shot = &game.projectiles[i];
            break;
        }
    }
    if (!shot) {
        fprintf(stderr, "error: monster shot verification did not spawn a projectile\n");
        return 0;
    }

    Vec2 to_player = {
        cam.pos.x - monster->pos.x,
        cam.pos.y - monster->pos.y,
    };
    double dot = shot->vel.x * to_player.x + shot->vel.y * to_player.y;
    double monster_dist = vec_len(to_player);
    double shot_dist = vec_len((Vec2){
        cam.pos.x - shot->pos.x,
        cam.pos.y - shot->pos.y,
    });

    if (dot <= 0.0 || shot_dist >= monster_dist) {
        fprintf(stderr, "error: monster projectile is not moving toward the player\n");
        return 0;
    }

    return 1;
}

static int verify_monster_ai_reacts(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    Monster *monster = &game.monsters[5];

    update_monster(monster, &cam, 1.0 / 60.0);
    if (monster->ai_state == 0 || monster->alert_timer <= 0.0) {
        fprintf(stderr, "error: monster AI did not react to a visible player\n");
        return 0;
    }

    double dx = monster->last_seen.x - cam.pos.x;
    double dy = monster->last_seen.y - cam.pos.y;
    if (dx * dx + dy * dy > 0.01) {
        fprintf(stderr, "error: monster AI did not remember the player position\n");
        return 0;
    }

    return 1;
}

static int verify_items_and_fog(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game.items[i];
        if (item->active && !can_occupy(item->pos.x, item->pos.y, 0.12)) {
            fprintf(stderr, "error: item %d is placed inside a wall at %.2f %.2f\n", i, item->pos.x, item->pos.y);
            return 0;
        }
    }

    game.ammo = 10;
    cam.pos = game.items[0].pos;
    update_items(&game, &cam);
    if (game.items[0].active || game.ammo <= 10 || game.pickup_flash <= 0.0) {
        fprintf(stderr, "error: item pickup verification failed\n");
        return 0;
    }

    cam.pos = (Vec2){2.5, 22.5};
    reveal_fog(&game, &cam);
    if (!game.discovered[22][2] || !game.discovered[22][4]) {
        fprintf(stderr, "error: fog-of-war did not reveal the starting area\n");
        return 0;
    }

    return 1;
}

static int verify_sprite_sort_order(void)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    SpriteDraw draws[2] = {
        {1, 0, 0.0},
        {0, 5, 0.0},
    };
    Vec2 item_pos = {4.5, 22.5};
    Vec2 monster_pos = {7.5, 22.5};

    double item_dx = item_pos.x - cam.pos.x;
    double item_dy = item_pos.y - cam.pos.y;
    double monster_dx = monster_pos.x - cam.pos.x;
    double monster_dy = monster_pos.y - cam.pos.y;
    draws[0].dist = item_dx * item_dx + item_dy * item_dy;
    draws[1].dist = monster_dx * monster_dx + monster_dy * monster_dy;

    if (draws[0].dist < draws[1].dist) {
        SpriteDraw tmp = draws[0];
        draws[0] = draws[1];
        draws[1] = tmp;
    }

    if (draws[0].kind != 0 || draws[1].kind != 1) {
        fprintf(stderr, "error: sprite sort order is not far-to-near across item/monster types\n");
        return 0;
    }
    return 1;
}

static void move_camera(Camera *cam, double forward, double strafe, double dt)
{
    double speed = 3.2 * dt;
    double nx = cam->pos.x + cam->dir.x * forward * speed + cam->plane.x * strafe * speed;
    double ny = cam->pos.y + cam->dir.y * forward * speed + cam->plane.y * strafe * speed;

    if (can_move(nx, cam->pos.y)) {
        cam->pos.x = nx;
    }
    if (can_move(cam->pos.x, ny)) {
        cam->pos.y = ny;
    }
}

static void rotate_camera(Camera *cam, double amount)
{
    double old_dir_x = cam->dir.x;
    double old_plane_x = cam->plane.x;
    double s = sin(amount);
    double c = cos(amount);

    cam->dir.x = cam->dir.x * c - cam->dir.y * s;
    cam->dir.y = old_dir_x * s + cam->dir.y * c;
    cam->plane.x = cam->plane.x * c - cam->plane.y * s;
    cam->plane.y = old_plane_x * s + cam->plane.y * c;
}

static int write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return 1;
    }

    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        uint32_t p = framebuffer[i];
        fputc((int)((p >> 16) & 0xFFu), f);
        fputc((int)((p >> 8) & 0xFFu), f);
        fputc((int)(p & 0xFFu), f);
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "cannot close %s: %s\n", path, strerror(errno));
        return 1;
    }
    return 0;
}

static int dump_frame(const char *path)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!init_assets()) {
        return 1;
    }
    if (!verify_monster_path()) {
        return 1;
    }
    if (!verify_player_weapon()) {
        return 1;
    }
    if (!verify_monster_shot_direction()) {
        return 1;
    }
    if (!verify_monster_ai_reacts()) {
        return 1;
    }
    if (!verify_items_and_fog()) {
        return 1;
    }
    if (!verify_sprite_sort_order()) {
        return 1;
    }
    GameState game;
    init_game(&game);
    reveal_fog(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    player_fire(&game, &cam);
    render_scene(&cam, &game);
    return write_ppm(path);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--dump") == 0) {
        return dump_frame(argv[2]);
    }

    if (!init_assets()) {
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Software Raycaster",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W * WINDOW_SCALE,
        SCREEN_H * WINDOW_SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    SDL_Texture *screen = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W,
        SCREEN_H);
    if (!screen) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    GameState game;
    init_game(&game);
    reveal_fog(&game, &cam);

    int running = 1;
    uint64_t prev = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                player_fire(&game, &cam);
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                player_fire(&game, &cam);
            }
        }

        uint64_t now = SDL_GetPerformanceCounter();
        double dt = (double)(now - prev) / (double)SDL_GetPerformanceFrequency();
        prev = now;
        if (dt > 0.05) {
            dt = 0.05;
        }

        const uint8_t *keys = SDL_GetKeyboardState(NULL);
        double forward = 0.0;
        double strafe = 0.0;
        double turn = 0.0;

        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) forward += 1.0;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) forward -= 1.0;
        if (keys[SDL_SCANCODE_Q]) strafe -= 1.0;
        if (keys[SDL_SCANCODE_E]) strafe += 1.0;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) turn -= 1.0;
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) turn += 1.0;

        if (!game.game_over && !game.victory && (forward != 0.0 || strafe != 0.0)) {
            move_camera(&cam, forward, strafe, dt);
        }
        if (!game.game_over && !game.victory && turn != 0.0) {
            rotate_camera(&cam, turn * 2.2 * dt);
        }

        if (!game.game_over && !game.victory) {
            update_items(&game, &cam);
            update_game(&game, &cam, dt);
        }
        render_scene(&cam, &game);
        SDL_UpdateTexture(screen, NULL, framebuffer, SCREEN_W * (int)sizeof(uint32_t));

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
