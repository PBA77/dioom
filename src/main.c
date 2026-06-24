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
#define MAX_PROJECTILES 20
#define MAX_MONSTERS 10
#define MAX_ITEMS 20
#define MAX_TORCHES 26
#define MAX_DECALS 32
#define MAX_PARTICLES 64
#define MAX_DOORS 4
#define MAX_SECRETS 3
#define MAX_LEVEL_ROOMS 10
#define LEVEL_TEST_SEED 0x00C0FFEEu
#define PLAYER_MAX_HEALTH 160
#define START_AMMO 48
#define START_FIREBALL_AMMO 0
#define MAX_FIREBALL_AMMO 24
#define PLAYER_DAMAGE 2
#define SHOT_COOLDOWN_TIME 0.24
#define FIREBALL_COOLDOWN_TIME 0.75
#define WEAPON_FLASH_TIME 0.18
#define FIREBALL_FLASH_TIME 0.26
#define FIREBALL_DIRECT_DAMAGE 5
#define FIREBALL_SPLASH_DAMAGE 4
#define FIREBALL_RADIUS 1.25
#define FOG_DENSITY 0.165
#define FOG_PASS_STRENGTH 1.10
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

enum {
    WEAPON_PISTOL = 0,
    WEAPON_FIREBALL = 1,
};

enum {
    PROJECTILE_ENEMY_BOLT = 0,
    PROJECTILE_PLAYER_FIREBALL = 1,
    PROJECTILE_EXPLOSION = 2,
};

enum {
    PROJECTILE_OWNER_ENEMY = 0,
    PROJECTILE_OWNER_PLAYER = 1,
    PROJECTILE_OWNER_NONE = 2,
};

enum {
    ITEM_HEALTH = 0,
    ITEM_AMMO = 1,
    ITEM_RAPID = 2,
    ITEM_DAMAGE = 3,
    ITEM_FIREBALL = 4,
    ITEM_KEY = 5,
};

enum {
    GENERATOR_ROOMS = 0,
    GENERATOR_TIGHT = 1,
    GENERATOR_BOSS = 2,
    GENERATOR_COUNT = 3,
};

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
    Vec2 patrol[4];
    int patrol_count;
    double alert_timer;
    double strafe_timer;
    int strafe_dir;
    double pain_timer;
    int is_boss;
} Monster;

typedef struct {
    int active;
    int owner;
    int type;
    int damage;
    double radius;
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
    Vec2 pos;
} Torch;

typedef struct {
    int x;
    int y;
    int locked;
    int opening;
    int open;
    double open_amount;
} Door;

typedef struct {
    int x;
    int y;
    int opening;
    int open;
    double open_amount;
} Secret;

typedef struct {
    int active;
    int type;
    Vec2 pos;
    double radius;
    double life;
} Decal;

typedef struct {
    int active;
    Vec2 pos;
    Vec2 vel;
    double life;
    double max_life;
    double size;
    uint32_t color;
} Particle;

typedef struct {
    Monster monsters[MAX_MONSTERS];
    int monster_count;
    Projectile projectiles[MAX_PROJECTILES];
    Item items[MAX_ITEMS];
    Decal decals[MAX_DECALS];
    Particle particles[MAX_PARTICLES];
    Door doors[MAX_DOORS];
    Secret secrets[MAX_SECRETS];
    double time;
    int player_health;
    int ammo;
    int fireball_ammo;
    int keys;
    int selected_weapon;
    int fireball_unlocked;
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
    int show_automap;
    int generator_mode;
    unsigned char discovered[MAP_H][MAP_W];
} GameState;

typedef struct {
    int kind;
    int index;
    double dist;
} SpriteDraw;

typedef struct {
    int active;
    double phase;
    double freq;
    double life;
    double max_life;
    double volume;
} SoundVoice;

static uint32_t framebuffer[SCREEN_W * SCREEN_H];
static uint32_t textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static uint32_t monster_sprites[MONSTER_TYPES][SPRITE_FRAMES][SPRITE_SIZE * SPRITE_SIZE];
static double z_buffer[SCREEN_W];
static double depth_buffer[SCREEN_W * SCREEN_H];
static double glow_buffer[SCREEN_W * SCREEN_H];
static double glow_blur_buffer[SCREEN_W * SCREEN_H];
static double light_buffer[SCREEN_W * SCREEN_H];
static GameState *active_game = NULL;
static SoundVoice sound_voices[12];
static SDL_AudioDeviceID audio_device = 0;
static double audio_rate = 44100.0;
static int level_map[MAP_H][MAP_W];
static Torch torches[MAX_TORCHES];
static uint32_t runtime_level_seed = LEVEL_TEST_SEED;
static int runtime_level_mode = GENERATOR_ROOMS;

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

static double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static double smooth01(double v)
{
    v = clamp01(v);
    return v * v * (3.0 - 2.0 * v);
}

static uint32_t fog_color(void)
{
    return rgb(38, 46, 44);
}

static double fog_amount(double distance)
{
    if (distance <= 0.0) {
        return 0.0;
    }
    return clamp01(1.0 - exp(-distance * FOG_DENSITY));
}

static uint32_t apply_fog(uint32_t color, double distance, double strength)
{
    return mix_color(color, fog_color(), fog_amount(distance) * clamp01(strength));
}

static double luminance(uint32_t color)
{
    double r = (double)((color >> 16) & 0xFFu);
    double g = (double)((color >> 8) & 0xFFu);
    double b = (double)(color & 0xFFu);
    return (r * 0.2126 + g * 0.7152 + b * 0.0722) / 255.0;
}

static uint32_t add_color(uint32_t color, uint32_t add, double amount)
{
    int r = (int)((color >> 16) & 0xFFu) + (int)(((add >> 16) & 0xFFu) * amount);
    int g = (int)((color >> 8) & 0xFFu) + (int)(((add >> 8) & 0xFFu) * amount);
    int b = (int)(color & 0xFFu) + (int)((add & 0xFFu) * amount);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

static uint32_t contrast_color(uint32_t color, double contrast, double brightness)
{
    int cr = (int)((color >> 16) & 0xFFu);
    int cg = (int)((color >> 8) & 0xFFu);
    int cb = (int)(color & 0xFFu);
    int r = (int)((cr - 128) * contrast + 128 + brightness);
    int g = (int)((cg - 128) * contrast + 128 + brightness);
    int b = (int)((cb - 128) * contrast + 128 + brightness);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

static void put_pixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        framebuffer[y * SCREEN_W + x] = color;
    }
}

static void add_glow(int x, int y, double amount)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        int idx = y * SCREEN_W + x;
        glow_buffer[idx] += amount;
        if (glow_buffer[idx] > 1.0) {
            glow_buffer[idx] = 1.0;
        }
    }
}

static void add_light(int x, int y, double amount)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        int idx = y * SCREEN_W + x;
        light_buffer[idx] += amount;
        if (light_buffer[idx] > 1.0) {
            light_buffer[idx] = 1.0;
        }
    }
}

static void reset_render_buffers(void)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        depth_buffer[i] = 1e30;
        glow_buffer[i] = 0.0;
        glow_blur_buffer[i] = 0.0;
        light_buffer[i] = 0.0;
    }
}

static void audio_callback(void *userdata, uint8_t *stream, int len)
{
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    int samples = len / (int)sizeof(int16_t);

    for (int i = 0; i < samples; ++i) {
        double sample = 0.0;
        for (int v = 0; v < (int)(sizeof(sound_voices) / sizeof(sound_voices[0])); ++v) {
            SoundVoice *voice = &sound_voices[v];
            if (!voice->active) {
                continue;
            }
            double t = voice->max_life > 0.0 ? voice->life / voice->max_life : 0.0;
            double env = clamp01(t);
            sample += sin(voice->phase) * voice->volume * env;
            voice->phase += (M_PI * 2.0 * voice->freq) / audio_rate;
            voice->life -= 1.0 / audio_rate;
            if (voice->life <= 0.0) {
                voice->active = 0;
            }
        }
        out[i] = (int16_t)(clamp01(sample * 0.5 + 0.5) * 65534.0 - 32767.0);
    }
}

static int init_audio(void)
{
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_device) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return 0;
    }
    audio_rate = (double)have.freq;
    SDL_PauseAudioDevice(audio_device, 0);
    return 1;
}

static void play_sound(double freq, double duration, double volume)
{
    if (!audio_device) {
        return;
    }
    SDL_LockAudioDevice(audio_device);
    for (int i = 0; i < (int)(sizeof(sound_voices) / sizeof(sound_voices[0])); ++i) {
        SoundVoice *voice = &sound_voices[i];
        if (!voice->active) {
            voice->active = 1;
            voice->phase = 0.0;
            voice->freq = freq;
            voice->life = duration;
            voice->max_life = duration;
            voice->volume = volume;
            break;
        }
    }
    SDL_UnlockAudioDevice(audio_device);
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
    if (active_game) {
        for (int i = 0; i < MAX_DOORS; ++i) {
            const Door *door = &active_game->doors[i];
            if (door->x == x && door->y == y && !door->open) {
                return door->locked ? 8 : 7;
            }
        }
        for (int i = 0; i < MAX_SECRETS; ++i) {
            const Secret *secret = &active_game->secrets[i];
            if (secret->x == x && secret->y == y && !secret->open) {
                return 6;
            }
        }
    }
    return level_map[y][x];
}

static double torch_light_at(double x, double y, double time)
{
    double light = 0.0;

    for (int i = 0; i < MAX_TORCHES; ++i) {
        double dx = x - torches[i].pos.x;
        double dy = y - torches[i].pos.y;
        double dist2 = dx * dx + dy * dy;
        if (dist2 > 18.0) {
            continue;
        }

        double fade = 1.0 - smooth01((dist2 - 5.0) / 13.0);
        double flicker = 1.04 + sin(time * 7.0 + i * 1.73) * 0.10 + sin(time * 13.0 + i * 0.41) * 0.05;
        light += flicker * 1.45 * fade / (1.0 + dist2 * 0.62);
    }

    return clamp01(light);
}

static uint32_t apply_torch_light(uint32_t color, double base_light, double torch_light, double tint)
{
    uint32_t lit = shade(color, base_light + torch_light);
    return mix_color(lit, rgb(255, 132, 48), clamp01(torch_light * tint));
}

static double wall_bump_light(int tex_idx, int tex_x, int tex_y, double hit_x, double hit_y)
{
    int lx = (tex_x - 1) & (TEX_SIZE - 1);
    int rx = (tex_x + 1) & (TEX_SIZE - 1);
    int uy = (tex_y - 1) & (TEX_SIZE - 1);
    int dy = (tex_y + 1) & (TEX_SIZE - 1);
    double h_l = luminance(textures[tex_idx][tex_y * TEX_SIZE + lx]);
    double h_r = luminance(textures[tex_idx][tex_y * TEX_SIZE + rx]);
    double h_u = luminance(textures[tex_idx][uy * TEX_SIZE + tex_x]);
    double h_d = luminance(textures[tex_idx][dy * TEX_SIZE + tex_x]);
    double gx = h_r - h_l;
    double gy = h_d - h_u;
    double light_dir = 0.0;

    for (int i = 0; i < MAX_TORCHES; ++i) {
        double dx = torches[i].pos.x - hit_x;
        double dyw = torches[i].pos.y - hit_y;
        double dist2 = dx * dx + dyw * dyw;
        if (dist2 < 12.0) {
            light_dir += (gx * dx + gy * dyw) / (1.0 + dist2);
        }
    }

    return clamp01(0.5 + light_dir * 2.8);
}

static void render_floor_ceiling(const Camera *cam, const GameState *game)
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
                framebuffer[y * SCREEN_W + x] = apply_fog(rgb(18, 17, 16), 9.0, 0.86);
                framebuffer[(SCREEN_H - y - 1) * SCREEN_W + x] = apply_fog(rgb(9, 9, 10), 9.0, 0.82);
            }
            continue;
        }

        double pos_z = 0.5 * SCREEN_H;
        double row_distance = pos_z / p;
        double floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / SCREEN_W;
        double floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / SCREEN_W;
        double floor_x = cam->pos.x + row_distance * ray_dir_x0;
        double floor_y = cam->pos.y + row_distance * ray_dir_y0;
        double light = 0.045 + 0.30 / (1.0 + row_distance * 0.17);

        for (int x = 0; x < SCREEN_W; ++x) {
            int cell_x = (int)floor(floor_x);
            int cell_y = (int)floor(floor_y);
            int tx = (int)(TEX_SIZE * (floor_x - cell_x)) & (TEX_SIZE - 1);
            int ty = (int)(TEX_SIZE * (floor_y - cell_y)) & (TEX_SIZE - 1);

            uint32_t floor_color = textures[floor_tex][ty * TEX_SIZE + tx];
            uint32_t ceil_color = textures[ceil_tex][ty * TEX_SIZE + tx];
            double torch_light = torch_light_at(floor_x, floor_y, game->time);
            uint32_t lit_floor = apply_torch_light(floor_color, light * 0.48, torch_light * 0.70, 0.26);
            uint32_t lit_ceil = apply_torch_light(ceil_color, light * 0.18, torch_light * 0.26, 0.10);
            framebuffer[y * SCREEN_W + x] = apply_fog(lit_floor, row_distance, 0.86);
            framebuffer[(SCREEN_H - y - 1) * SCREEN_W + x] = apply_fog(lit_ceil, row_distance, 0.76);
            depth_buffer[y * SCREEN_W + x] = row_distance;
            depth_buffer[(SCREEN_H - y - 1) * SCREEN_W + x] = row_distance;
            add_light(x, y, torch_light * 0.18);
            add_light(x, SCREEN_H - y - 1, torch_light * 0.08);

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
    double monster_scale = monster->is_boss ? 1.35 : 1.0;
    if (!project_sprite(cam, monster->pos, monster_scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;

    int bob = (int)(sin(monster->pos.x * 2.3 + monster->pos.y * 1.7) * (monster->is_boss ? 3.0 : 1.5));
    int draw_start_y = -sprite_h / 2 + SCREEN_H / 2 + bob;
    int draw_end_y = sprite_h / 2 + SCREEN_H / 2 + bob;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    int frame = monster_frame_for_camera(cam, monster);
    double light = (monster->is_boss ? 1.15 : 1.0) / (1.0 + depth * 0.055);

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
                uint32_t lit = shade(color, light);
                if (monster->pain_timer > 0.0) {
                    lit = mix_color(lit, rgb(255, 210, 118), clamp01(monster->pain_timer / 0.18));
                    add_glow(stripe, y, monster->pain_timer * 0.35);
                }
                framebuffer[y * SCREEN_W + stripe] = apply_fog(lit, depth, 0.82);
                depth_buffer[y * SCREEN_W + stripe] = depth;
            }
        }
    }
}

static uint32_t projectile_texel(const Projectile *projectile, int tex_x, int tex_y)
{
    double dx = (tex_x + 0.5) - PROJECTILE_SIZE / 2.0;
    double dy = (tex_y + 0.5) - PROJECTILE_SIZE / 2.0;
    double dist = sqrt(dx * dx + dy * dy);

    if (projectile->type == PROJECTILE_EXPLOSION) {
        double ring = PROJECTILE_SIZE * (0.22 + projectile->life * 0.85);
        if (dist > PROJECTILE_SIZE * 0.50 || dist < ring * 0.30) {
            return 0;
        }
        if (((tex_x * 5 + tex_y * 3) & 7) < 3) {
            return rgb(255, 188, 66);
        }
        return dist < ring ? rgb(255, 106, 28) : rgb(126, 42, 20);
    }

    if (dist > PROJECTILE_SIZE * 0.48) {
        return 0;
    }

    if (projectile->type == PROJECTILE_PLAYER_FIREBALL) {
        if (dist < PROJECTILE_SIZE * 0.20) {
            return rgb(255, 246, 156);
        }
        if (((tex_x * 13 + tex_y * 9) & 15) < 7) {
            return rgb(255, 142, 34);
        }
        return rgb(198, 52, 22);
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
    double scale = projectile->type == PROJECTILE_EXPLOSION ? projectile->radius * 0.90 :
                   projectile->type == PROJECTILE_PLAYER_FIREBALL ? 0.48 : 0.34;
    if (!project_sprite(cam, projectile->pos, scale, &sprite_screen_x, &sprite_h, &depth)) {
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

    double light = projectile->type == PROJECTILE_EXPLOSION ? 1.55 :
                   projectile->type == PROJECTILE_PLAYER_FIREBALL ? 1.35 / (1.0 + depth * 0.02) :
                   1.15 / (1.0 + depth * 0.03);
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

            uint32_t color = projectile_texel(projectile, tex_x, tex_y);
            if (color != 0) {
                double fog_strength = projectile->type == PROJECTILE_EXPLOSION ? 0.12 : 0.28;
                framebuffer[y * SCREEN_W + stripe] = apply_fog(shade(color, light), depth, fog_strength);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                if (projectile->type == PROJECTILE_PLAYER_FIREBALL || projectile->type == PROJECTILE_EXPLOSION) {
                    add_glow(stripe, y, projectile->type == PROJECTILE_EXPLOSION ? 0.85 : 0.55);
                    add_light(stripe, y, projectile->type == PROJECTILE_EXPLOSION ? 0.35 : 0.22);
                }
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
    case ITEM_HEALTH:
        if (abs(tex_x - PROJECTILE_SIZE / 2) < 4 || abs(tex_y - PROJECTILE_SIZE / 2) < 4) {
            return rgb(245, 236, 218);
        }
        return border ? rgb(94, 12, 18) : rgb(192, 24, 34);
    case ITEM_AMMO:
        if ((tex_x / 4) & 1) {
            return rgb(234, 192, 74);
        }
        return border ? rgb(92, 54, 22) : rgb(166, 112, 38);
    case ITEM_RAPID:
        if (((tex_x + tex_y) & 7) < 3) {
            return rgb(136, 232, 255);
        }
        return border ? rgb(20, 62, 94) : rgb(42, 152, 210);
    case ITEM_DAMAGE:
        if (dist < PROJECTILE_SIZE * 0.18) {
            return rgb(255, 244, 160);
        }
        return border ? rgb(86, 20, 104) : rgb(186, 64, 230);
    case ITEM_FIREBALL:
        if (dist < PROJECTILE_SIZE * 0.18 || ((tex_x + tex_y) & 7) == 0) {
            return rgb(255, 228, 118);
        }
        return border ? rgb(88, 28, 16) : rgb(230, 82, 24);
    default:
        if (tex_y > 12 && tex_y < 18 && tex_x > 8 && tex_x < 24) {
            return rgb(238, 202, 86);
        }
        if (tex_x > 18 && tex_x < 22 && tex_y > 16 && tex_y < 27) {
            return rgb(180, 128, 42);
        }
        if (dist < PROJECTILE_SIZE * 0.18) {
            return rgb(255, 232, 128);
        }
        return border ? rgb(88, 64, 24) : rgb(210, 164, 58);
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
                framebuffer[y * SCREEN_W + stripe] = apply_fog(shade(color, light), depth, 0.72);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                if (item->type == ITEM_FIREBALL || item->type == ITEM_RAPID || item->type == ITEM_DAMAGE) {
                    add_glow(stripe, y, item->type == ITEM_FIREBALL ? 0.22 : 0.12);
                }
            }
        }
    }
}

static uint32_t torch_texel(int tex_x, int tex_y, double time, int index)
{
    double flicker = sin(time * 16.0 + index * 0.71) * 1.6 + sin(time * 29.0 + index * 1.37) * 0.9;
    double flame_x = tex_x - (PROJECTILE_SIZE / 2.0 + flicker);
    double flame_y = tex_y - 10.0;
    double flame_width = 8.5 - tex_y * 0.16 + sin(tex_y * 0.7 + time * 18.0 + index) * 0.9;

    if (tex_y >= 2 && tex_y <= 23 && flame_width > 1.0) {
        double flame_shape = fabs(flame_x) / flame_width + fabs(flame_y) / 18.0;
        if (flame_shape < 0.74) {
            if (flame_shape < 0.36) {
                return rgb(255, 238, 132);
            }
            if (((tex_x + tex_y + index) & 3) == 0) {
                return rgb(255, 172, 52);
            }
            return rgb(222, 78, 24);
        }
    }

    if (tex_y >= 20 && tex_y <= 25 && tex_x >= 9 && tex_x <= 23) {
        return rgb(92, 58, 36);
    }
    if (tex_y >= 24 && tex_y <= 30 && tex_x >= 14 && tex_x <= 18) {
        return rgb(58, 50, 46);
    }
    if (tex_y >= 28 && tex_y <= 31 && tex_x >= 11 && tex_x <= 21) {
        return rgb(42, 38, 36);
    }

    return 0;
}

static void render_torch(const Camera *cam, const Torch *torch, int index, double time)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    if (!project_sprite(cam, torch->pos, 0.50, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h * 2 / 3;
    if (sprite_w < 1) {
        return;
    }

    int draw_start_y = SCREEN_H / 2 - sprite_h * 3 / 4;
    int draw_end_y = draw_start_y + sprite_h;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = 1.90 / (1.0 + depth * 0.020);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - draw_start_y) * PROJECTILE_SIZE;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }

            double gx = (tex_x - PROJECTILE_SIZE / 2.0) / 15.0;
            double gy = (tex_y - 12.0) / 18.0;
            double glow = clamp01(1.0 - sqrt(gx * gx + gy * gy));
            if (glow > 0.0) {
                double amount = glow * 0.46 / (1.0 + depth * 0.035);
                uint32_t current = framebuffer[y * SCREEN_W + stripe];
                framebuffer[y * SCREEN_W + stripe] = mix_color(current, rgb(255, 146, 42), clamp01(amount));
                add_glow(stripe, y, amount * 0.75);
                add_light(stripe, y, amount * 0.28);
            }

            uint32_t color = torch_texel(tex_x, tex_y, time, index);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = apply_fog(shade(color, light), depth, 0.20);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                add_glow(stripe, y, 0.42);
            }
        }
    }
}

static void render_world_sprites(const Camera *cam, const GameState *game)
{
    SpriteDraw draws[MAX_MONSTERS + MAX_PROJECTILES + MAX_ITEMS + MAX_TORCHES + MAX_PARTICLES];
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

    for (int i = 0; i < MAX_TORCHES; ++i) {
        double dx = torches[i].pos.x - cam->pos.x;
        double dy = torches[i].pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){3, i, dx * dx + dy * dy};
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        const Particle *particle = &game->particles[i];
        if (!particle->active) {
            continue;
        }
        double dx = particle->pos.x - cam->pos.x;
        double dy = particle->pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){4, i, dx * dx + dy * dy};
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
        } else if (draw.kind == 2) {
            render_projectile(cam, &game->projectiles[draw.index]);
        } else if (draw.kind == 3) {
            render_torch(cam, &torches[draw.index], draw.index, game->time);
        } else {
            const Particle *particle = &game->particles[draw.index];
            int sprite_screen_x;
            int sprite_h;
            double depth;
            if (!project_sprite(cam, particle->pos, particle->size, &sprite_screen_x, &sprite_h, &depth)) {
                continue;
            }
            int sprite_w = sprite_h;
            int start_x = sprite_screen_x - sprite_w / 2;
            int end_x = sprite_screen_x + sprite_w / 2;
            int start_y = SCREEN_H / 2 - sprite_h / 2;
            int end_y = SCREEN_H / 2 + sprite_h / 2;
            if (start_x < 0) start_x = 0;
            if (end_x >= SCREEN_W) end_x = SCREEN_W - 1;
            if (start_y < 0) start_y = 0;
            if (end_y >= SCREEN_H - 14) end_y = SCREEN_H - 15;

            double life_t = particle->max_life > 0.0 ? particle->life / particle->max_life : 0.0;
            for (int y = start_y; y <= end_y; ++y) {
                double ny = (y - (SCREEN_H / 2.0)) / (sprite_h * 0.5);
                for (int x = start_x; x <= end_x; ++x) {
                    if (depth >= z_buffer[x] + 0.18) {
                        continue;
                    }
                    double nx = (x - sprite_screen_x) / (sprite_w * 0.5);
                    double d = nx * nx + ny * ny;
                    if (d > 1.0) {
                        continue;
                    }
                    double soft = clamp01((z_buffer[x] - depth + 0.22) / 0.50);
                    double a = (1.0 - d) * life_t * soft * 0.42;
                    int idx = y * SCREEN_W + x;
                    framebuffer[idx] = mix_color(framebuffer[idx], particle->color, a);
                    add_glow(x, y, a * 0.16);
                }
            }
        }
    }
}

static void render_volumetric_fog(const Camera *cam, const GameState *game)
{
    uint32_t fog = fog_color();

    for (int x = 0; x < SCREEN_W; ++x) {
        double wall_depth = z_buffer[x];
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;

        for (int y = 0; y < SCREEN_H - 14; ++y) {
            int from_horizon = abs(y - SCREEN_H / 2);
            double pixel_depth = wall_depth;
            if (from_horizon > 0) {
                double row_depth = (0.5 * SCREEN_H) / from_horizon;
                if (row_depth < pixel_depth) {
                    pixel_depth = row_depth;
                }
            }

            double pixel_fog = fog_amount(pixel_depth) * FOG_PASS_STRENGTH;
            if (pixel_fog <= 0.002) {
                continue;
            }

            double dy = fabs((y - SCREEN_H * 0.5) / (SCREEN_H * 0.5));
            double horizon = clamp01(1.0 - dy);
            double lower = y > SCREEN_H / 2 ? (y - SCREEN_H / 2.0) / (SCREEN_H / 2.0) : 0.0;
            double ground = lower * lower;
            double wave_a = sin(x * 0.047 + y * 0.013 + game->time * 0.72);
            double wave_b = sin(x * 0.021 - y * 0.096 - game->time * 0.34);
            double wave_c = sin(x * 0.111 + y * 0.041 + game->time * 0.19);
            double cloud = clamp01((wave_a + wave_b * 0.85 + wave_c * 0.55 + 1.02) * 0.36);
            cloud = cloud * cloud * (1.45 - cloud * 0.45);
            double amount = pixel_fog * (0.08 + horizon * 0.34 + ground * 1.55) * (0.34 + cloud * 1.58);

            if (amount > 0.004) {
                uint32_t color = framebuffer[y * SCREEN_W + x];
                framebuffer[y * SCREEN_W + x] = mix_color(color, fog, clamp01(amount));
            }

            if (pixel_depth > 0.35) {
                double torch_scatter = 0.0;
                int steps = 4;
                for (int sample = 1; sample <= steps; ++sample) {
                    double t = pixel_depth * (sample / (double)(steps + 1));
                    Vec2 p = {
                        cam->pos.x + ray_dir_x * t,
                        cam->pos.y + ray_dir_y * t,
                    };
                    torch_scatter += torch_light_at(p.x, p.y, game->time) * (1.0 - sample * 0.10);
                }

                torch_scatter = clamp01(torch_scatter / steps);
                double warm_amount = torch_scatter * pixel_fog * (0.10 + ground * 0.55 + cloud * 0.28);
                if (warm_amount > 0.003) {
                    uint32_t color = framebuffer[y * SCREEN_W + x];
                    framebuffer[y * SCREEN_W + x] = mix_color(color, rgb(166, 78, 32), clamp01(warm_amount));
                }
            }
        }
    }
}

static void render_light_buffer(void)
{
    for (int y = 0; y < SCREEN_H - 14; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            int idx = y * SCREEN_W + x;
            double light = light_buffer[idx];
            if (light > 0.002) {
                framebuffer[idx] = mix_color(framebuffer[idx], rgb(255, 134, 48), clamp01(light * 0.18));
            }
        }
    }
}

static void render_bloom(void)
{
    for (int y = 1; y < SCREEN_H - 15; ++y) {
        for (int x = 1; x < SCREEN_W - 1; ++x) {
            int idx = y * SCREEN_W + x;
            double s =
                glow_buffer[idx] * 0.34 +
                glow_buffer[idx - 1] * 0.12 +
                glow_buffer[idx + 1] * 0.12 +
                glow_buffer[idx - SCREEN_W] * 0.12 +
                glow_buffer[idx + SCREEN_W] * 0.12 +
                glow_buffer[idx - SCREEN_W - 1] * 0.045 +
                glow_buffer[idx - SCREEN_W + 1] * 0.045 +
                glow_buffer[idx + SCREEN_W - 1] * 0.045 +
                glow_buffer[idx + SCREEN_W + 1] * 0.045;
            glow_blur_buffer[idx] = s;
        }
    }

    for (int y = 2; y < SCREEN_H - 16; ++y) {
        for (int x = 2; x < SCREEN_W - 2; ++x) {
            int idx = y * SCREEN_W + x;
            double s =
                glow_blur_buffer[idx] * 0.36 +
                glow_blur_buffer[idx - 2] * 0.08 +
                glow_blur_buffer[idx + 2] * 0.08 +
                glow_blur_buffer[idx - SCREEN_W * 2] * 0.08 +
                glow_blur_buffer[idx + SCREEN_W * 2] * 0.08 +
                glow_blur_buffer[idx - 1] * 0.08 +
                glow_blur_buffer[idx + 1] * 0.08 +
                glow_blur_buffer[idx - SCREEN_W] * 0.08 +
                glow_blur_buffer[idx + SCREEN_W] * 0.08;
            if (s > 0.004) {
                framebuffer[idx] = add_color(framebuffer[idx], rgb(255, 122, 40), s * 0.55);
            }
        }
    }
}

static void render_color_grade(void)
{
    for (int y = 0; y < SCREEN_H - 14; ++y) {
        double ny = (y - SCREEN_H * 0.5) / (SCREEN_H * 0.5);
        for (int x = 0; x < SCREEN_W; ++x) {
            int idx = y * SCREEN_W + x;
            double nx = (x - SCREEN_W * 0.5) / (SCREEN_W * 0.5);
            double vignette = clamp01((nx * nx + ny * ny) * 0.34);
            uint32_t c = framebuffer[idx];
            c = contrast_color(c, 1.12, -5.0);
            c = mix_color(c, rgb(20, 28, 30), vignette * 0.45);
            if (light_buffer[idx] > 0.03 || glow_buffer[idx] > 0.03) {
                c = mix_color(c, rgb(255, 132, 48), clamp01((light_buffer[idx] + glow_buffer[idx]) * 0.05));
            }
            framebuffer[idx] = c;
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
            framebuffer[y * SCREEN_W + x] = mix_color(color, rgb(190, 18, 18), clamp01(intensity * 0.55));
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
    uint32_t core = mix_color(rgb(255, 120, 28), rgb(255, 245, 170), clamp01(t));
    int jitter = (int)(sin(game->time * 80.0) * 2.0);
    draw_line(SCREEN_W / 2 + 6, SCREEN_H - 41, SCREEN_W / 2 + jitter, SCREEN_H / 2 + 2, core);
    draw_line(SCREEN_W / 2 + 7, SCREEN_H - 41, SCREEN_W / 2 + jitter + 1, SCREEN_H / 2 + 2, rgb(255, 210, 80));
}

static void render_weapon(const GameState *game)
{
    int bob = (int)(sin(game->time * 7.5) * 2.0);
    double flash_time = game->selected_weapon == WEAPON_FIREBALL ? FIREBALL_FLASH_TIME : WEAPON_FLASH_TIME;
    double fire_t = game->weapon_flash > 0.0 ? game->weapon_flash / flash_time : 0.0;
    int recoil_y = (int)(fire_t * fire_t * 15.0);
    int recoil_x = (int)(sin(game->time * 120.0) * fire_t * 4.0);
    int x = SCREEN_W / 2 - 24 + recoil_x;
    int y = SCREEN_H - 42 + bob + recoil_y;

    if (game->selected_weapon == WEAPON_FIREBALL) {
        fill_rect(x + 22, y + 14, 20, 28, rgb(58, 34, 24));
        fill_rect(x + 25, y + 8, 14, 30, rgb(108, 58, 32));
        fill_rect(x + 18, y + 4, 28, 12, rgb(80, 34, 22));
        fill_rect(x + 22, y + 6, 20, 7, rgb(168, 66, 28));
        fill_rect(x + 27, y + 3, 10, 10, rgb(255, 140, 36));
        fill_rect(x + 30, y + 5, 4, 4, rgb(255, 236, 128));
        if (game->weapon_flash > 0.0) {
            int flash = 16 + (int)(fire_t * 30.0);
            fill_rect(x + 32 - flash / 2, y - 8, flash, flash, rgb(255, 98, 24));
            fill_rect(x + 32 - flash / 4, y - 2, flash / 2, flash / 2, rgb(255, 238, 120));
        }
        return;
    }

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

    uint32_t pistol_slot = game->selected_weapon == WEAPON_PISTOL ? rgb(210, 180, 90) : rgb(62, 58, 48);
    uint32_t fire_slot = game->selected_weapon == WEAPON_FIREBALL ? rgb(230, 94, 32) : rgb(64, 42, 32);
    fill_rect(188, SCREEN_H - 12, 12, 10, pistol_slot);
    fill_rect(191, SCREEN_H - 9, 6, 4, rgb(24, 24, 24));
    fill_rect(204, SCREEN_H - 12, 12, 10, game->fireball_unlocked ? fire_slot : rgb(34, 30, 28));
    fill_rect(207, SCREEN_H - 9, 6, 4, game->fireball_unlocked ? rgb(255, 162, 54) : rgb(18, 18, 18));
    int fire_pips = game->fireball_ammo > 10 ? 10 : game->fireball_ammo;
    for (int i = 0; i < fire_pips; ++i) {
        fill_rect(222 + i * 4, SCREEN_H - 10, 2, 6, rgb(232, 92, 28));
    }
    for (int i = 0; i < GENERATOR_COUNT; ++i) {
        uint32_t c = i == game->generator_mode ? rgb(92, 190, 160) : rgb(42, 48, 46);
        fill_rect(248 + i * 7, SCREEN_H - 11, 5, 8, c);
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
            uint32_t c = item->type == ITEM_HEALTH ? rgb(210, 42, 48) :
                         item->type == ITEM_AMMO ? rgb(220, 170, 64) :
                         item->type == ITEM_RAPID ? rgb(54, 174, 230) :
                         item->type == ITEM_FIREBALL ? rgb(230, 88, 28) :
                         item->type == ITEM_KEY ? rgb(238, 202, 86) : rgb(190, 70, 230);
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

static void render_full_automap(const Camera *cam, const GameState *game)
{
    const int cell = 7;
    const int ox = (SCREEN_W - MAP_W * cell) / 2;
    const int oy = (SCREEN_H - 14 - MAP_H * cell) / 2;

    fill_rect(0, 0, SCREEN_W, SCREEN_H - 14, rgb(4, 5, 6));
    fill_rect(ox - 4, oy - 4, MAP_W * cell + 8, MAP_H * cell + 8, rgb(15, 13, 12));

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            uint32_t c = rgb(7, 8, 9);
            if (game->discovered[y][x]) {
                int wall = map_at(x, y);
                c = wall ? rgb(88, 70, 58) : rgb(28, 28, 25);
                if (wall == 7) c = rgb(126, 82, 40);
                if (wall == 8) c = rgb(132, 102, 34);
                if (wall == 6) c = rgb(66, 50, 72);
            }
            fill_rect(ox + x * cell, oy + y * cell, cell - 1, cell - 1, c);
        }
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *item = &game->items[i];
        int ix = (int)item->pos.x;
        int iy = (int)item->pos.y;
        if (item->active && ix >= 0 && ix < MAP_W && iy >= 0 && iy < MAP_H && game->discovered[iy][ix]) {
            uint32_t c = item->type == ITEM_KEY ? rgb(242, 210, 74) :
                         item->type == ITEM_HEALTH ? rgb(218, 42, 54) :
                         item->type == ITEM_FIREBALL ? rgb(238, 94, 30) :
                         item->type == ITEM_RAPID ? rgb(54, 180, 230) :
                         item->type == ITEM_DAMAGE ? rgb(190, 70, 230) : rgb(220, 170, 64);
            fill_rect(ox + ix * cell + 2, oy + iy * cell + 2, 3, 3, c);
        }
    }

    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        int mx = (int)monster->pos.x;
        int my = (int)monster->pos.y;
        if (monster->active && mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H && game->discovered[my][mx]) {
            fill_rect(ox + mx * cell + 1, oy + my * cell + 1, cell - 3, cell - 3,
                      monster->is_boss ? rgb(210, 42, 32) : rgb(150, 32, 28));
        }
    }

    int px = (int)cam->pos.x;
    int py = (int)cam->pos.y;
    fill_rect(ox + px * cell + 1, oy + py * cell + 1, cell - 2, cell - 2, rgb(236, 220, 72));
    put_pixel(ox + px * cell + cell / 2 + (int)(cam->dir.x * 3.0),
              oy + py * cell + cell / 2 + (int)(cam->dir.y * 3.0),
              rgb(255, 248, 160));
}

static void render_pause_overlay(void)
{
    fill_rect(SCREEN_W / 2 - 42, 52, 84, 34, rgb(10, 10, 12));
    fill_rect(SCREEN_W / 2 - 24, 60, 12, 18, rgb(190, 166, 100));
    fill_rect(SCREEN_W / 2 + 12, 60, 12, 18, rgb(190, 166, 100));
    fill_rect(SCREEN_W / 2 - 32, 90, 64, 4, rgb(84, 72, 50));
}

static void render_screen_ellipse(int cx, int cy, int rx, int ry, double depth, uint32_t color, double strength)
{
    if (rx < 1 || ry < 1) {
        return;
    }

    int start_x = cx - rx;
    int end_x = cx + rx;
    int start_y = cy - ry;
    int end_y = cy + ry;
    if (start_x < 0) start_x = 0;
    if (end_x >= SCREEN_W) end_x = SCREEN_W - 1;
    if (start_y < 0) start_y = 0;
    if (end_y >= SCREEN_H - 14) end_y = SCREEN_H - 15;

    for (int y = start_y; y <= end_y; ++y) {
        double ny = (y - cy) / (double)ry;
        for (int x = start_x; x <= end_x; ++x) {
            if (depth >= z_buffer[x] + 0.10) {
                continue;
            }
            double nx = (x - cx) / (double)rx;
            double d = nx * nx + ny * ny;
            if (d > 1.0) {
                continue;
            }
            double a = (1.0 - d) * strength;
            int idx = y * SCREEN_W + x;
            framebuffer[idx] = mix_color(framebuffer[idx], color, clamp01(a));
        }
    }
}

static void render_dynamic_shadows(const Camera *cam, const GameState *game)
{
    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }
        int sx;
        int sh;
        double depth;
        if (project_sprite(cam, monster->pos, 1.0, &sx, &sh, &depth)) {
            int scale = monster->is_boss ? 2 : 1;
            render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2 - 3, sh * scale / 3, sh * scale / 14 + 1, depth, rgb(0, 0, 0), monster->is_boss ? 0.46 : 0.34);
        }
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }
        int sx;
        int sh;
        double depth;
        if (project_sprite(cam, item->pos, 0.42, &sx, &sh, &depth)) {
            render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2 - 1, sh / 4, sh / 14 + 1, depth, rgb(0, 0, 0), 0.22);
        }
    }
}

static void render_decals(const Camera *cam, const GameState *game)
{
    for (int i = 0; i < MAX_DECALS; ++i) {
        const Decal *decal = &game->decals[i];
        if (!decal->active) {
            continue;
        }
        int sx;
        int sh;
        double depth;
        if (!project_sprite(cam, decal->pos, decal->radius, &sx, &sh, &depth)) {
            continue;
        }
        double fade = clamp01(decal->life / 30.0);
        render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2, sh / 2, sh / 8 + 1, depth, rgb(38, 14, 8), 0.42 * fade);
        render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2, sh / 4, sh / 16 + 1, depth, rgb(120, 42, 18), 0.18 * fade);
    }
}

static void render_scene(const Camera *cam, const GameState *game)
{
    reset_render_buffers();
    render_floor_ceiling(cam, game);

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

        double hit_x = cam->pos.x + ray_dir_x * perp_wall_dist;
        double hit_y = cam->pos.y + ray_dir_y * perp_wall_dist;

        int tex_x = (int)(wall_x * TEX_SIZE);
        if ((side == 0 && ray_dir_x > 0.0) || (side == 1 && ray_dir_y < 0.0)) {
            tex_x = TEX_SIZE - tex_x - 1;
        }

        int tex_idx = (wall - 1) % TEX_COUNT;
        double step = (double)TEX_SIZE / line_h;
        double tex_pos = (draw_start - SCREEN_H / 2.0 + line_h / 2.0) * step;
        double light = side == 1 ? 0.17 : 0.27;
        light *= 1.0 / (1.0 + perp_wall_dist * 0.12);
        double torch_light = torch_light_at(hit_x, hit_y, game->time) * (side == 1 ? 0.92 : 1.08);
        z_buffer[x] = perp_wall_dist;

        for (int y = draw_start; y <= draw_end; ++y) {
            int tex_y = ((int)tex_pos) & (TEX_SIZE - 1);
            tex_pos += step;
            uint32_t color = textures[tex_idx][tex_y * TEX_SIZE + tex_x];
            double bump = wall_bump_light(tex_idx, tex_x, tex_y, hit_x, hit_y);
            uint32_t lit = apply_torch_light(color, light * (0.84 + bump * 0.22), torch_light * (0.72 + bump * 0.50), 0.30);
            framebuffer[y * SCREEN_W + x] = apply_fog(lit, perp_wall_dist, 1.0);
            depth_buffer[y * SCREEN_W + x] = perp_wall_dist;
            add_light(x, y, torch_light * 0.10);
        }
    }

    render_decals(cam, game);
    render_dynamic_shadows(cam, game);
    render_world_sprites(cam, game);
    render_volumetric_fog(cam, game);
    render_light_buffer();
    render_bloom();
    render_color_grade();
    render_hit_flash(game);
    render_crosshair();
    render_shot_trace(game);
    render_weapon(game);
    render_hud(game);
    render_minimap(cam, game);
    if (game->show_automap) {
        render_full_automap(cam, game);
        render_hud(game);
    }
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

static int monster_max_hp(int type)
{
    switch (type) {
    case 0: return 7;
    case 2: return 3;
    default: return 4;
    }
}

static double monster_patrol_speed(int type)
{
    switch (type) {
    case 0: return 0.82;
    case 2: return 1.32;
    default: return 1.05;
    }
}

static double monster_chase_speed(int type)
{
    switch (type) {
    case 0: return 1.15;
    case 2: return 2.05;
    default: return 1.55;
    }
}

static double monster_attack_speed(int type)
{
    switch (type) {
    case 0: return 1.05;
    case 2: return 2.25;
    default: return 1.65;
    }
}

static double monster_retreat_speed(int type)
{
    switch (type) {
    case 0: return 0.75;
    case 2: return 1.55;
    default: return 1.25;
    }
}

static double monster_preferred_distance(int type)
{
    switch (type) {
    case 0: return 3.2;
    case 2: return 2.4;
    default: return 4.8;
    }
}

static double monster_shot_cooldown(int type, int index)
{
    double offset = (index % 3) * 0.14;
    switch (type) {
    case 0: return 1.55 + offset;
    case 2: return 1.35 + offset;
    default: return 1.15 + offset;
    }
}

static int monster_projectile_damage(int type)
{
    switch (type) {
    case 0: return 18;
    case 2: return 8;
    default: return 12;
    }
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

typedef struct {
    uint32_t state;
} LevelRng;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} LevelRoom;

static uint32_t rng_next(LevelRng *rng)
{
    rng->state = rng->state * 1664525u + 1013904223u;
    return rng->state;
}

static int rng_range(LevelRng *rng, int min, int max)
{
    return min + (int)(rng_next(rng) % (uint32_t)(max - min + 1));
}

static int level_wall_tex(int x, int y, uint32_t seed)
{
    uint32_t v = (uint32_t)(x * 73856093u) ^ (uint32_t)(y * 19349663u) ^ seed;
    return 1 + (int)(v % 7u);
}

static void level_fill_walls(uint32_t seed)
{
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            level_map[y][x] = level_wall_tex(x, y, seed);
        }
    }
}

static void carve_tile(int x, int y)
{
    if (x > 0 && x < MAP_W - 1 && y > 0 && y < MAP_H - 1) {
        level_map[y][x] = 0;
    }
}

static void carve_room(LevelRoom room)
{
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            carve_tile(x, y);
        }
    }
}

static void carve_h_corridor(int x0, int x1, int y)
{
    if (x0 > x1) {
        int t = x0;
        x0 = x1;
        x1 = t;
    }
    for (int x = x0; x <= x1; ++x) {
        carve_tile(x, y);
    }
}

static void carve_v_corridor(int x, int y0, int y1)
{
    if (y0 > y1) {
        int t = y0;
        y0 = y1;
        y1 = t;
    }
    for (int y = y0; y <= y1; ++y) {
        carve_tile(x, y);
    }
}

static Vec2 room_center(LevelRoom room)
{
    return (Vec2){room.x + room.w * 0.5, room.y + room.h * 0.5};
}

static double start_dist2(int x, int y)
{
    double dx = x + 0.5 - 2.5;
    double dy = y + 0.5 - 22.5;
    return dx * dx + dy * dy;
}

static int generated_floor(int x, int y)
{
    return x > 0 && x < MAP_W - 1 && y > 0 && y < MAP_H - 1 && level_map[y][x] == 0;
}

static int reserved_dynamic_tile(const GameState *game, int x, int y)
{
    for (int i = 0; i < MAX_DOORS; ++i) {
        if (game->doors[i].x == x && game->doors[i].y == y) {
            return 1;
        }
    }
    for (int i = 0; i < MAX_SECRETS; ++i) {
        if (game->secrets[i].x == x && game->secrets[i].y == y) {
            return 1;
        }
    }
    return 0;
}

static int occupied_spawn_tile(const GameState *game, int x, int y)
{
    if (reserved_dynamic_tile(game, x, y)) {
        return 1;
    }
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game->items[i].active && (int)game->items[i].pos.x == x && (int)game->items[i].pos.y == y) {
            return 1;
        }
    }
    for (int i = 0; i < game->monster_count; ++i) {
        if (game->monsters[i].active && (int)game->monsters[i].pos.x == x && (int)game->monsters[i].pos.y == y) {
            return 1;
        }
    }
    return 0;
}

static Vec2 pick_floor_spot(LevelRng *rng, const GameState *game, double min_dist2)
{
    Vec2 best = {2.5, 22.5};
    double best_dist = -1.0;
    for (int attempt = 0; attempt < 220; ++attempt) {
        int x = rng_range(rng, 1, MAP_W - 2);
        int y = rng_range(rng, 1, MAP_H - 2);
        double d = start_dist2(x, y);
        if (generated_floor(x, y) && !occupied_spawn_tile(game, x, y) && d >= min_dist2) {
            return (Vec2){x + 0.5, y + 0.5};
        }
        if (generated_floor(x, y) && !occupied_spawn_tile(game, x, y) && d > best_dist) {
            best_dist = d;
            best = (Vec2){x + 0.5, y + 0.5};
        }
    }
    return best;
}

static Vec2 pick_floor_spot_in_rect(LevelRng *rng, const GameState *game, int rx, int ry, int rw, int rh)
{
    for (int attempt = 0; attempt < 120; ++attempt) {
        int x = rng_range(rng, rx, rx + rw - 1);
        int y = rng_range(rng, ry, ry + rh - 1);
        if (generated_floor(x, y) && !occupied_spawn_tile(game, x, y)) {
            return (Vec2){x + 0.5, y + 0.5};
        }
    }
    return (Vec2){rx + rw * 0.5, ry + rh * 0.5};
}

static void place_generated_doors(GameState *game, LevelRng *rng)
{
    int count = 0;
    for (int pass = 0; pass < 2 && count < MAX_DOORS; ++pass) {
        for (int attempt = 0; attempt < 420 && count < MAX_DOORS; ++attempt) {
            int x = rng_range(rng, 2, MAP_W - 3);
            int y = rng_range(rng, 2, MAP_H - 3);
            if (!generated_floor(x, y) || reserved_dynamic_tile(game, x, y) || start_dist2(x, y) < 20.0) {
                continue;
            }

            int vertical_gap = level_map[y][x - 1] > 0 && level_map[y][x + 1] > 0 &&
                               generated_floor(x, y - 1) && generated_floor(x, y + 1);
            int horizontal_gap = level_map[y - 1][x] > 0 && level_map[y + 1][x] > 0 &&
                                 generated_floor(x - 1, y) && generated_floor(x + 1, y);
            if (!vertical_gap && !horizontal_gap) {
                continue;
            }

            int locked = count < 2;
            game->doors[count++] = (Door){x, y, locked, 0, 0, 0.0};
        }
    }
}

static void place_generated_secrets(GameState *game, LevelRng *rng)
{
    int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int count = 0;

    for (int attempt = 0; attempt < 700 && count < MAX_SECRETS; ++attempt) {
        int x = rng_range(rng, 2, MAP_W - 3);
        int y = rng_range(rng, 2, MAP_H - 3);
        if (level_map[y][x] == 0 || reserved_dynamic_tile(game, x, y) || start_dist2(x, y) < 18.0) {
            continue;
        }

        int dir_i = rng_range(rng, 0, 3);
        for (int r = 0; r < 4; ++r) {
            int *dir = dirs[(dir_i + r) & 3];
            int ox = x + dir[0];
            int oy = y + dir[1];
            int hx = x - dir[0];
            int hy = y - dir[1];
            if (!generated_floor(ox, oy) || hx <= 1 || hx >= MAP_W - 2 || hy <= 1 || hy >= MAP_H - 2) {
                continue;
            }
            if (level_map[hy][hx] == 0) {
                continue;
            }

            carve_tile(hx, hy);
            carve_tile(hx - dir[1], hy - dir[0]);
            carve_tile(hx + dir[1], hy + dir[0]);
            level_map[y][x] = level_wall_tex(x, y, rng->state);
            game->secrets[count++] = (Secret){x, y, 0, 0, 0.0};
            break;
        }
    }
}

static void place_generated_torches(const GameState *game)
{
    int count = 0;
    memset(torches, 0, sizeof(torches));

    for (int y = 1; y < MAP_H - 1 && count < MAX_TORCHES; ++y) {
        for (int x = 1; x < MAP_W - 1 && count < MAX_TORCHES; ++x) {
            if (!generated_floor(x, y) || reserved_dynamic_tile(game, x, y) || ((x * 11 + y * 7) % 5) != 0) {
                continue;
            }
            if (level_map[y][x - 1] > 0) {
                torches[count++].pos = (Vec2){x + 0.08, y + 0.5};
            } else if (level_map[y][x + 1] > 0) {
                torches[count++].pos = (Vec2){x + 0.92, y + 0.5};
            } else if (level_map[y - 1][x] > 0) {
                torches[count++].pos = (Vec2){x + 0.5, y + 0.08};
            } else if (level_map[y + 1][x] > 0) {
                torches[count++].pos = (Vec2){x + 0.5, y + 0.92};
            }
        }
    }

    for (int y = 1; y < MAP_H - 1 && count < MAX_TORCHES; ++y) {
        for (int x = 1; x < MAP_W - 1 && count < MAX_TORCHES; ++x) {
            if (generated_floor(x, y) && !reserved_dynamic_tile(game, x, y) && level_map[y][x - 1] > 0) {
                torches[count++].pos = (Vec2){x + 0.08, y + 0.5};
            }
        }
    }
}

static void place_generated_items(GameState *game, LevelRng *rng)
{
    static const int item_types[MAX_ITEMS] = {
        ITEM_KEY, ITEM_HEALTH, ITEM_RAPID, ITEM_FIREBALL, ITEM_AMMO,
        ITEM_HEALTH, ITEM_RAPID, ITEM_DAMAGE, ITEM_AMMO, ITEM_HEALTH,
        ITEM_AMMO, ITEM_HEALTH, ITEM_FIREBALL, ITEM_AMMO, ITEM_HEALTH,
        ITEM_DAMAGE, ITEM_FIREBALL, ITEM_RAPID, ITEM_AMMO, ITEM_HEALTH,
    };
    for (int i = 0; i < MAX_ITEMS; ++i) {
        double min_dist = i == 0 ? 2.0 : 24.0;
        Vec2 pos = i == 0 ? (Vec2){4.5, 22.5} : pick_floor_spot(rng, game, min_dist);
        game->items[i] = (Item){1, item_types[i], pos};
    }
}

static void place_generated_monsters(GameState *game, LevelRng *rng, int boss_room, LevelRoom room)
{
    game->monster_count = MAX_MONSTERS;
    static const int monster_types[MAX_MONSTERS] = {1, 1, 1, 0, 1, 1, 2, 1, 0, 1};
    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        monster->active = 1;
        monster->pos = boss_room && i == game->monster_count - 1
            ? pick_floor_spot_in_rect(rng, game, room.x, room.y, room.w, room.h)
            : pick_floor_spot(rng, game, i == game->monster_count - 1 ? 180.0 : 36.0);
        monster->shoot_timer = 0.45 + i * 0.27;
        monster->target_waypoint = 1;
        monster->route = i;
        monster->type = monster_types[i];
        monster->hp = monster_max_hp(monster->type);
        monster->facing = (Vec2){0.0, -1.0};
        monster->ai_state = 0;
        monster->last_seen = monster->pos;
        monster->patrol[0] = monster->pos;
        monster->patrol[1] = boss_room && i == game->monster_count - 1
            ? pick_floor_spot_in_rect(rng, game, room.x, room.y, room.w, room.h)
            : pick_floor_spot(rng, game, 30.0);
        monster->patrol_count = 2;
        monster->alert_timer = 0.0;
        monster->strafe_timer = 0.4 + i * 0.11;
        monster->strafe_dir = (i & 1) ? 1 : -1;
        monster->pain_timer = 0.0;
        monster->is_boss = i == game->monster_count - 1;
        if (monster->is_boss) {
            monster->type = 0;
            monster->hp = boss_room ? 28 : 18;
        }
    }
}

static void carve_maze(LevelRng *rng)
{
    int stack_x[144];
    int stack_y[144];
    int top = 0;

    carve_tile(3, 21);
    stack_x[top] = 3;
    stack_y[top] = 21;
    top++;

    while (top > 0) {
        int cx = stack_x[top - 1];
        int cy = stack_y[top - 1];
        int dirs[4] = {0, 1, 2, 3};
        for (int i = 0; i < 4; ++i) {
            int j = rng_range(rng, i, 3);
            int t = dirs[i];
            dirs[i] = dirs[j];
            dirs[j] = t;
        }

        int carved = 0;
        for (int i = 0; i < 4; ++i) {
            int dx = dirs[i] == 0 ? 2 : (dirs[i] == 1 ? -2 : 0);
            int dy = dirs[i] == 2 ? 2 : (dirs[i] == 3 ? -2 : 0);
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx < 1 || nx > MAP_W - 3 || ny < 1 || ny > MAP_H - 3 || generated_floor(nx, ny)) {
                continue;
            }
            carve_tile(cx + dx / 2, cy + dy / 2);
            carve_tile(nx, ny);
            stack_x[top] = nx;
            stack_y[top] = ny;
            top++;
            carved = 1;
            break;
        }
        if (!carved) {
            top--;
        }
    }
}

static void carve_tight_level(GameState *game, LevelRng *rng)
{
    (void)game;
    carve_room((LevelRoom){1, 20, 5, 3});
    carve_maze(rng);
    carve_room((LevelRoom){19, 1, 4, 3});
    carve_h_corridor(19, 21, 3);
}

static void generate_rooms_level(LevelRng *rng, uint32_t seed)
{
    LevelRoom rooms[MAX_LEVEL_ROOMS];
    int room_count = 0;

    level_fill_walls(seed);
    rooms[room_count++] = (LevelRoom){1, 20, 5, 3};

    for (int gy = 0; gy < 3 && room_count < MAX_LEVEL_ROOMS; ++gy) {
        for (int gx = 0; gx < 3 && room_count < MAX_LEVEL_ROOMS; ++gx) {
            int x = 1 + gx * 7 + rng_range(rng, 0, 2);
            int y = 1 + gy * 6 + rng_range(rng, 0, 2);
            int w = 4 + rng_range(rng, 0, 2);
            int h = 4 + rng_range(rng, 0, 2);
            if (x + w >= MAP_W - 1) w = MAP_W - 2 - x;
            if (y + h >= MAP_H - 1) h = MAP_H - 2 - y;
            rooms[room_count++] = (LevelRoom){x, y, w, h};
        }
    }

    for (int i = 0; i < room_count; ++i) {
        carve_room(rooms[i]);
    }
    for (int i = 1; i < room_count; ++i) {
        Vec2 a = room_center(rooms[i - 1]);
        Vec2 b = room_center(rooms[i]);
        if (rng_next(rng) & 1u) {
            carve_h_corridor((int)a.x, (int)b.x, (int)a.y);
            carve_v_corridor((int)b.x, (int)a.y, (int)b.y);
        } else {
            carve_v_corridor((int)a.x, (int)a.y, (int)b.y);
            carve_h_corridor((int)a.x, (int)b.x, (int)b.y);
        }
    }
    carve_h_corridor(2, (int)room_center(rooms[1]).x, 22);
    carve_v_corridor((int)room_center(rooms[1]).x, (int)room_center(rooms[1]).y, 22);
}

static void generate_level(GameState *game, uint32_t seed, int mode)
{
    LevelRng rng = {seed ? seed : LEVEL_TEST_SEED};
    LevelRoom boss_room = {15, 1, 8, 7};
    level_fill_walls(seed);

    if (mode == GENERATOR_TIGHT) {
        carve_tight_level(game, &rng);
    } else if (mode == GENERATOR_BOSS) {
        carve_tight_level(game, &rng);
        carve_room(boss_room);
        carve_v_corridor(18, 7, 10);
        game->doors[0] = (Door){18, 8, 1, 0, 0, 0.0};
    } else {
        generate_rooms_level(&rng, seed);
    }

    if (mode == GENERATOR_BOSS) {
        game->doors[0] = (Door){18, 8, 1, 0, 0, 0.0};
    } else {
        place_generated_doors(game, &rng);
    }
    place_generated_secrets(game, &rng);
    place_generated_torches(game);
    place_generated_items(game, &rng);
    place_generated_monsters(game, &rng, mode == GENERATOR_BOSS, boss_room);
}

static void init_game_seed(GameState *game, uint32_t seed, int mode)
{
    memset(game, 0, sizeof(*game));
    active_game = game;
    game->generator_mode = mode;
    game->player_health = PLAYER_MAX_HEALTH;
    game->ammo = START_AMMO;
    game->fireball_ammo = START_FIREBALL_AMMO;
    game->selected_weapon = WEAPON_PISTOL;
    game->fireball_unlocked = 0;
    generate_level(game, seed, mode);
}

static void init_game(GameState *game)
{
    init_game_seed(game, LEVEL_TEST_SEED, GENERATOR_ROOMS);
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
    if (!monster->active) {
        return;
    }
    const Vec2 *waypoints = monster->patrol;
    int waypoint_count = monster->patrol_count;
    if (waypoint_count <= 0) {
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
        monster->strafe_timer = (monster->type == 2 ? 0.38 : (monster->type == 0 ? 1.05 : 0.70)) + 0.09 * (monster->route % 4);
        monster->strafe_dir *= -1;
    }

    if (monster->ai_state == 2 && sees_player) {
        Vec2 dir_to_player = vec_norm(to_player);
        double preferred = monster_preferred_distance(monster->type);
        if (player_dist > preferred + 0.7) {
            move_monster_toward(monster, cam->pos, monster_attack_speed(monster->type), dt);
        } else if (player_dist < preferred - 0.8) {
            Vec2 away = {
                monster->pos.x - dir_to_player.x,
                monster->pos.y - dir_to_player.y,
            };
            move_monster_toward(monster, away, monster_retreat_speed(monster->type), dt);
        } else {
            double strafe_speed = monster->type == 2 ? 1.55 : (monster->type == 0 ? 0.72 : 1.05);
            Vec2 side = {
                -dir_to_player.y * monster->strafe_dir * strafe_speed * dt,
                dir_to_player.x * monster->strafe_dir * strafe_speed * dt,
            };
            move_monster_by(monster, side);
        }
        return;
    }

    if (monster->ai_state == 1) {
        if (move_monster_toward(monster, monster->last_seen, monster_chase_speed(monster->type), dt)) {
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
    if (!move_monster_toward(monster, target, monster_patrol_speed(monster->type), dt)) {
        monster->target_waypoint = (monster->target_waypoint + 1) % waypoint_count;
    }
}

static void spawn_explosion(GameState *game, Vec2 pos, double radius)
{
    play_sound(92.0, 0.24, 0.34);
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_NONE;
            p->type = PROJECTILE_EXPLOSION;
            p->pos = pos;
            p->life = 0.22;
            p->radius = radius;
            break;
        }
    }

    for (int i = 0; i < MAX_DECALS; ++i) {
        Decal *d = &game->decals[i];
        if (!d->active) {
            d->active = 1;
            d->type = 0;
            d->pos = pos;
            d->radius = radius * 0.85;
            d->life = 30.0;
            break;
        }
    }

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < MAX_PARTICLES; ++j) {
            Particle *p = &game->particles[j];
            if (!p->active) {
                double a = i * (M_PI * 2.0 / 10.0);
                double speed = 0.28 + (i % 3) * 0.10;
                p->active = 1;
                p->pos = pos;
                p->vel = (Vec2){cos(a) * speed, sin(a) * speed};
                p->life = 0.85 + (i % 4) * 0.08;
                p->max_life = p->life;
                p->size = 0.32 + (i % 3) * 0.08;
                p->color = i & 1 ? rgb(96, 74, 58) : rgb(146, 70, 34);
                break;
            }
        }
    }
}

static void damage_monster(GameState *game, Monster *monster, int damage, Vec2 source)
{
    if (!monster->active) {
        return;
    }

    monster->hp -= damage;
    monster->pain_timer = 0.18;
    monster->alert_timer = 5.0;
    Vec2 push = vec_norm((Vec2){
        monster->pos.x - source.x,
        monster->pos.y - source.y,
    });
    if (push.x != 0.0 || push.y != 0.0) {
        move_monster_by(monster, (Vec2){push.x * 0.08, push.y * 0.08});
    }

    if (monster->hp <= 0) {
        monster->active = 0;
        game->kills += 1;
        spawn_explosion(game, monster->pos, 0.80);
    }
}

static void explode_player_fireball(GameState *game, Vec2 pos, int damage, double radius)
{
    double radius2 = radius * radius;

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }

        double dx = monster->pos.x - pos.x;
        double dy = monster->pos.y - pos.y;
        double dist2 = dx * dx + dy * dy;
        if (dist2 > radius2) {
            continue;
        }

        double dist = sqrt(dist2);
        int splash = (int)ceil(damage * (1.0 - dist / radius));
        if (splash < 1) {
            splash = 1;
        }
        damage_monster(game, monster, splash, pos);
    }

    spawn_explosion(game, pos, radius);
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
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_ENEMY;
            p->type = PROJECTILE_ENEMY_BOLT;
            p->damage = monster_projectile_damage(monster->type);
            p->radius = 0.0;
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

static int spawn_player_fireball(GameState *game, const Camera *cam)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_PLAYER;
            p->type = PROJECTILE_PLAYER_FIREBALL;
            p->damage = FIREBALL_SPLASH_DAMAGE + (game->damage_timer > 0.0 ? 2 : 0);
            p->radius = FIREBALL_RADIUS;
            p->pos = (Vec2){
                cam->pos.x + cam->dir.x * 0.48,
                cam->pos.y + cam->dir.y * 0.48,
            };
            p->vel = (Vec2){cam->dir.x * 5.4, cam->dir.y * 5.4};
            p->life = 2.4;
            return 1;
        }
    }
    return 0;
}

static void update_projectiles(GameState *game, const Camera *cam, double dt)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            continue;
        }

        p->life -= dt;
        if (p->type == PROJECTILE_EXPLOSION) {
            if (p->life <= 0.0) {
                p->active = 0;
            }
            continue;
        }

        Vec2 old_pos = p->pos;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;

        if (p->life <= 0.0 || map_at((int)p->pos.x, (int)p->pos.y) != 0) {
            if (p->type == PROJECTILE_PLAYER_FIREBALL) {
                explode_player_fireball(game, old_pos, p->damage, p->radius);
            }
            p->active = 0;
            continue;
        }

        if (p->owner == PROJECTILE_OWNER_ENEMY) {
            double dx = p->pos.x - cam->pos.x;
            double dy = p->pos.y - cam->pos.y;
            if (dx * dx + dy * dy < 0.18 && game->hit_flash <= 0.0) {
                p->active = 0;
                game->player_health -= p->damage;
                if (game->player_health <= 0) {
                    game->player_health = 0;
                    game->game_over = 1;
                }
                game->hit_flash = 0.18;
            }
        } else if (p->owner == PROJECTILE_OWNER_PLAYER) {
            for (int m = 0; m < game->monster_count; ++m) {
                Monster *monster = &game->monsters[m];
                if (!monster->active) {
                    continue;
                }
                double dx = p->pos.x - monster->pos.x;
                double dy = p->pos.y - monster->pos.y;
                if (dx * dx + dy * dy < 0.36) {
                    int direct = FIREBALL_DIRECT_DAMAGE + (game->damage_timer > 0.0 ? 2 : 0);
                    damage_monster(game, monster, direct, p->pos);
                    explode_player_fireball(game, p->pos, p->damage, p->radius);
                    p->active = 0;
                    break;
                }
            }
        }
    }
}

static void pickup_item(GameState *game, Item *item)
{
    switch (item->type) {
    case ITEM_HEALTH:
        game->player_health += 45;
        if (game->player_health > PLAYER_MAX_HEALTH) {
            game->player_health = PLAYER_MAX_HEALTH;
        }
        break;
    case ITEM_AMMO:
        game->ammo += 18;
        if (game->ammo > 99) {
            game->ammo = 99;
        }
        break;
    case ITEM_RAPID:
        game->rapid_timer = 12.0;
        break;
    case ITEM_DAMAGE:
        game->damage_timer = 12.0;
        break;
    case ITEM_FIREBALL:
        game->fireball_unlocked = 1;
        game->selected_weapon = WEAPON_FIREBALL;
        game->fireball_ammo += 6;
        if (game->fireball_ammo > MAX_FIREBALL_AMMO) {
            game->fireball_ammo = MAX_FIREBALL_AMMO;
        }
        break;
    default:
        game->keys += 1;
        break;
    }

    game->pickup_flash = 0.22;
    play_sound(item->type == ITEM_KEY ? 720.0 : 640.0, 0.09, 0.18);
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

static void select_weapon(GameState *game, int weapon)
{
    if (weapon == WEAPON_FIREBALL && !game->fireball_unlocked) {
        return;
    }
    game->selected_weapon = weapon;
}

static void interact_world(GameState *game, const Camera *cam)
{
    int tx = (int)(cam->pos.x + cam->dir.x * 0.95);
    int ty = (int)(cam->pos.y + cam->dir.y * 0.95);

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->x == tx && door->y == ty && !door->open) {
            if (!door->locked || game->keys > 0) {
                door->opening = 1;
                door->locked = 0;
                play_sound(160.0, 0.18, 0.20);
            }
            return;
        }
    }

    for (int i = 0; i < MAX_SECRETS; ++i) {
        Secret *secret = &game->secrets[i];
        if (secret->x == tx && secret->y == ty && !secret->open) {
            secret->opening = 1;
            play_sound(118.0, 0.24, 0.22);
            return;
        }
    }
}

static void player_fire(GameState *game, const Camera *cam)
{
    if (game->game_over || game->victory || game->shot_cooldown > 0.0) {
        return;
    }

    if (game->selected_weapon == WEAPON_FIREBALL) {
        if (!game->fireball_unlocked || game->fireball_ammo <= 0) {
            return;
        }
        if (!spawn_player_fireball(game, cam)) {
            return;
        }
        play_sound(260.0, 0.20, 0.28);
        game->fireball_ammo -= 1;
        game->shot_cooldown = game->rapid_timer > 0.0 ? FIREBALL_COOLDOWN_TIME * 0.70 : FIREBALL_COOLDOWN_TIME;
        game->weapon_flash = FIREBALL_FLASH_TIME;
        game->shot_trace = 0.0;
        return;
    }

    if (game->ammo <= 0) {
        return;
    }

    game->ammo -= 1;
    play_sound(520.0, 0.055, 0.18);
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
        damage_monster(game, monster, damage, cam->pos);
    }
}

static void update_game(GameState *game, const Camera *cam, double dt)
{
    game->time += dt;
    reveal_fog(game, cam);

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->opening && !door->open) {
            door->open_amount += dt * 2.4;
            if (door->open_amount >= 1.0) {
                door->open_amount = 1.0;
                door->open = 1;
            }
        }
    }
    for (int i = 0; i < MAX_SECRETS; ++i) {
        Secret *secret = &game->secrets[i];
        if (secret->opening && !secret->open) {
            secret->open_amount += dt * 1.6;
            if (secret->open_amount >= 1.0) {
                secret->open_amount = 1.0;
                secret->open = 1;
                level_map[secret->y][secret->x] = 0;
            }
        }
    }

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        update_monster(monster, cam, dt);
        if (monster->facing_lock > 0.0) {
            monster->facing_lock -= dt;
            if (monster->facing_lock < 0.0) {
                monster->facing_lock = 0.0;
            }
        }
        if (monster->pain_timer > 0.0) {
            monster->pain_timer -= dt;
            if (monster->pain_timer < 0.0) {
                monster->pain_timer = 0.0;
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
            monster->shoot_timer += monster_shot_cooldown(monster->type, i);
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

    for (int i = 0; i < MAX_DECALS; ++i) {
        Decal *decal = &game->decals[i];
        if (decal->active) {
            decal->life -= dt;
            if (decal->life <= 0.0) {
                decal->active = 0;
            }
        }
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        Particle *particle = &game->particles[i];
        if (!particle->active) {
            continue;
        }
        particle->life -= dt;
        if (particle->life <= 0.0) {
            particle->active = 0;
            continue;
        }
        particle->pos.x += particle->vel.x * dt;
        particle->pos.y += particle->vel.y * dt;
        particle->vel.x *= 1.0 - dt * 0.45;
        particle->vel.y *= 1.0 - dt * 0.45;
        if (!can_occupy(particle->pos.x, particle->pos.y, 0.05)) {
            particle->active = 0;
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
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };

    player_fire(&game, &cam);
    if (game.ammo != START_AMMO - 1 || game.monsters[0].hp != 2 || !game.monsters[0].active) {
        fprintf(stderr, "error: player weapon failed first hit verification\n");
        return 0;
    }

    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != START_AMMO - 2 || game.monsters[0].active || game.kills != 1) {
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
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    Monster *monster = &game.monsters[0];

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
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    Monster *monster = &game.monsters[0];

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

    int ammo_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_AMMO) {
            ammo_item = i;
            break;
        }
    }
    game.ammo = 10;
    if (ammo_item < 0) {
        fprintf(stderr, "error: no ammo pickup available for verification\n");
        return 0;
    }
    cam.pos = game.items[ammo_item].pos;
    update_items(&game, &cam);
    if (game.items[ammo_item].active || game.ammo <= 10 || game.pickup_flash <= 0.0) {
        fprintf(stderr, "error: item pickup verification failed\n");
        return 0;
    }

    int found_fireball = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_FIREBALL) {
            cam.pos = game.items[i].pos;
            update_items(&game, &cam);
            found_fireball = 1;
            break;
        }
    }
    if (!found_fireball || !game.fireball_unlocked || game.selected_weapon != WEAPON_FIREBALL || game.fireball_ammo <= 0) {
        fprintf(stderr, "error: fireball pickup verification failed\n");
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

static int verify_fireball_weapon(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    select_weapon(&game, WEAPON_FIREBALL);
    if (game.selected_weapon != WEAPON_PISTOL) {
        fprintf(stderr, "error: locked fireball weapon was selected\n");
        return 0;
    }

    game.fireball_unlocked = 1;
    game.fireball_ammo = 2;
    select_weapon(&game, WEAPON_FIREBALL);
    if (game.selected_weapon != WEAPON_FIREBALL) {
        fprintf(stderr, "error: unlocked fireball weapon was not selected\n");
        return 0;
    }

    game.monster_count = 2;
    game.monsters[0].active = 1;
    game.monsters[0].hp = 10;
    game.monsters[0].pos = (Vec2){6.0, 22.5};
    game.monsters[1].active = 1;
    game.monsters[1].hp = 10;
    game.monsters[1].pos = (Vec2){6.2, 22.8};
    memset(game.projectiles, 0, sizeof(game.projectiles));

    player_fire(&game, &cam);
    if (game.fireball_ammo != 1 || game.shot_cooldown <= 0.0) {
        fprintf(stderr, "error: fireball did not consume ammo and set cooldown\n");
        return 0;
    }

    for (int i = 0; i < 90; ++i) {
        update_projectiles(&game, &cam, 1.0 / 60.0);
    }

    if (game.monsters[0].hp >= 10 || game.monsters[1].hp >= 10) {
        fprintf(stderr, "error: fireball did not apply direct and splash damage\n");
        return 0;
    }

    return 1;
}

static int verify_projectile_ownership(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    game.monster_count = 1;
    game.monsters[0].active = 1;
    game.monsters[0].hp = 10;
    game.monsters[0].pos = (Vec2){3.0, 22.5};
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.projectiles[0] = (Projectile){
        .active = 1,
        .owner = PROJECTILE_OWNER_ENEMY,
        .type = PROJECTILE_ENEMY_BOLT,
        .damage = 99,
        .pos = game.monsters[0].pos,
        .vel = {0.0, 0.0},
        .life = 1.0,
    };
    update_projectiles(&game, &cam, 1.0 / 60.0);
    if (game.monsters[0].hp != 10) {
        fprintf(stderr, "error: enemy projectile damaged a monster\n");
        return 0;
    }

    int health = game.player_health;
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.monster_count = 0;
    game.projectiles[0] = (Projectile){
        .active = 1,
        .owner = PROJECTILE_OWNER_PLAYER,
        .type = PROJECTILE_PLAYER_FIREBALL,
        .damage = 99,
        .radius = FIREBALL_RADIUS,
        .pos = cam.pos,
        .vel = {0.0, 0.0},
        .life = 1.0,
    };
    update_projectiles(&game, &cam, 1.0 / 60.0);
    if (game.player_health != health) {
        fprintf(stderr, "error: player fireball damaged the player\n");
        return 0;
    }

    return 1;
}

static int verify_monster_roles(void)
{
    GameState game;
    init_game(&game);

    for (int i = 0; i < game.monster_count; ++i) {
        Monster *monster = &game.monsters[i];
        if (monster->is_boss) {
            if (monster->type != 0 || monster->hp != 18) {
                fprintf(stderr, "error: boss monster role is not deterministic\n");
                return 0;
            }
            continue;
        }
        if (monster->hp != monster_max_hp(monster->type)) {
            fprintf(stderr, "error: monster %d hp does not match its role\n", i);
            return 0;
        }
    }

    if (!(monster_chase_speed(2) > monster_chase_speed(1) &&
          monster_chase_speed(1) > monster_chase_speed(0))) {
        fprintf(stderr, "error: monster role speeds are not ordered as expected\n");
        return 0;
    }
    if (!(monster_projectile_damage(0) > monster_projectile_damage(1) &&
          monster_projectile_damage(1) > monster_projectile_damage(2))) {
        fprintf(stderr, "error: monster projectile damage roles are not ordered as expected\n");
        return 0;
    }

    return 1;
}

static int setup_interaction_camera(Camera *cam, int tx, int ty)
{
    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; ++i) {
        int px = tx + dirs[i][0];
        int py = ty + dirs[i][1];
        if (!generated_floor(px, py)) {
            continue;
        }
        cam->pos = (Vec2){px + 0.5, py + 0.5};
        cam->dir = (Vec2){-dirs[i][0], -dirs[i][1]};
        cam->plane = (Vec2){-cam->dir.y * 0.66, cam->dir.x * 0.66};
        return 1;
    }
    return 0;
}

static int verify_doors_and_secrets(void)
{
    GameState game;
    init_game(&game);
    Camera cam;
    int door_index = -1;

    for (int i = 0; i < MAX_DOORS; ++i) {
        if (game.doors[i].locked) {
            door_index = i;
            break;
        }
    }
    if (door_index < 0 || !setup_interaction_camera(&cam, game.doors[door_index].x, game.doors[door_index].y)) {
        fprintf(stderr, "error: generated level has no usable locked door\n");
        return 0;
    }

    if (map_at(game.doors[door_index].x, game.doors[door_index].y) == 0) {
        fprintf(stderr, "error: locked door does not block before interaction\n");
        return 0;
    }

    interact_world(&game, &cam);
    if (game.doors[door_index].opening || game.doors[door_index].open) {
        fprintf(stderr, "error: locked door opened without a key\n");
        return 0;
    }

    game.keys = 1;
    interact_world(&game, &cam);
    for (int i = 0; i < 32; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.doors[door_index].open || map_at(game.doors[door_index].x, game.doors[door_index].y) != 0) {
        fprintf(stderr, "error: keyed door did not open cleanly\n");
        return 0;
    }

    int secret_index = -1;
    for (int i = 0; i < MAX_SECRETS; ++i) {
        if (setup_interaction_camera(&cam, game.secrets[i].x, game.secrets[i].y)) {
            secret_index = i;
            break;
        }
    }
    if (secret_index < 0) {
        fprintf(stderr, "error: generated level has no usable secret\n");
        return 0;
    }
    if (map_at(game.secrets[secret_index].x, game.secrets[secret_index].y) == 0) {
        fprintf(stderr, "error: secret does not block before interaction\n");
        return 0;
    }
    interact_world(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.secrets[secret_index].open || map_at(game.secrets[secret_index].x, game.secrets[secret_index].y) != 0) {
        fprintf(stderr, "error: secret wall did not open cleanly\n");
        return 0;
    }

    return 1;
}

static int verify_generator_modes(void)
{
    for (int mode = 0; mode < GENERATOR_COUNT; ++mode) {
        GameState game;
        init_game_seed(&game, LEVEL_TEST_SEED + (uint32_t)mode * 97u, mode);

        if (game.generator_mode != mode || !can_move(2.5, 22.5) || !can_occupy(4.5, 22.5, 0.12)) {
            fprintf(stderr, "error: generator mode %d did not create a valid entrance\n", mode);
            return 0;
        }

        for (int i = 0; i < MAX_ITEMS; ++i) {
            if (game.items[i].active && !can_occupy(game.items[i].pos.x, game.items[i].pos.y, 0.12)) {
                fprintf(stderr, "error: generator mode %d placed item %d inside a wall\n", mode, i);
                return 0;
            }
        }
        for (int i = 0; i < game.monster_count; ++i) {
            if (game.monsters[i].active && !can_occupy(game.monsters[i].pos.x, game.monsters[i].pos.y, 0.28)) {
                fprintf(stderr, "error: generator mode %d placed monster %d inside a wall\n", mode, i);
                return 0;
            }
        }

        if (mode == GENERATOR_TIGHT && !generated_floor(21, 2)) {
            fprintf(stderr, "error: tight generator did not create a far exit room\n");
            return 0;
        }
        if (mode == GENERATOR_BOSS) {
            Monster *boss = &game.monsters[game.monster_count - 1];
            if (!boss->is_boss || boss->hp < 28 ||
                boss->pos.x < 15.0 || boss->pos.x > 23.0 ||
                boss->pos.y < 1.0 || boss->pos.y > 8.0 ||
                !game.doors[0].locked || game.doors[0].x != 18 || game.doors[0].y != 8) {
                fprintf(stderr, "error: boss generator did not create boss chamber setup\n");
                return 0;
            }
        }
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

static int verify_torches(void)
{
    GameState game;
    init_game(&game);

    for (int i = 0; i < MAX_TORCHES; ++i) {
        Vec2 pos = torches[i].pos;
        int cx = (int)pos.x;
        int cy = (int)pos.y;
        int near_wall =
            map_at(cx - 1, cy) > 0 ||
            map_at(cx + 1, cy) > 0 ||
            map_at(cx, cy - 1) > 0 ||
            map_at(cx, cy + 1) > 0;

        if (!can_occupy(pos.x, pos.y, 0.04)) {
            fprintf(stderr, "error: torch %d is placed inside a wall at %.2f %.2f\n", i, pos.x, pos.y);
            return 0;
        }
        if (!near_wall) {
            fprintf(stderr, "error: torch %d is not mounted next to a wall at %.2f %.2f\n", i, pos.x, pos.y);
            return 0;
        }
    }

    return 1;
}

static int verify_volumetric_fog(void)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.time = 1.0;
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    double near_fog = fog_amount(1.0);
    double far_fog = fog_amount(12.0);
    if (far_fog <= near_fog) {
        fprintf(stderr, "error: distance fog does not increase with depth\n");
        return 0;
    }

    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        framebuffer[i] = rgb(90, 72, 56);
    }
    for (int x = 0; x < SCREEN_W; ++x) {
        z_buffer[x] = 12.0;
    }

    uint32_t before = framebuffer[(SCREEN_H / 2) * SCREEN_W + (SCREEN_W / 2)];
    render_volumetric_fog(&cam, &game);
    uint32_t after = framebuffer[(SCREEN_H / 2) * SCREEN_W + (SCREEN_W / 2)];

    if (before == after) {
        fprintf(stderr, "error: volumetric fog pass did not modify the framebuffer\n");
        return 0;
    }
    return 1;
}

static void move_camera(Camera *cam, const GameState *game, double forward, double strafe, double dt)
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

    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }
        Vec2 diff = {cam->pos.x - monster->pos.x, cam->pos.y - monster->pos.y};
        double dist = vec_len(diff);
        double min_dist = monster->is_boss ? 0.72 : 0.48;
        if (dist > 0.001 && dist < min_dist) {
            Vec2 push = vec_norm(diff);
            double amount = (min_dist - dist) * 0.45;
            double px = cam->pos.x + push.x * amount;
            double py = cam->pos.y + push.y * amount;
            if (can_move(px, cam->pos.y)) {
                cam->pos.x = px;
            }
            if (can_move(cam->pos.x, py)) {
                cam->pos.y = py;
            }
        }
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

static void reset_run(GameState *game, Camera *cam)
{
    *cam = (Camera){
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    init_game_seed(game, runtime_level_seed++, runtime_level_mode);
    reveal_fog(game, cam);
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
    if (!verify_fireball_weapon()) {
        return 1;
    }
    if (!verify_projectile_ownership()) {
        return 1;
    }
    if (!verify_monster_roles()) {
        return 1;
    }
    if (!verify_doors_and_secrets()) {
        return 1;
    }
    if (!verify_generator_modes()) {
        return 1;
    }
    if (!verify_sprite_sort_order()) {
        return 1;
    }
    if (!verify_torches()) {
        return 1;
    }
    if (!verify_volumetric_fog()) {
        return 1;
    }
    GameState game;
    init_game(&game);
    reveal_fog(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    game.fireball_unlocked = 1;
    game.fireball_ammo = 6;
    select_weapon(&game, WEAPON_FIREBALL);
    player_fire(&game, &cam);
    for (int i = 0; i < 18; ++i) {
        update_projectiles(&game, &cam, 1.0 / 60.0);
    }
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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!init_audio()) {
        SDL_Quit();
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
        SDL_CloseAudioDevice(audio_device);
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
        SDL_CloseAudioDevice(audio_device);
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
        SDL_CloseAudioDevice(audio_device);
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    Camera cam;
    GameState game;
    runtime_level_seed = LEVEL_TEST_SEED ^ (uint32_t)SDL_GetTicks() ^ (uint32_t)SDL_GetPerformanceCounter();
    reset_run(&game, &cam);

    int running = 1;
    int paused = 0;
    int fullscreen = 0;
    uint64_t prev = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_1) {
                select_weapon(&game, WEAPON_PISTOL);
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_2) {
                select_weapon(&game, WEAPON_FIREBALL);
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_3 && event.key.repeat == 0) {
                runtime_level_mode = GENERATOR_ROOMS;
                reset_run(&game, &cam);
                paused = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_4 && event.key.repeat == 0) {
                runtime_level_mode = GENERATOR_TIGHT;
                reset_run(&game, &cam);
                paused = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_5 && event.key.repeat == 0) {
                runtime_level_mode = GENERATOR_BOSS;
                reset_run(&game, &cam);
                paused = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                if (!paused) {
                    player_fire(&game, &cam);
                }
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_f) {
                if (!paused) {
                    interact_world(&game, &cam);
                }
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB && event.key.repeat == 0) {
                game.show_automap = !game.show_automap;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p && event.key.repeat == 0) {
                paused = !paused;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r && event.key.repeat == 0) {
                reset_run(&game, &cam);
                paused = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11 && event.key.repeat == 0) {
                fullscreen = !fullscreen;
                if (SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
                    fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
                    running = 0;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (!paused) {
                    player_fire(&game, &cam);
                }
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

        if (!paused && !game.game_over && !game.victory && (forward != 0.0 || strafe != 0.0)) {
            move_camera(&cam, &game, forward, strafe, dt);
        }
        if (!paused && !game.game_over && !game.victory && turn != 0.0) {
            rotate_camera(&cam, turn * 2.2 * dt);
        }

        if (!paused && !game.game_over && !game.victory) {
            update_items(&game, &cam);
            update_game(&game, &cam, dt);
        }
        render_scene(&cam, &game);
        if (paused) {
            render_pause_overlay();
        }
        SDL_UpdateTexture(screen, NULL, framebuffer, SCREEN_W * (int)sizeof(uint32_t));

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(screen);
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
