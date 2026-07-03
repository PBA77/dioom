#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "prompt_font.h"

#ifndef SCREEN_W
#define SCREEN_W 640
#endif
#ifndef SCREEN_H
#define SCREEN_H 480
#endif
#ifndef WINDOW_SCALE
#define WINDOW_SCALE 2
#endif
#define TEX_SIZE 64
#define TEX_ATLAS_COLS 4
#define TEX_COUNT 10
#define TEX_ATLAS_ROWS ((TEX_COUNT + TEX_ATLAS_COLS - 1) / TEX_ATLAS_COLS)
#define WALL_DOOR 8
#define WALL_LOCKED_DOOR 9
#define TEX_DOOR 8
#define TEX_LOCKED_DOOR 9
#define SPRITE_SIZE 64
#define GIANT_SKELETON_SPRITE_SIZE 128
#define BOSS_SPRITE_SIZE 128
#define MONSTER_ANIM_FRAMES 4
#define BOSS_ANIM_FRAMES 4
#define SPRITE_FRAMES 4
#define MONSTER_TYPES 6
#define TREE_TYPES 4
#define HOUSE_TEX_COUNT 4
#define HOUSE_ATLAS_COLS 2
#define HOUSE_ATLAS_ROWS ((HOUSE_TEX_COUNT + HOUSE_ATLAS_COLS - 1) / HOUSE_ATLAS_COLS)
#define FURNITURE_SIZE 64
#define FURNITURE_SPRITE_COUNT 8
#define DECAL_SIZE 64
#define DECAL_COUNT 16
#define WALL_DECAL_SIZE 64
#define WALL_DECAL_COUNT 16
#define PROJECTILE_SIZE 32
#define ITEM_SPRITE_COUNT 16
#define WEAPON_SPRITE_SIZE 64
#define WEAPON_SPRITE_COUNT 8
#define MAX_PROJECTILES 20
#define MAX_MONSTERS 10
#define MAX_ITEMS 28
#define MAX_TORCHES 26
#define MAX_DECALS 96
#define MAX_WALL_DECALS 64
#define MAX_PARTICLES 64
#define MAX_TREES 96
#define MAX_HOUSES 3
#define MAX_PROPS 12
#define TREE_COLLISION_RADIUS 0.46
#define TREE_RENDER_NEAR_CLIP 0.30
#define HOUSE_COLLISION_PADDING 0.18
#define HOUSE_RENDER_NEAR_CLIP 0.08
#define HOUSE_WALL_HEIGHT 1.08
#define HOUSE_ROOF_RISE 0.42
#define HOUSE_ROOF_OVERHANG 0.16
#define HOUSE_HEIGHT (HOUSE_WALL_HEIGHT + HOUSE_ROOF_RISE)
#define DOOR_OPEN_SPEED 1.45
#define MAX_DOORS 4
#define MAX_SECRETS 3
#define MAX_PORTALS 5
#define MAX_LEVEL_ROOMS 10
#define RELIC_COUNT 4
#define RELIC_MASK_ALL ((1 << RELIC_COUNT) - 1)
#define FM_MUSIC_VOLUME 0.18
#define AUDIO_VOLUME_STEPS 8
#define DEFAULT_SFX_VOLUME_STEP 8
#define DEFAULT_MUSIC_VOLUME_STEP 6
#define FM_NOTE_ATTACK 0.010
#define FM_NOTE_DECAY 7.2
#define FM_NOTE_MAX_HOLD 0.18
#define FM_NOTE_RELEASE_DECAY 20.0
#define BOSS_HP 90
#define LEVEL_TEST_SEED 0x00C0FFEEu
#define PLAYER_MAX_HEALTH 160
#define START_AMMO 48
#define MAX_PISTOL_AMMO 99
#define START_FIREBALL_AMMO 0
#define MAX_FIREBALL_AMMO 24
#define SHOP_AMMO_PRICE 20
#define SHOP_HEALTH_PRICE 35
#define SHOP_MAX_HP_PRICE 85
#define SHOP_DAMAGE_PRICE 110
#define SHOP_AMMO_CAP_PRICE 75
#define SHOP_SHOTGUN_PRICE 95
#define SHOP_AMMO_AMOUNT 18
#define SHOP_HEALTH_AMOUNT 45
#define MAX_HEALTH_UPGRADES 3
#define HEALTH_UPGRADE_AMOUNT 20
#define MAX_DAMAGE_UPGRADES 2
#define MAX_AMMO_CAP_UPGRADES 3
#define AMMO_CAP_UPGRADE_AMOUNT 18
#define KNIFE_DAMAGE 3
#define KNIFE_RANGE 1.35
#define KNIFE_COOLDOWN_TIME 0.36
#define PLAYER_DAMAGE 2
#define SHOT_COOLDOWN_TIME 0.24
#define SHOTGUN_DAMAGE 5
#define SHOTGUN_RANGE 5.8
#define SHOTGUN_COOLDOWN_TIME 0.62
#define SHOTGUN_AMMO_COST 3
#define FIREBALL_COOLDOWN_TIME 0.75
#define WEAPON_FLASH_TIME 0.18
#define FIREBALL_FLASH_TIME 0.26
#define MUZZLE_LIGHT_TIME 0.075
#define HIT_MARKER_TIME 0.16
#define HIT_RIM_TIME 0.14
#define PLAYER_DAMAGE_FLASH_TIME 0.28
#define SCREEN_SHAKE_TIME 0.22
#define MONSTER_WINDUP_TIME 0.36
#define BOSS_WINDUP_TIME 0.44
#define FIREBALL_DIRECT_DAMAGE 5
#define FIREBALL_SPLASH_DAMAGE 4
#define FIREBALL_RADIUS 1.25
#define FOG_DENSITY 0.165
#define FOG_PASS_STRENGTH 1.10
#define FOG_LUT_SIZE 4096
#define FOG_LUT_MAX_DISTANCE 80.0
#define MAP_W 24
#define MAP_H 24
#define TEXTURE_ATLAS_PATH "assets/textures.ppm"
#define MONSTER_ATLAS_PATH "assets/monsters.ppm"
#define GIANT_SKELETON_ATLAS_PATH "assets/giant_skeleton.ppm"
#define BOSS_ATLAS_PATH "assets/boss.ppm"
#define TREE_ATLAS_PATH "assets/trees.ppm"
#define HOUSE_ATLAS_PATH "assets/houses.ppm"
#define FURNITURE_ATLAS_PATH "assets/furniture.ppm"
#define RELIC_ATLAS_PATH "assets/relics.ppm"
#define ITEM_ATLAS_PATH "assets/items.ppm"
#define WEAPON_ATLAS_PATH "assets/weapons.ppm"
#define DECAL_ATLAS_PATH "assets/decals.ppm"
#define WALL_DECAL_ATLAS_PATH "assets/wall_decals.ppm"
#define MUSIC_DIES_IRAE_PATH "assets/music/dies_irae.mid"
#define MUSIC_TOCCATA_PATH "assets/music/toccata_fugue.mid"
#define MUSIC_MASONIC_FUNERAL_PATH "assets/music/masonic_funeral.mid"
#define MUSIC_PATHETIQUE_PATH "assets/music/pathetique_1.mid"
#define SETTINGS_PATH "dioom.ini"
#define SAVEGAME_SLOT_COUNT 8
#define SAVEGAME_PATH_FORMAT "dioom_slot%d.sav"
#define SAVEGAME_TMP_PATH_FORMAT "dioom_slot%d.sav.tmp"
#define SAVEGAME_MAGIC 0x4D4F4944u
#define SAVEGAME_VERSION 7u
#define MAX_SAMPLE_VOICES 16
#define MAX_MIDI_VOICES 24
#define MONSTER_FLYING_HEAD 4
#define MONSTER_GIANT_SKELETON 5
#define MONSTER_BOSS_BUTCHER 6

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    double x;
    double y;
} Vec2;

typedef struct {
    double roughness;
    double metallic;
    double wetness;
    double normal_strength;
} Material;

enum {
    WEAPON_KNIFE = 0,
    WEAPON_PISTOL = 1,
    WEAPON_FIREBALL = 2,
    WEAPON_SHOTGUN = 3,
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
    PARTICLE_SMOKE = 0,
};

enum {
    ITEM_HEALTH = 0,
    ITEM_AMMO = 1,
    ITEM_RAPID = 2,
    ITEM_DAMAGE = 3,
    ITEM_FIREBALL = 4,
    ITEM_PISTOL = 5,
    ITEM_KEY = 6,
    ITEM_RELIC = 7,
    ITEM_GOLD = 8,
    ITEM_SHRINE = 9,
    ITEM_BONEPILE = 10,
};

enum {
    ITEM_SPRITE_HEALTH = 0,
    ITEM_SPRITE_AMMO,
    ITEM_SPRITE_RAPID,
    ITEM_SPRITE_DAMAGE,
    ITEM_SPRITE_FIREBALL,
    ITEM_SPRITE_PISTOL,
    ITEM_SPRITE_KEY,
    ITEM_SPRITE_GOLD,
    ITEM_SPRITE_SHRINE,
    ITEM_SPRITE_BONEPILE,
    ITEM_SPRITE_PORTAL_DUNGEON,
    ITEM_SPRITE_PORTAL_FOREST,
    ITEM_SPRITE_PORTAL_BOSS_LOCKED,
    ITEM_SPRITE_PORTAL_BOSS_OPEN,
    ITEM_SPRITE_BOLT,
    ITEM_SPRITE_EXPLOSION,
};

enum {
    SHOP_ITEM_AMMO = 0,
    SHOP_ITEM_HEALTH,
    SHOP_ITEM_MAX_HP,
    SHOP_ITEM_DAMAGE,
    SHOP_ITEM_AMMO_CAP,
    SHOP_ITEM_SHOTGUN,
    SHOP_ITEM_EXIT,
    SHOP_ITEM_COUNT
};

enum {
    WEAPON_SPRITE_KNIFE = 0,
    WEAPON_SPRITE_KNIFE_SLASH,
    WEAPON_SPRITE_PISTOL,
    WEAPON_SPRITE_PISTOL_FLASH,
    WEAPON_SPRITE_FIREBALL,
    WEAPON_SPRITE_FIREBALL_CAST,
    WEAPON_SPRITE_SHOTGUN,
    WEAPON_SPRITE_SHOTGUN_FLASH,
};

enum {
    GENERATOR_ROOMS = 0,
    GENERATOR_TIGHT = 1,
    GENERATOR_BOSS = 2,
    GENERATOR_FOREST = 3,
    GENERATOR_HOUSE = 4,
    GENERATOR_COUNT = 5,
};

enum {
    PROP_BED = 0,
    PROP_TABLE,
    PROP_CHAIR,
    PROP_CHEST,
    PROP_CABINET,
    PROP_CRATE,
    PROP_BARREL,
    PROP_STASH,
};

enum {
    MUSIC_TRACK_DIES_IRAE = 0,
    MUSIC_TRACK_TOCCATA,
    MUSIC_TRACK_MASONIC_FUNERAL,
    MUSIC_TRACK_PATHETIQUE,
    MUSIC_TRACK_COUNT,
    MUSIC_TRACK_FOREST = MUSIC_TRACK_COUNT
};

enum {
    SFX_PISTOL = 0,
    SFX_FIREBALL,
    SFX_EXPLOSION,
    SFX_PICKUP,
    SFX_HURT,
    SFX_DEATH,
    SFX_MELEE,
    SFX_PORTAL,
    SFX_DOOR,
    SFX_LOCKED,
    SFX_RELIC,
    SFX_SHRINE,
    SFX_COUNT,
};

enum {
    RENDER_QUALITY_PBR = 0,
    RENDER_QUALITY_FAST = 1,
};

enum {
    RENDER_EFFECTS_OFF = 0,
    RENDER_EFFECTS_PRESET1 = 1,
    RENDER_EFFECTS_PRESET2 = 2,
    RENDER_EFFECTS_PRESET3 = 3,
    RENDER_EFFECTS_COUNT = 4,
};

enum {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_NIGHTMARE,
    DIFFICULTY_COUNT
};

#ifndef DEFAULT_RENDER_QUALITY
#define DEFAULT_RENDER_QUALITY RENDER_QUALITY_FAST
#endif

#ifndef DEFAULT_RENDER_EFFECTS
#define DEFAULT_RENDER_EFFECTS RENDER_EFFECTS_OFF
#endif

#ifndef DEFAULT_DIFFICULTY
#define DEFAULT_DIFFICULTY DIFFICULTY_NORMAL
#endif

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
    double hit_rim_timer;
    double attack_anim_timer;
    double attack_windup_timer;
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
    int relic_index;
    Vec2 pos;
} Item;

typedef struct {
    int active;
    int x;
    int y;
    int target_mode;
    int exit_to_forest;
    int relic_index;
    int boss_gate;
} Portal;

typedef struct {
    Vec2 pos;
} Torch;

typedef struct {
    int active;
    int variant;
    Vec2 pos;
} Tree;

typedef struct {
    int active;
    int variant;
    Vec2 pos;
    double half_w;
    double half_d;
    uint32_t loot_mask;
    int visited;
} House;

typedef struct {
    int active;
    int type;
    int loot_type;
    int loot_amount;
    int loot_slot;
    int looted;
    Vec2 pos;
    double half_w;
    double half_d;
    double height;
} Prop;

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
    int variant;
    Vec2 pos;
    double radius;
    double life;
    double max_life;
    double angle;
} Decal;

typedef struct {
    int active;
    int x;
    int y;
    int side;
    int variant;
    double u;
    double v;
    double width;
    double height;
    double strength;
    int next;
} WallDecal;

typedef struct {
    int active;
    int type;
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
    Tree trees[MAX_TREES];
    House houses[MAX_HOUSES];
    Prop props[MAX_PROPS];
    Decal decals[MAX_DECALS];
    WallDecal wall_decals[MAX_WALL_DECALS];
    int wall_decal_head[MAP_H][MAP_W][2];
    Particle particles[MAX_PARTICLES];
    Door doors[MAX_DOORS];
    Secret secrets[MAX_SECRETS];
    Portal portals[MAX_PORTALS];
    double time;
    int player_health;
    int ammo;
    int fireball_ammo;
    int keys;
    int selected_weapon;
    int pistol_unlocked;
    int fireball_unlocked;
    int shotgun_unlocked;
    int max_health_upgrades;
    int damage_upgrades;
    int ammo_cap_upgrades;
    int gold;
    int kills;
    double shot_cooldown;
    double weapon_flash;
    double muzzle_light;
    double shot_trace;
    double hit_marker;
    double hit_flash;
    double player_damage_flash;
    double damage_dir_x;
    double damage_dir_y;
    double screen_shake_timer;
    double screen_shake_strength;
    double pickup_flash;
    double rapid_timer;
    double damage_timer;
    int victory;
    int game_over;
    int show_automap;
    int generator_mode;
    int difficulty;
    int trainer;
    int in_dungeon;
    int current_house_index;
    int current_house_variant;
    uint32_t current_house_loot_mask;
    int relic_mask;
    int relic_count;
    int dungeon_relic_index;
    int boss_unlocked;
    double relic_flash;
    int relic_notice_count;
    double help_timer;
    int show_help;
    unsigned char discovered[MAP_H][MAP_W];
} GameState;

static int player_max_health(const GameState *game)
{
    int upgrades = game ? game->max_health_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_HEALTH_UPGRADES) upgrades = MAX_HEALTH_UPGRADES;
    return PLAYER_MAX_HEALTH + upgrades * HEALTH_UPGRADE_AMOUNT;
}

static int pistol_ammo_cap(const GameState *game)
{
    int upgrades = game ? game->ammo_cap_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_AMMO_CAP_UPGRADES) upgrades = MAX_AMMO_CAP_UPGRADES;
    return MAX_PISTOL_AMMO + upgrades * AMMO_CAP_UPGRADE_AMOUNT;
}

static int weapon_damage_bonus(const GameState *game)
{
    int upgrades = game ? game->damage_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_DAMAGE_UPGRADES) upgrades = MAX_DAMAGE_UPGRADES;
    return upgrades;
}

typedef struct {
    int valid;
    GameState game;
    Camera camera;
    int map[MAP_H][MAP_W];
    Torch torches[MAX_TORCHES];
} SavedLevel;

static int normalize_difficulty(int difficulty)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT) {
        return DIFFICULTY_NORMAL;
    }
    return difficulty;
}

static const char *difficulty_menu_text(int difficulty)
{
    switch (normalize_difficulty(difficulty)) {
    case DIFFICULTY_EASY:
        return "TRUDNOSC LATWY";
    case DIFFICULTY_HARD:
        return "TRUDNOSC TRUDNY";
    case DIFFICULTY_NIGHTMARE:
        return "TRUDNOSC NIGHTMARE";
    default:
        return "TRUDNOSC NORMAL";
    }
}

static const char *difficulty_config_text(int difficulty)
{
    switch (normalize_difficulty(difficulty)) {
    case DIFFICULTY_EASY:
        return "easy";
    case DIFFICULTY_HARD:
        return "hard";
    case DIFFICULTY_NIGHTMARE:
        return "nightmare";
    default:
        return "normal";
    }
}

static int parse_difficulty_name(const char *text, int *out_difficulty)
{
    if (strcmp(text, "easy") == 0) {
        *out_difficulty = DIFFICULTY_EASY;
        return 1;
    }
    if (strcmp(text, "normal") == 0) {
        *out_difficulty = DIFFICULTY_NORMAL;
        return 1;
    }
    if (strcmp(text, "hard") == 0) {
        *out_difficulty = DIFFICULTY_HARD;
        return 1;
    }
    if (strcmp(text, "nightmare") == 0) {
        *out_difficulty = DIFFICULTY_NIGHTMARE;
        return 1;
    }
    return 0;
}

static int adjust_difficulty(int difficulty, int delta)
{
    difficulty = normalize_difficulty(difficulty);
    difficulty = (difficulty + delta) % DIFFICULTY_COUNT;
    if (difficulty < 0) {
        difficulty += DIFFICULTY_COUNT;
    }
    return difficulty;
}

static int scale_monster_hp_for_difficulty(int hp, int difficulty)
{
    double scale = 1.0;
    switch (normalize_difficulty(difficulty)) {
    case DIFFICULTY_EASY:
        scale = 0.75;
        break;
    case DIFFICULTY_HARD:
        scale = 1.35;
        break;
    case DIFFICULTY_NIGHTMARE:
        scale = 1.80;
        break;
    default:
        break;
    }
    int scaled = (int)(hp * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

static int scale_enemy_damage_for_difficulty(const GameState *game, int damage)
{
    double scale = 1.0;
    switch (normalize_difficulty(game ? game->difficulty : DIFFICULTY_NORMAL)) {
    case DIFFICULTY_EASY:
        scale = 0.70;
        break;
    case DIFFICULTY_HARD:
        scale = 1.35;
        break;
    case DIFFICULTY_NIGHTMARE:
        scale = 1.75;
        break;
    default:
        break;
    }
    int scaled = (int)(damage * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

static void apply_player_damage_from(GameState *game, int damage, Vec2 source, Vec2 player_pos, double shake_strength)
{
    if (game->trainer) {
        return;
    }
    game->player_health -= scale_enemy_damage_for_difficulty(game, damage);
    Vec2 dir = {
        source.x - player_pos.x,
        source.y - player_pos.y,
    };
    double dir_len = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (dir_len > 0.0001) {
        dir.x /= dir_len;
        dir.y /= dir_len;
    } else {
        dir = (Vec2){0.0, 0.0};
    }
    game->damage_dir_x = dir.x;
    game->damage_dir_y = dir.y;
    game->hit_flash = 0.18;
    game->player_damage_flash = PLAYER_DAMAGE_FLASH_TIME;
    game->screen_shake_timer = SCREEN_SHAKE_TIME;
    if (shake_strength > game->screen_shake_strength) {
        game->screen_shake_strength = shake_strength;
    }
    if (game->player_health <= 0) {
        game->player_health = 0;
        game->game_over = 1;
    }
}

static void apply_player_damage(GameState *game, int damage)
{
    apply_player_damage_from(game, damage, (Vec2){0.0, 0.0}, (Vec2){0.0, 0.0}, 0.10);
}

typedef struct {
    int kind;
    int index;
    double dist;
} SpriteDraw;

typedef struct {
    double floor_ms;
    double wall_ms;
    double sprite_ms;
    double fog_ms;
    double bloom_ms;
    double post_ms;
    double total_ms;
} RenderProfile;

typedef struct {
    Uint8 *data;
    Uint32 length;
} SfxSample;

typedef struct {
    int active;
    int sfx;
    Uint32 cursor;
    double volume;
} SampleVoice;

typedef struct {
    double time;
    double drone_phase;
    double drone_mod_phase;
} FmMusicState;

typedef struct {
    double time;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    uint8_t on;
} MidiMusicEvent;

typedef struct {
    MidiMusicEvent *events;
    int event_count;
    int next_event;
    double playhead;
    double length;
} MidiMusic;

enum {
    FM_INST_BASS = 0,
    FM_INST_ORGAN,
    FM_INST_EPIANO,
    FM_INST_BELL,
    FM_INST_PAD,
    FM_INST_LEAD,
    FM_INST_COUNT
};

typedef struct {
    double mod_ratio;
    double index_base;
    double velocity_index;
    double sub_level;
    double attack;
    double decay;
    double sustain;
    double hold;
    double release;
    double level;
} FmInstrument;

typedef struct {
    int active;
    int note;
    int channel;
    int instrument;
    double velocity;
    double phase;
    double mod_phase;
    double age;
    double release_time;
    double release_level;
} FmMidiVoice;

static uint32_t framebuffer[SCREEN_W * SCREEN_H];
static uint32_t textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static uint32_t forest_wall_textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static double texture_luma[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static double texture_bump[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static double texture_bright[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static double texture_grad_x[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static double texture_grad_y[TEX_COUNT][TEX_SIZE * TEX_SIZE];
static const Material texture_materials[TEX_COUNT] = {
    {0.82, 0.00, 0.10, 0.42},
    {0.76, 0.00, 0.08, 0.36},
    {0.88, 0.00, 0.24, 0.50},
    {0.94, 0.00, 0.02, 0.22},
    {0.92, 0.00, 0.14, 0.30},
    {0.86, 0.00, 0.38, 0.54},
    {0.72, 0.00, 0.20, 0.48},
    {0.80, 0.00, 0.28, 0.56},
    {0.68, 0.00, 0.18, 0.60},
    {0.62, 0.02, 0.12, 0.66},
};
static uint32_t monster_sprites[MONSTER_TYPES][SPRITE_FRAMES][MONSTER_ANIM_FRAMES][SPRITE_SIZE * SPRITE_SIZE];
static uint32_t giant_skeleton_sprites[SPRITE_FRAMES][MONSTER_ANIM_FRAMES][GIANT_SKELETON_SPRITE_SIZE * GIANT_SKELETON_SPRITE_SIZE];
static uint32_t boss_sprites[SPRITE_FRAMES][BOSS_ANIM_FRAMES][BOSS_SPRITE_SIZE * BOSS_SPRITE_SIZE];
static uint32_t tree_sprites[TREE_TYPES][SPRITE_SIZE * SPRITE_SIZE];
static uint32_t house_textures[HOUSE_TEX_COUNT][TEX_SIZE * TEX_SIZE];
static uint32_t furniture_sprites[FURNITURE_SPRITE_COUNT][FURNITURE_SIZE * FURNITURE_SIZE];
static uint32_t relic_sprites[RELIC_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];
static uint32_t item_sprites[ITEM_SPRITE_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];
static uint32_t weapon_sprites[WEAPON_SPRITE_COUNT][WEAPON_SPRITE_SIZE * WEAPON_SPRITE_SIZE];
static uint32_t decal_sprites[DECAL_COUNT][DECAL_SIZE * DECAL_SIZE];
static uint32_t wall_decal_sprites[WALL_DECAL_COUNT][WALL_DECAL_SIZE * WALL_DECAL_SIZE];
static double z_buffer[SCREEN_W];
static double depth_buffer[SCREEN_W * SCREEN_H];
static double glow_buffer[SCREEN_W * SCREEN_H];
static double glow_blur_buffer[SCREEN_W * SCREEN_H];
static double bloom_work_buffer[SCREEN_W * SCREEN_H];
static double light_buffer[SCREEN_W * SCREEN_H];
static uint32_t post_buffer[SCREEN_W * SCREEN_H];
static double post_luma_buffer[SCREEN_W * (SCREEN_H - 14)];
static double vignette_buffer[SCREEN_W * (SCREEN_H - 14)];
static int vignette_ready = 0;
static double fog_lut[FOG_LUT_SIZE + 1];
static double forest_fog_lut[FOG_LUT_SIZE + 1];
static int fog_lut_ready = 0;
static RenderProfile *active_profile = NULL;
static int render_quality = DEFAULT_RENDER_QUALITY;
static int render_effects = DEFAULT_RENDER_EFFECTS;

static double house_min_x(const House *house);
static double house_max_x(const House *house);
static double house_min_y(const House *house);
static double house_max_y(const House *house);
static double vec_len(Vec2 v);
static Vec2 vec_norm(Vec2 v);
static double vec_dot(Vec2 a, Vec2 b);
static int parse_render_effects(const char *text, int *out_effects);
static const char *render_effects_config_text(int effects);
static void blend_rect(int x, int y, int w, int h, uint32_t color, double amount);
static GameState *active_game = NULL;
static SfxSample sfx_samples[SFX_COUNT];
static SampleVoice sample_voices[MAX_SAMPLE_VOICES];
static FmMusicState fm_music;
static MidiMusic midi_tracks[MUSIC_TRACK_COUNT];
static FmMidiVoice midi_voices[MAX_MIDI_VOICES];
static int active_music_track = MUSIC_TRACK_DIES_IRAE;
static SDL_AudioDeviceID audio_device = 0;
static double audio_rate = 44100.0;
static int sfx_volume_step = DEFAULT_SFX_VOLUME_STEP;
static int music_volume_step = DEFAULT_MUSIC_VOLUME_STEP;
static double sfx_volume = DEFAULT_SFX_VOLUME_STEP / (double)AUDIO_VOLUME_STEPS;
static double music_volume = DEFAULT_MUSIC_VOLUME_STEP / (double)AUDIO_VOLUME_STEPS;
static int level_map[MAP_H][MAP_W];
static Torch torches[MAX_TORCHES];
static double torch_flicker_cache[MAX_TORCHES];
static double torch_flicker_cache_time = 0.0;
static int torch_flicker_cache_ready = 0;
static double moon_visibility_cache[MAP_H][MAP_W];
static int moon_visibility_cache_ready = 0;
static uint32_t runtime_level_seed = LEVEL_TEST_SEED;
static int runtime_level_mode = GENERATOR_FOREST;
static int runtime_difficulty = DEFAULT_DIFFICULTY;
static int runtime_trainer = 0;
static SavedLevel saved_forest;

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

static uint32_t fog_color_for_game(const GameState *game)
{
    if (game && game->generator_mode == GENERATOR_FOREST) {
        return rgb(44, 60, 54);
    }
    return fog_color();
}

static void build_fog_luts(void)
{
    for (int i = 0; i <= FOG_LUT_SIZE; ++i) {
        double d = (FOG_LUT_MAX_DISTANCE * i) / FOG_LUT_SIZE;
        fog_lut[i] = clamp01(1.0 - exp(-d * FOG_DENSITY));
        forest_fog_lut[i] = clamp01(1.0 - exp(-d * 0.220));
    }
    fog_lut_ready = 1;
}

static double fog_amount_from_lut(double distance, const double *lut)
{
    if (distance <= 0.0) {
        return 0.0;
    }
    if (!fog_lut_ready) {
        build_fog_luts();
    }
    if (distance >= FOG_LUT_MAX_DISTANCE) {
        return 1.0;
    }

    double scaled = distance * (FOG_LUT_SIZE / FOG_LUT_MAX_DISTANCE);
    int idx = (int)scaled;
    double t = scaled - idx;
    return lut[idx] + (lut[idx + 1] - lut[idx]) * t;
}

static double fog_amount(double distance)
{
    return fog_amount_from_lut(distance, fog_lut);
}

static double fog_amount_for_game(const GameState *game, double distance)
{
    if (!game || game->generator_mode != GENERATOR_FOREST) {
        return fog_amount(distance);
    }
    return clamp01(fog_amount_from_lut(distance, forest_fog_lut) + game->relic_count * 0.035);
}

static uint32_t apply_fog(uint32_t color, double distance, double strength)
{
    return mix_color(color, fog_color(), fog_amount(distance) * clamp01(strength));
}

static uint32_t apply_game_fog(const GameState *game, uint32_t color, double distance, double strength)
{
    return mix_color(color, fog_color_for_game(game), fog_amount_for_game(game, distance) * clamp01(strength));
}

static double luminance(uint32_t color)
{
    double r = (double)((color >> 16) & 0xFFu);
    double g = (double)((color >> 8) & 0xFFu);
    double b = (double)(color & 0xFFu);
    return (r * 0.2126 + g * 0.7152 + b * 0.0722) / 255.0;
}

static uint32_t add_color(uint32_t color, uint32_t add, double amount);

static double pow5(double v)
{
    double v2 = v * v;
    return v2 * v2 * v;
}

static double texture_bump_light(int tex_idx, int tex_x, int tex_y)
{
    return texture_bump[tex_idx][tex_y * TEX_SIZE + tex_x];
}

static uint32_t apply_fast_material(uint32_t albedo, double diffuse_light, double direct_light)
{
    return shade(albedo, diffuse_light + direct_light * 0.18);
}

static uint32_t apply_pbr_material(uint32_t albedo, const Material *material, double diffuse_light, double direct_light, double bump_light, double fresnel, double bright_texel, double wetness_boost, uint32_t light_tint)
{
    double wetness = clamp01(material->wetness + wetness_boost);
    double roughness = material->roughness - wetness * 0.42;
    if (roughness < 0.08) {
        roughness = 0.08;
    }
    roughness = clamp01(roughness);

    double metallic = clamp01(material->metallic);
    double normal = 0.72 + bump_light * material->normal_strength;
    double diffuse = diffuse_light * normal * (1.0 - metallic * 0.55) * (1.0 - wetness * 0.16);
    uint32_t lit = shade(albedo, diffuse);

    double gloss = 1.0 - roughness;
    double wet_sheen = wetness * wetness * 0.58;
    double specular = direct_light * (0.035 + gloss * gloss * 0.46 + metallic * 0.40 + wet_sheen);
    specular *= 0.45 + bump_light * 0.75;
    specular *= 0.70 + fresnel * 0.85;
    specular *= 0.58 + bright_texel * 0.42;

    uint32_t highlight = mix_color(rgb(242, 242, 232), light_tint, 0.58);
    highlight = mix_color(highlight, albedo, metallic * 0.72);
    lit = add_color(lit, highlight, specular);

    if (wetness > 0.0) {
        uint32_t wet_tint = mix_color(albedo, rgb(18, 24, 24), 0.34);
        lit = mix_color(lit, wet_tint, wetness * 0.10);
    }

    return lit;
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

static void wrap_audio_phase(double *phase)
{
    double cycle = M_PI * 2.0;
    if (*phase >= cycle) {
        *phase -= cycle;
    } else if (*phase < 0.0) {
        *phase += cycle;
    }
}

static double fm_osc(double *carrier_phase, double *mod_phase, double carrier_hz, double mod_hz, double index)
{
    double sample = sin(*carrier_phase + sin(*mod_phase) * index);
    double phase_step = (M_PI * 2.0) / audio_rate;
    *carrier_phase += carrier_hz * phase_step;
    *mod_phase += mod_hz * phase_step;
    wrap_audio_phase(carrier_phase);
    wrap_audio_phase(mod_phase);
    return sample;
}

static double midi_note_frequency(int note)
{
    static const int shifts[MUSIC_TRACK_COUNT] = {-12, 0, -24, -12};
    int shift = active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? shifts[active_music_track] : -12;
    note += shift;
    if (note < 12) {
        note = 12;
    }
    if (note > 108) {
        note = 108;
    }
    return 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
}

static const FmInstrument *fm_instrument(int instrument)
{
    static const FmInstrument instruments[FM_INST_COUNT] = {
        {0.503, 1.0, 1.4, 0.30, 0.006, 8.4, 0.18, 0.22, 18.0, 0.82},
        {1.997, 2.1, 2.2, 0.12, 0.014, 2.0, 0.72, 0.30, 8.2, 0.70},
        {2.002, 1.5, 1.8, 0.18, 0.010, 4.6, 0.34, 0.42, 7.0, 0.78},
        {3.018, 3.7, 2.8, 0.04, 0.004, 7.8, 0.10, 0.32, 5.8, 0.54},
        {1.251, 0.9, 1.0, 0.22, 0.050, 1.2, 0.68, 0.58, 3.8, 0.48},
        {2.414, 3.1, 3.0, 0.08, 0.008, 5.8, 0.22, 0.24, 10.0, 0.62},
    };
    if (instrument < 0 || instrument >= FM_INST_COUNT) {
        instrument = FM_INST_EPIANO;
    }
    return &instruments[instrument];
}

static int active_music_track_index(void)
{
    return active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? active_music_track : MUSIC_TRACK_DIES_IRAE;
}

static int fm_instrument_for_note(int track, int channel, int note)
{
    if (note < 43) {
        return FM_INST_BASS;
    }
    if (track == MUSIC_TRACK_MASONIC_FUNERAL) {
        if (note >= 72) return FM_INST_BELL;
        return (channel & 1) ? FM_INST_PAD : FM_INST_EPIANO;
    }
    if (track == MUSIC_TRACK_PATHETIQUE) {
        if (note >= 76) return FM_INST_BELL;
        if (note < 55) return FM_INST_EPIANO;
        return (channel & 1) ? FM_INST_PAD : FM_INST_EPIANO;
    }
    if (track == MUSIC_TRACK_TOCCATA) {
        if (note >= 74) return FM_INST_LEAD;
        return (channel % 3) == 0 ? FM_INST_ORGAN : FM_INST_EPIANO;
    }
    if (note >= 76) {
        return FM_INST_BELL;
    }
    return (channel & 1) ? FM_INST_ORGAN : FM_INST_EPIANO;
}

static double active_music_drone_root(void)
{
    static const double roots[MUSIC_TRACK_COUNT] = {36.71, 27.50, 29.14, 24.50};
    if (active_music_track == MUSIC_TRACK_FOREST) {
        return 34.65;
    }
    int track = active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? active_music_track : MUSIC_TRACK_DIES_IRAE;
    return roots[track];
}

static double fm_midi_voice_envelope(const FmMidiVoice *voice)
{
    const FmInstrument *instrument = fm_instrument(voice->instrument);
    double attack = instrument->attack > 0.001 ? instrument->attack : FM_NOTE_ATTACK;
    double sustain = instrument->sustain;
    double hold = clamp01(voice->age / attack) *
        (sustain + (1.0 - sustain) * exp(-voice->age * instrument->decay));
    if (voice->release_time >= 0.0) {
        return voice->release_level * exp(-voice->release_time * instrument->release);
    }
    return hold;
}

static void fm_midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel)
{
    int slot = -1;
    double oldest = -1.0;
    for (int i = 0; i < MAX_MIDI_VOICES; ++i) {
        if (!midi_voices[i].active) {
            slot = i;
            break;
        }
        double age_score = midi_voices[i].release_time >= 0.0 ? midi_voices[i].age + 1000.0 : midi_voices[i].age;
        if (age_score > oldest) {
            oldest = age_score;
            slot = i;
        }
    }

    if (slot < 0) {
        return;
    }
    midi_voices[slot] = (FmMidiVoice){
        .active = 1,
        .note = note,
        .channel = channel,
        .instrument = fm_instrument_for_note(active_music_track_index(), channel, note),
        .velocity = clamp01(velocity / 127.0),
        .phase = 0.0,
        .mod_phase = 0.0,
        .age = 0.0,
        .release_time = -1.0,
        .release_level = 0.0,
    };
}

static void fm_midi_note_off(uint8_t note, uint8_t channel)
{
    for (int i = 0; i < MAX_MIDI_VOICES; ++i) {
        FmMidiVoice *voice = &midi_voices[i];
        if (voice->active && voice->note == note && voice->channel == channel && voice->release_time < 0.0) {
            voice->release_level = fm_midi_voice_envelope(voice);
            voice->release_time = 0.0;
        }
    }
}

static double fm_midi_voices_sample(void)
{
    double sample = 0.0;
    double dt = 1.0 / audio_rate;
    double phase_step = M_PI * 2.0 / audio_rate;

    for (int i = 0; i < MAX_MIDI_VOICES; ++i) {
        FmMidiVoice *voice = &midi_voices[i];
        if (!voice->active) {
            continue;
        }

        const FmInstrument *instrument = fm_instrument(voice->instrument);
        if (voice->release_time < 0.0 && voice->age >= instrument->hold) {
            voice->release_level = fm_midi_voice_envelope(voice);
            voice->release_time = 0.0;
        }

        double env = fm_midi_voice_envelope(voice);
        if (voice->release_time >= 0.0 && env < 0.001) {
            voice->active = 0;
            continue;
        }

        double freq = midi_note_frequency(voice->note);
        double ratio = instrument->mod_ratio * (voice->note & 1 ? 1.003 : 0.997);
        double index = instrument->index_base + voice->velocity * instrument->velocity_index;
        double tone = sin(voice->phase + sin(voice->mod_phase) * index);
        tone += sin(voice->phase * 0.5) * instrument->sub_level;
        sample += tone * env * voice->velocity * instrument->level * 0.090;

        voice->phase += freq * phase_step;
        voice->mod_phase += freq * ratio * phase_step;
        wrap_audio_phase(&voice->phase);
        wrap_audio_phase(&voice->mod_phase);
        voice->age += dt;
        if (voice->release_time >= 0.0) {
            voice->release_time += dt;
        }
    }
    return sample;
}

static double fm_midi_music_sample(void)
{
    if (active_music_track < 0 || active_music_track >= MUSIC_TRACK_COUNT) {
        return 0.0;
    }
    MidiMusic *music = &midi_tracks[active_music_track];
    if (!music->events || music->event_count <= 0 || music->length <= 0.0) {
        return 0.0;
    }

    while (music->next_event < music->event_count &&
           music->events[music->next_event].time <= music->playhead) {
        const MidiMusicEvent *event = &music->events[music->next_event++];
        if (event->on) {
            fm_midi_note_on(event->note, event->velocity, event->channel);
        } else {
            fm_midi_note_off(event->note, event->channel);
        }
    }

    double sample = fm_midi_voices_sample();
    music->playhead += 1.0 / audio_rate;
    if (music->playhead >= music->length) {
        music->playhead = 0.0;
        music->next_event = 0;
        memset(midi_voices, 0, sizeof(midi_voices));
    }
    return sample;
}

static double fm_music_sample(void)
{
    double root = active_music_drone_root();
    double fade = clamp01(fm_music.time / 3.0);
    double pulse = 0.78 + 0.22 * sin(fm_music.time * M_PI * 0.11);
    double drone = fm_osc(&fm_music.drone_phase, &fm_music.drone_mod_phase, root, root * 1.503, 3.2);
    double midi = fm_midi_music_sample();
    fm_music.time += 1.0 / audio_rate;

    double sample = (drone * 0.08 * pulse + midi * 1.18) * fade * FM_MUSIC_VOLUME * music_volume;
    return sample / (1.0 + fabs(sample) * 0.45);
}

static int clamp_volume_step(int step)
{
    if (step < 0) {
        return 0;
    }
    if (step > AUDIO_VOLUME_STEPS) {
        return AUDIO_VOLUME_STEPS;
    }
    return step;
}

static void set_audio_volume_steps(int sfx_step, int music_step)
{
    int next_sfx = clamp_volume_step(sfx_step);
    int next_music = clamp_volume_step(music_step);
    if (audio_device) {
        SDL_LockAudioDevice(audio_device);
    }
    sfx_volume_step = next_sfx;
    music_volume_step = next_music;
    sfx_volume = sfx_volume_step / (double)AUDIO_VOLUME_STEPS;
    music_volume = music_volume_step / (double)AUDIO_VOLUME_STEPS;
    if (audio_device) {
        SDL_UnlockAudioDevice(audio_device);
    }
}

static void reset_midi_track(MidiMusic *music)
{
    music->playhead = 0.0;
    music->next_event = 0;
}

static void set_active_music_track(int track)
{
    if (track < 0 || track > MUSIC_TRACK_FOREST) {
        track = MUSIC_TRACK_FOREST;
    }
    if (audio_device) {
        SDL_LockAudioDevice(audio_device);
    }
    active_music_track = track;
    if (active_music_track < MUSIC_TRACK_COUNT) {
        reset_midi_track(&midi_tracks[active_music_track]);
    }
    memset(midi_voices, 0, sizeof(midi_voices));
    fm_music.time = 0.0;
    fm_music.drone_phase = 0.0;
    fm_music.drone_mod_phase = 0.0;
    if (audio_device) {
        SDL_UnlockAudioDevice(audio_device);
    }
}

static int music_track_for_relic(int relic_index)
{
    static const int tracks[RELIC_COUNT] = {
        MUSIC_TRACK_DIES_IRAE,
        MUSIC_TRACK_MASONIC_FUNERAL,
        MUSIC_TRACK_PATHETIQUE,
        MUSIC_TRACK_TOCCATA,
    };
    if (relic_index < 0 || relic_index >= RELIC_COUNT) {
        return MUSIC_TRACK_TOCCATA;
    }
    return tracks[relic_index];
}

static void audio_callback(void *userdata, uint8_t *stream, int len)
{
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    int samples = len / (int)sizeof(int16_t);

    for (int i = 0; i < samples; ++i) {
        double sample = fm_music_sample();
        for (int v = 0; v < MAX_SAMPLE_VOICES; ++v) {
            SampleVoice *voice = &sample_voices[v];
            if (!voice->active) {
                continue;
            }
            const SfxSample *sfx = &sfx_samples[voice->sfx];
            if (voice->cursor + sizeof(int16_t) > sfx->length) {
                voice->active = 0;
                continue;
            }
            int16_t src;
            memcpy(&src, sfx->data + voice->cursor, sizeof(src));
            double env = 1.0;
            Uint32 samples_left = (sfx->length - voice->cursor) / (Uint32)sizeof(int16_t);
            if (samples_left < 512) {
                env = samples_left / 512.0;
            }
            sample += (src / 32768.0) * voice->volume * env * sfx_volume;
            voice->cursor += (Uint32)sizeof(int16_t);
        }
        if (sample > 1.0) sample = 1.0;
        if (sample < -1.0) sample = -1.0;
        out[i] = (int16_t)(sample * 32767.0);
    }
}

static const char *sfx_path(int sfx)
{
    static const char *paths[SFX_COUNT] = {
        "assets/sfx/pistol.wav",
        "assets/sfx/fireball.wav",
        "assets/sfx/explosion.wav",
        "assets/sfx/pickup.wav",
        "assets/sfx/hurt.wav",
        "assets/sfx/death.wav",
        "assets/sfx/melee.wav",
        "assets/sfx/portal.wav",
        "assets/sfx/door.wav",
        "assets/sfx/locked.wav",
        "assets/sfx/relic.wav",
        "assets/sfx/shrine.wav",
    };
    return paths[sfx];
}

static void free_sfx(void)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        if (sfx_samples[i].data) {
            SDL_FreeWAV(sfx_samples[i].data);
            sfx_samples[i].data = NULL;
            sfx_samples[i].length = 0;
        }
    }
}

static int load_sfx_samples(const SDL_AudioSpec *device_spec)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        SDL_AudioSpec spec;
        Uint8 *data = NULL;
        Uint32 length = 0;
        const char *path = sfx_path(i);
        if (!SDL_LoadWAV(path, &spec, &data, &length)) {
            fprintf(stderr, "error: cannot load required sound %s: %s\n", path, SDL_GetError());
            free_sfx();
            return 0;
        }
        if (spec.freq != device_spec->freq ||
            spec.format != device_spec->format ||
            spec.channels != device_spec->channels) {
            fprintf(stderr,
                    "error: sound %s must be %d Hz mono signed 16-bit PCM WAV\n",
                    path,
                    device_spec->freq);
            SDL_FreeWAV(data);
            free_sfx();
            return 0;
        }
        sfx_samples[i].data = data;
        sfx_samples[i].length = length;
    }
    return 1;
}

static int verify_sfx_assets(void)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        const char *path = sfx_path(i);
        FILE *f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "error: required sound asset is missing: %s\n", path);
            return 0;
        }

        uint8_t header[44];
        size_t read = fread(header, 1, sizeof(header), f);
        fclose(f);
        if (read != sizeof(header) ||
            memcmp(header, "RIFF", 4) != 0 ||
            memcmp(header + 8, "WAVE", 4) != 0 ||
            memcmp(header + 12, "fmt ", 4) != 0) {
            fprintf(stderr, "error: required sound asset is not a PCM WAV: %s\n", path);
            return 0;
        }

        int channels = header[22] | (header[23] << 8);
        int rate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
        int bits = header[34] | (header[35] << 8);
        if (channels != 1 || rate != 44100 || bits != 16) {
            fprintf(stderr, "error: required sound asset must be 44.1 kHz mono 16-bit WAV: %s\n", path);
            return 0;
        }
    }
    return 1;
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
    memset(&fm_music, 0, sizeof(fm_music));
    memset(midi_voices, 0, sizeof(midi_voices));
    if (!load_sfx_samples(&have)) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
        return 0;
    }
    SDL_PauseAudioDevice(audio_device, 0);
    return 1;
}

static void shutdown_audio(void)
{
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    free_sfx();
}

static void play_sfx(int sfx, double volume)
{
    if (!audio_device) {
        return;
    }
    SDL_LockAudioDevice(audio_device);
    for (int i = 0; i < MAX_SAMPLE_VOICES; ++i) {
        SampleVoice *voice = &sample_voices[i];
        if (!voice->active) {
            voice->active = 1;
            voice->sfx = sfx;
            voice->cursor = 0;
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
             width == TEX_SIZE * TEX_ATLAS_COLS &&
             height == TEX_SIZE * TEX_ATLAS_ROWS &&
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
        int tile_x = (tex % TEX_ATLAS_COLS) * TEX_SIZE;
        int tile_y = (tex / TEX_ATLAS_COLS) * TEX_SIZE;

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

static void build_texture_luts(void)
{
    for (int tex = 0; tex < TEX_COUNT; ++tex) {
        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                int idx = y * TEX_SIZE + x;
                texture_luma[tex][idx] = luminance(textures[tex][idx]);
            }
        }
        for (int y = 0; y < TEX_SIZE; ++y) {
            int uy = (y - 1) & (TEX_SIZE - 1);
            int dy = (y + 1) & (TEX_SIZE - 1);
            for (int x = 0; x < TEX_SIZE; ++x) {
                int lx = (x - 1) & (TEX_SIZE - 1);
                int rx = (x + 1) & (TEX_SIZE - 1);
                int idx = y * TEX_SIZE + x;
                double h = texture_luma[tex][idx];
                double h_l = texture_luma[tex][y * TEX_SIZE + lx];
                double h_r = texture_luma[tex][y * TEX_SIZE + rx];
                double h_u = texture_luma[tex][uy * TEX_SIZE + x];
                double h_d = texture_luma[tex][dy * TEX_SIZE + x];
                double edge = fabs(h_r - h_l) + fabs(h_d - h_u);
                double local = h - (h_l + h_r + h_u + h_d) * 0.25;
                texture_bump[tex][idx] = clamp01(0.50 + local * 1.55 + edge * 0.34);
                texture_bright[tex][idx] = smooth01((h - 0.24) / 0.62);
                texture_grad_x[tex][idx] = h_r - h_l;
                texture_grad_y[tex][idx] = h_d - h_u;
            }
        }
        uint32_t forest_rows[TEX_SIZE];
        for (int y = 0; y < TEX_SIZE; ++y) {
            int r = 0;
            int g = 0;
            int b = 0;
            int samples = 0;
            for (int oy = -4; oy <= 4; ++oy) {
                int sy = y + oy;
                if (sy < 0) {
                    sy = 0;
                } else if (sy >= TEX_SIZE) {
                    sy = TEX_SIZE - 1;
                }
                for (int sx = 0; sx < TEX_SIZE; ++sx) {
                    uint32_t c = textures[tex][sy * TEX_SIZE + sx];
                    r += (int)((c >> 16) & 0xFFu);
                    g += (int)((c >> 8) & 0xFFu);
                    b += (int)(c & 0xFFu);
                    samples++;
                }
            }
            double height_tint = 0.10 + smooth01((double)y / (TEX_SIZE - 1)) * 0.10;
            forest_rows[y] = mix_color(rgb(r / samples, g / samples, b / samples), rgb(38, 52, 46), height_tint);
        }
        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                forest_wall_textures[tex][y * TEX_SIZE + x] = forest_rows[y];
            }
        }
    }
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
             width == SPRITE_SIZE * MONSTER_ANIM_FRAMES &&
             height == SPRITE_SIZE * SPRITE_FRAMES * MONSTER_TYPES &&
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
        int type_y = type * SPRITE_FRAMES * SPRITE_SIZE;
        for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
            int frame_y = type_y + frame * SPRITE_SIZE;
            for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
                int frame_x = anim * SPRITE_SIZE;
                for (int y = 0; y < SPRITE_SIZE; ++y) {
                    for (int x = 0; x < SPRITE_SIZE; ++x) {
                        size_t src = ((size_t)(frame_y + y) * (size_t)width + (size_t)(frame_x + x)) * 3;
                        monster_sprites[type][frame][anim][y * SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
                    }
                }
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_giant_skeleton_atlas(const char *path)
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
             width == GIANT_SKELETON_SPRITE_SIZE * MONSTER_ANIM_FRAMES &&
             height == GIANT_SKELETON_SPRITE_SIZE * SPRITE_FRAMES &&
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

    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        int frame_y = frame * GIANT_SKELETON_SPRITE_SIZE;
        for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
            int frame_x = anim * GIANT_SKELETON_SPRITE_SIZE;
            for (int y = 0; y < GIANT_SKELETON_SPRITE_SIZE; ++y) {
                for (int x = 0; x < GIANT_SKELETON_SPRITE_SIZE; ++x) {
                    size_t src = ((size_t)(frame_y + y) * (size_t)width + (size_t)(frame_x + x)) * 3;
                    giant_skeleton_sprites[frame][anim][y * GIANT_SKELETON_SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
                }
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_boss_atlas(const char *path)
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
             width == BOSS_SPRITE_SIZE * BOSS_ANIM_FRAMES &&
             height == BOSS_SPRITE_SIZE * SPRITE_FRAMES &&
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

    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        int frame_y = frame * BOSS_SPRITE_SIZE;
        for (int anim = 0; anim < BOSS_ANIM_FRAMES; ++anim) {
            int frame_x = anim * BOSS_SPRITE_SIZE;
            for (int y = 0; y < BOSS_SPRITE_SIZE; ++y) {
                for (int x = 0; x < BOSS_SPRITE_SIZE; ++x) {
                    size_t src = ((size_t)(frame_y + y) * (size_t)width + (size_t)(frame_x + x)) * 3;
                    boss_sprites[frame][anim][y * BOSS_SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
                }
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_tree_atlas(const char *path)
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
             width == SPRITE_SIZE * TREE_TYPES &&
             height == SPRITE_SIZE &&
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

    for (int type = 0; type < TREE_TYPES; ++type) {
        int type_x = type * SPRITE_SIZE;
        for (int y = 0; y < SPRITE_SIZE; ++y) {
            for (int x = 0; x < SPRITE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(type_x + x)) * 3;
                tree_sprites[type][y * SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_house_atlas(const char *path)
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
             width == TEX_SIZE * HOUSE_ATLAS_COLS &&
             height == TEX_SIZE * HOUSE_ATLAS_ROWS &&
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

    for (int tex = 0; tex < HOUSE_TEX_COUNT; ++tex) {
        int tile_x = (tex % HOUSE_ATLAS_COLS) * TEX_SIZE;
        int tile_y = (tex / HOUSE_ATLAS_COLS) * TEX_SIZE;

        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                size_t src = ((size_t)(tile_y + y) * (size_t)width + (size_t)(tile_x + x)) * 3;
                house_textures[tex][y * TEX_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_furniture_atlas(const char *path)
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
             width == FURNITURE_SIZE * FURNITURE_SPRITE_COUNT &&
             height == FURNITURE_SIZE &&
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

    for (int sprite = 0; sprite < FURNITURE_SPRITE_COUNT; ++sprite) {
        int sprite_x = sprite * FURNITURE_SIZE;
        for (int y = 0; y < FURNITURE_SIZE; ++y) {
            for (int x = 0; x < FURNITURE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(sprite_x + x)) * 3;
                furniture_sprites[sprite][y * FURNITURE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_relic_atlas(const char *path)
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
             width == PROJECTILE_SIZE * RELIC_COUNT &&
             height == PROJECTILE_SIZE &&
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

    for (int relic = 0; relic < RELIC_COUNT; ++relic) {
        int relic_x = relic * PROJECTILE_SIZE;
        for (int y = 0; y < PROJECTILE_SIZE; ++y) {
            for (int x = 0; x < PROJECTILE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(relic_x + x)) * 3;
                relic_sprites[relic][y * PROJECTILE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_item_atlas(const char *path)
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
             width == PROJECTILE_SIZE * ITEM_SPRITE_COUNT &&
             height == PROJECTILE_SIZE &&
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

    for (int sprite = 0; sprite < ITEM_SPRITE_COUNT; ++sprite) {
        int sprite_x = sprite * PROJECTILE_SIZE;
        for (int y = 0; y < PROJECTILE_SIZE; ++y) {
            for (int x = 0; x < PROJECTILE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(sprite_x + x)) * 3;
                item_sprites[sprite][y * PROJECTILE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_weapon_atlas(const char *path)
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
             width == WEAPON_SPRITE_SIZE * WEAPON_SPRITE_COUNT &&
             height == WEAPON_SPRITE_SIZE &&
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

    for (int sprite = 0; sprite < WEAPON_SPRITE_COUNT; ++sprite) {
        int sprite_x = sprite * WEAPON_SPRITE_SIZE;
        for (int y = 0; y < WEAPON_SPRITE_SIZE; ++y) {
            for (int x = 0; x < WEAPON_SPRITE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(sprite_x + x)) * 3;
                weapon_sprites[sprite][y * WEAPON_SPRITE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_decal_atlas(const char *path)
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
             width == DECAL_SIZE * DECAL_COUNT &&
             height == DECAL_SIZE &&
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

    for (int decal = 0; decal < DECAL_COUNT; ++decal) {
        int decal_x = decal * DECAL_SIZE;
        for (int y = 0; y < DECAL_SIZE; ++y) {
            for (int x = 0; x < DECAL_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(decal_x + x)) * 3;
                decal_sprites[decal][y * DECAL_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

static int load_wall_decal_atlas(const char *path)
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
             width == WALL_DECAL_SIZE * WALL_DECAL_COUNT &&
             height == WALL_DECAL_SIZE &&
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

    for (int decal = 0; decal < WALL_DECAL_COUNT; ++decal) {
        int decal_x = decal * WALL_DECAL_SIZE;
        for (int y = 0; y < WALL_DECAL_SIZE; ++y) {
            for (int x = 0; x < WALL_DECAL_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(decal_x + x)) * 3;
                wall_decal_sprites[decal][y * WALL_DECAL_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
            }
        }
    }

    free(pixels);
    return 1;
}

typedef struct {
    uint32_t tick;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    uint8_t on;
} MidiRawEvent;

typedef struct {
    uint32_t tick;
    uint32_t us_per_quarter;
} MidiTempoEvent;

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)((data[0] << 8) | data[1]);
}

static uint32_t read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static int read_midi_vlq(const uint8_t *data, size_t end, size_t *pos, uint32_t *value)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        if (*pos >= end) {
            return 0;
        }
        uint8_t b = data[(*pos)++];
        v = (v << 7) | (uint32_t)(b & 0x7Fu);
        if ((b & 0x80u) == 0) {
            *value = v;
            return 1;
        }
    }
    return 0;
}

static int append_midi_raw_event(MidiRawEvent **events, int *count, int *capacity, MidiRawEvent event)
{
    if (*count >= *capacity) {
        int next_capacity = *capacity ? *capacity * 2 : 1024;
        MidiRawEvent *next = realloc(*events, (size_t)next_capacity * sizeof(**events));
        if (!next) {
            return 0;
        }
        *events = next;
        *capacity = next_capacity;
    }
    (*events)[(*count)++] = event;
    return 1;
}

static int append_midi_tempo_event(MidiTempoEvent **events, int *count, int *capacity, MidiTempoEvent event)
{
    if (*count >= *capacity) {
        int next_capacity = *capacity ? *capacity * 2 : 8;
        MidiTempoEvent *next = realloc(*events, (size_t)next_capacity * sizeof(**events));
        if (!next) {
            return 0;
        }
        *events = next;
        *capacity = next_capacity;
    }
    (*events)[(*count)++] = event;
    return 1;
}

static int compare_midi_raw_events(const void *a, const void *b)
{
    const MidiRawEvent *ea = (const MidiRawEvent *)a;
    const MidiRawEvent *eb = (const MidiRawEvent *)b;
    if (ea->tick < eb->tick) return -1;
    if (ea->tick > eb->tick) return 1;
    if (ea->on != eb->on) return ea->on ? 1 : -1;
    if (ea->channel != eb->channel) return (int)ea->channel - (int)eb->channel;
    return (int)ea->note - (int)eb->note;
}

static int compare_midi_tempo_events(const void *a, const void *b)
{
    const MidiTempoEvent *ea = (const MidiTempoEvent *)a;
    const MidiTempoEvent *eb = (const MidiTempoEvent *)b;
    if (ea->tick < eb->tick) return -1;
    if (ea->tick > eb->tick) return 1;
    return 0;
}

static int parse_midi_track(const uint8_t *data,
                            size_t start,
                            size_t end,
                            MidiRawEvent **raw_events,
                            int *raw_count,
                            int *raw_capacity,
                            MidiTempoEvent **tempo_events,
                            int *tempo_count,
                            int *tempo_capacity)
{
    size_t pos = start;
    uint32_t tick = 0;
    uint8_t running_status = 0;

    while (pos < end) {
        uint32_t delta = 0;
        if (!read_midi_vlq(data, end, &pos, &delta)) {
            return 0;
        }
        tick += delta;
        if (pos >= end) {
            return 0;
        }

        uint8_t status = data[pos];
        if (status & 0x80u) {
            pos++;
            running_status = status;
        } else if (running_status) {
            status = running_status;
        } else {
            return 0;
        }

        if (status == 0xFFu) {
            if (pos >= end) {
                return 0;
            }
            uint8_t meta_type = data[pos++];
            uint32_t length = 0;
            if (!read_midi_vlq(data, end, &pos, &length) || pos + length > end) {
                return 0;
            }
            if (meta_type == 0x51u && length == 3) {
                uint32_t us_per_quarter =
                    ((uint32_t)data[pos] << 16) | ((uint32_t)data[pos + 1] << 8) | (uint32_t)data[pos + 2];
                if (!append_midi_tempo_event(tempo_events, tempo_count, tempo_capacity,
                                             (MidiTempoEvent){tick, us_per_quarter})) {
                    return 0;
                }
            }
            pos += length;
            continue;
        }

        if (status == 0xF0u || status == 0xF7u) {
            uint32_t length = 0;
            if (!read_midi_vlq(data, end, &pos, &length) || pos + length > end) {
                return 0;
            }
            pos += length;
            continue;
        }

        uint8_t event_type = status & 0xF0u;
        uint8_t channel = status & 0x0Fu;
        if (event_type == 0x80u || event_type == 0x90u) {
            if (pos + 2 > end) {
                return 0;
            }
            uint8_t note = data[pos++];
            uint8_t velocity = data[pos++];
            uint8_t on = event_type == 0x90u && velocity > 0;
            if (!append_midi_raw_event(raw_events, raw_count, raw_capacity,
                                       (MidiRawEvent){tick, note, velocity, channel, on})) {
                return 0;
            }
        } else if (event_type == 0xA0u || event_type == 0xB0u || event_type == 0xE0u) {
            if (pos + 2 > end) {
                return 0;
            }
            pos += 2;
        } else if (event_type == 0xC0u || event_type == 0xD0u) {
            if (pos + 1 > end) {
                return 0;
            }
            pos += 1;
        } else {
            return 0;
        }
    }
    return pos == end;
}

static double midi_tick_to_seconds(uint32_t tick, const MidiTempoEvent *tempos, int tempo_count, int ticks_per_quarter)
{
    uint32_t last_tick = 0;
    uint32_t tempo = 500000;
    double seconds = 0.0;

    for (int i = 0; i < tempo_count; ++i) {
        if (tempos[i].tick > tick) {
            break;
        }
        if (tempos[i].tick > last_tick) {
            seconds += (double)(tempos[i].tick - last_tick) * (double)tempo / 1000000.0 / (double)ticks_per_quarter;
            last_tick = tempos[i].tick;
        }
        tempo = tempos[i].us_per_quarter;
    }

    seconds += (double)(tick - last_tick) * (double)tempo / 1000000.0 / (double)ticks_per_quarter;
    return seconds;
}

static void free_midi_tracks(void)
{
    for (int i = 0; i < MUSIC_TRACK_COUNT; ++i) {
        free(midi_tracks[i].events);
    }
    memset(midi_tracks, 0, sizeof(midi_tracks));
    memset(midi_voices, 0, sizeof(midi_voices));
    active_music_track = MUSIC_TRACK_FOREST;
}

static int load_midi_music(const char *path, MidiMusic *music)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open required MIDI music asset %s: %s\n", path, strerror(errno));
        return 0;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long file_size = ftell(f);
    if (file_size < 22 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }

    uint8_t *data = malloc((size_t)file_size);
    if (!data) {
        fclose(f);
        return 0;
    }
    int ok = fread(data, 1, (size_t)file_size, f) == (size_t)file_size;
    fclose(f);
    if (!ok) {
        free(data);
        return 0;
    }

    MidiRawEvent *raw_events = NULL;
    MidiTempoEvent *tempo_events = NULL;
    int raw_count = 0;
    int raw_capacity = 0;
    int tempo_count = 0;
    int tempo_capacity = 0;

    size_t pos = 0;
    if (memcmp(data, "MThd", 4) != 0 || read_be32(data + 4) < 6) {
        ok = 0;
    } else {
        uint16_t format = read_be16(data + 8);
        uint16_t track_count = read_be16(data + 10);
        uint16_t division = read_be16(data + 12);
        uint32_t header_size = read_be32(data + 4);
        pos = 8u + header_size;
        if ((format != 0 && format != 1) || track_count == 0 || (division & 0x8000u) != 0 || pos > (size_t)file_size) {
            ok = 0;
        } else {
            int ticks_per_quarter = (int)division;
            for (uint16_t track = 0; ok && track < track_count; ++track) {
                if (pos + 8 > (size_t)file_size || memcmp(data + pos, "MTrk", 4) != 0) {
                    ok = 0;
                    break;
                }
                uint32_t track_size = read_be32(data + pos + 4);
                pos += 8;
                if (pos + track_size > (size_t)file_size) {
                    ok = 0;
                    break;
                }
                ok = parse_midi_track(data, pos, pos + track_size,
                                      &raw_events, &raw_count, &raw_capacity,
                                      &tempo_events, &tempo_count, &tempo_capacity);
                pos += track_size;
            }

            if (ok) {
                int has_start_tempo = 0;
                for (int i = 0; i < tempo_count; ++i) {
                    if (tempo_events[i].tick == 0) {
                        has_start_tempo = 1;
                        break;
                    }
                }
                if (!has_start_tempo &&
                    !append_midi_tempo_event(&tempo_events, &tempo_count, &tempo_capacity,
                                             (MidiTempoEvent){0, 500000})) {
                    ok = 0;
                }
            }
            if (ok && raw_count > 0) {
                qsort(raw_events, (size_t)raw_count, sizeof(*raw_events), compare_midi_raw_events);
                qsort(tempo_events, (size_t)tempo_count, sizeof(*tempo_events), compare_midi_tempo_events);

                MidiMusicEvent *events = malloc((size_t)raw_count * sizeof(*events));
                if (!events) {
                    ok = 0;
                } else {
                    for (int i = 0; i < raw_count; ++i) {
                        events[i] = (MidiMusicEvent){
                            midi_tick_to_seconds(raw_events[i].tick, tempo_events, tempo_count, ticks_per_quarter),
                            raw_events[i].note,
                            raw_events[i].velocity,
                            raw_events[i].channel,
                            raw_events[i].on,
                        };
                    }
                    free(music->events);
                    music->events = events;
                    music->event_count = raw_count;
                    music->length = events[raw_count - 1].time + 3.0;
                    music->next_event = 0;
                    music->playhead = 0.0;
                }
            }
        }
    }

    free(raw_events);
    free(tempo_events);
    free(data);

    if (!ok || music->event_count == 0) {
        fprintf(stderr, "error: cannot parse required MIDI music asset %s\n", path);
        free(music->events);
        memset(music, 0, sizeof(*music));
        return 0;
    }
    return 1;
}

static int load_music_assets(void)
{
    static const char *paths[MUSIC_TRACK_COUNT] = {
        MUSIC_DIES_IRAE_PATH,
        MUSIC_TOCCATA_PATH,
        MUSIC_MASONIC_FUNERAL_PATH,
        MUSIC_PATHETIQUE_PATH,
    };

    free_midi_tracks();
    for (int i = 0; i < MUSIC_TRACK_COUNT; ++i) {
        if (!load_midi_music(paths[i], &midi_tracks[i])) {
            free_midi_tracks();
            return 0;
        }
    }
    set_active_music_track(MUSIC_TRACK_FOREST);
    return 1;
}

static int init_assets(void)
{
    if (!load_texture_atlas(TEXTURE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required texture atlas %s\n", TEXTURE_ATLAS_PATH);
        return 0;
    }
    build_texture_luts();
    if (!load_monster_atlas(MONSTER_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required monster atlas %s\n", MONSTER_ATLAS_PATH);
        return 0;
    }
    if (!load_giant_skeleton_atlas(GIANT_SKELETON_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required giant skeleton atlas %s\n", GIANT_SKELETON_ATLAS_PATH);
        return 0;
    }
    if (!load_boss_atlas(BOSS_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required boss atlas %s\n", BOSS_ATLAS_PATH);
        return 0;
    }
    if (!load_tree_atlas(TREE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required tree atlas %s\n", TREE_ATLAS_PATH);
        return 0;
    }
    if (!load_house_atlas(HOUSE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required house atlas %s\n", HOUSE_ATLAS_PATH);
        return 0;
    }
    if (!load_furniture_atlas(FURNITURE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required furniture atlas %s\n", FURNITURE_ATLAS_PATH);
        return 0;
    }
    if (!load_relic_atlas(RELIC_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required relic atlas %s\n", RELIC_ATLAS_PATH);
        return 0;
    }
    if (!load_item_atlas(ITEM_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required item atlas %s\n", ITEM_ATLAS_PATH);
        return 0;
    }
    if (!load_weapon_atlas(WEAPON_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required weapon atlas %s\n", WEAPON_ATLAS_PATH);
        return 0;
    }
    if (!load_decal_atlas(DECAL_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required decal atlas %s\n", DECAL_ATLAS_PATH);
        return 0;
    }
    if (!load_wall_decal_atlas(WALL_DECAL_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required wall decal atlas %s\n", WALL_DECAL_ATLAS_PATH);
        return 0;
    }
    if (!load_music_assets()) {
        return 0;
    }
    return 1;
}

static const Door *door_at_tile(const GameState *game, int x, int y)
{
    if (!game) {
        return NULL;
    }
    for (int i = 0; i < MAX_DOORS; ++i) {
        const Door *door = &game->doors[i];
        if (door->x == x && door->y == y) {
            return door;
        }
    }
    return NULL;
}

static int map_at(int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
        return 1;
    }
    if (active_game) {
        const Door *door = door_at_tile(active_game, x, y);
        if (door && !door->open) {
            return door->locked ? WALL_LOCKED_DOOR : WALL_DOOR;
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

static int is_door_wall(int wall)
{
    return wall == WALL_DOOR || wall == WALL_LOCKED_DOOR;
}

static int wall_texture_index(int wall)
{
    if (wall == WALL_DOOR) {
        return TEX_DOOR;
    }
    if (wall == WALL_LOCKED_DOOR) {
        return TEX_LOCKED_DOOR;
    }
    int tex_idx = wall - 1;
    if (tex_idx < 0) {
        tex_idx = 0;
    }
    return tex_idx % TEX_COUNT;
}

static double ray_wall_u(const Camera *cam, double ray_dir_x, double ray_dir_y, double perp_wall_dist, int side)
{
    double wall_x = side == 0
        ? cam->pos.y + perp_wall_dist * ray_dir_y
        : cam->pos.x + perp_wall_dist * ray_dir_x;
    wall_x -= floor(wall_x);

    if ((side == 0 && ray_dir_x > 0.0) || (side == 1 && ray_dir_y < 0.0)) {
        wall_x = 1.0 - wall_x;
    }
    return wall_x;
}

static int ray_hits_opening_door(const Door *door, double wall_u)
{
    if (!door || !door->opening || door->locked) {
        return 1;
    }
    return wall_u >= clamp01(door->open_amount);
}

static void prepare_torch_flicker_cache(double time)
{
    if (torch_flicker_cache_ready && torch_flicker_cache_time == time) {
        return;
    }
    for (int i = 0; i < MAX_TORCHES; ++i) {
        torch_flicker_cache[i] = 1.04 + sin(time * 7.0 + i * 1.73) * 0.10 + sin(time * 13.0 + i * 0.41) * 0.05;
    }
    torch_flicker_cache_time = time;
    torch_flicker_cache_ready = 1;
}

static double torch_light_at(double x, double y, double time)
{
    double light = 0.0;
    prepare_torch_flicker_cache(time);

    for (int i = 0; i < MAX_TORCHES; ++i) {
        double dx = x - torches[i].pos.x;
        double dy = y - torches[i].pos.y;
        double dist2 = dx * dx + dy * dy;
        if (dist2 > 18.0) {
            continue;
        }

        double fade = 1.0 - smooth01((dist2 - 5.0) / 13.0);
        light += torch_flicker_cache[i] * 1.45 * fade / (1.0 + dist2 * 0.62);
    }

    return clamp01(light);
}

static double player_torch_light_at(const Camera *cam, const GameState *game, double x, double y)
{
    if (game->generator_mode == GENERATOR_FOREST) {
        return 0.0;
    }

    Vec2 torch = {
        cam->pos.x + cam->dir.x * 0.42 + cam->plane.x * 0.24,
        cam->pos.y + cam->dir.y * 0.42 + cam->plane.y * 0.24,
    };
    double dx = x - torch.x;
    double dy = y - torch.y;
    double dist2 = dx * dx + dy * dy;
    if (dist2 > 11.0) {
        return 0.0;
    }

    double dist = sqrt(dist2);
    double dir_x = dist > 0.001 ? dx / dist : cam->dir.x;
    double dir_y = dist > 0.001 ? dy / dist : cam->dir.y;
    double forward = clamp01((dir_x * cam->dir.x + dir_y * cam->dir.y) * 0.5 + 0.5);
    double fade = 1.0 - smooth01((dist2 - 1.2) / 9.8);
    double flicker = 0.96 + sin(game->time * 8.5) * 0.055 + sin(game->time * 17.0) * 0.025;
    return clamp01(flicker * (0.48 + forward * 0.20) * fade / (1.0 + dist2 * 0.34));
}

static double trace_forest_moon_visibility(double x, double y)
{
    const double moon_x = 0.88;
    const double moon_y = -0.48;
    double visibility = 1.0;

    for (int i = 1; i <= 9; ++i) {
        double sx = x - moon_x * i * 0.72;
        double sy = y - moon_y * i * 0.72;
        if (map_at((int)sx, (int)sy) > 0) {
            visibility = 0.20 + i * 0.055;
            break;
        }
    }

    return clamp01(visibility);
}

static void build_moon_visibility_cache(void)
{
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            moon_visibility_cache[y][x] = trace_forest_moon_visibility(x + 0.5, y + 0.5);
        }
    }
    moon_visibility_cache_ready = 1;
}

static double forest_moon_visibility_at(double x, double y)
{
    if (!moon_visibility_cache_ready) {
        build_moon_visibility_cache();
    }
    int ix = (int)x;
    int iy = (int)y;
    if (ix < 0 || ix >= MAP_W || iy < 0 || iy >= MAP_H) {
        return 1.0;
    }
    return moon_visibility_cache[iy][ix];
}

static void wall_bump_light_vector(double hit_x, double hit_y, double *light_x, double *light_y)
{
    double lx = 0.0;
    double ly = 0.0;

    for (int i = 0; i < MAX_TORCHES; ++i) {
        double dx = torches[i].pos.x - hit_x;
        double dyw = torches[i].pos.y - hit_y;
        double dist2 = dx * dx + dyw * dyw;
        if (dist2 < 12.0) {
            double weight = 1.0 / (1.0 + dist2);
            lx += dx * weight;
            ly += dyw * weight;
        }
    }

    *light_x = lx;
    *light_y = ly;
}

static double wall_bump_light(int tex_idx, int tex_x, int tex_y, double light_x, double light_y)
{
    int idx = tex_y * TEX_SIZE + tex_x;
    double light_dir = texture_grad_x[tex_idx][idx] * light_x + texture_grad_y[tex_idx][idx] * light_y;
    return clamp01(0.5 + light_dir * 2.8);
}

static uint32_t star_hash(int x, int y)
{
    uint32_t v = (uint32_t)x * 747796405u ^ (uint32_t)y * 2891336453u ^ 0x9E3779B9u;
    v ^= v >> 16;
    v *= 2246822519u;
    v ^= v >> 13;
    v *= 3266489917u;
    v ^= v >> 16;
    return v;
}

static void render_forest_sky(const Camera *cam, const GameState *game)
{
    enum { STAR_COLUMNS = 4096 };
    int horizon = SCREEN_H / 2;
    int star_x[SCREEN_W];
    const double star_scale = STAR_COLUMNS / (2.0 * M_PI);

    for (int x = 0; x < SCREEN_W; ++x) {
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;
        double angle = atan2(ray_dir_y, ray_dir_x);
        if (angle < 0.0) {
            angle += 2.0 * M_PI;
        }
        star_x[x] = ((int)(angle * star_scale)) & (STAR_COLUMNS - 1);
    }

    for (int y = 0; y <= horizon; ++y) {
        double t = y / (double)(horizon > 0 ? horizon : 1);
        uint32_t top = rgb(3, 9, 18);
        uint32_t mid = rgb(11, 26, 32);
        uint32_t haze = rgb(30, 48, 46);
        uint32_t sky = mix_color(mix_color(top, mid, t), haze, smooth01((t - 0.42) / 0.58) * 0.45);

        for (int x = 0; x < SCREEN_W; ++x) {
            uint32_t color = sky;
            if (y < horizon - 10) {
                uint32_t h = star_hash(star_x[x], y);
                if ((h & 0x7FFu) > 2041u) {
                    int sparkle = 165 + (int)((h >> 10) & 63u);
                    double fade = 1.0 - smooth01((y - (horizon * 0.50)) / (horizon * 0.45));
                    uint32_t star = rgb(clamp_u8(sparkle), clamp_u8(sparkle), clamp_u8(sparkle + 12));
                    color = mix_color(color, star, clamp01(0.55 + fade * 0.35));
                }
            }
            framebuffer[y * SCREEN_W + x] = color;
            depth_buffer[y * SCREEN_W + x] = 80.0;
        }
    }

    (void)game;
}

static void render_floor_ceiling(const Camera *cam, const GameState *game)
{
    int fast_render = render_quality == RENDER_QUALITY_FAST;
    int forest_mode = game->generator_mode == GENERATOR_FOREST;
    const int floor_tex = game->generator_mode == GENERATOR_FOREST ? 5 : 6;
    const int ceil_tex = game->generator_mode == GENERATOR_FOREST ? 4 : 3;
    double ray_dir_x0 = cam->dir.x - cam->plane.x;
    double ray_dir_y0 = cam->dir.y - cam->plane.y;
    double ray_dir_x1 = cam->dir.x + cam->plane.x;
    double ray_dir_y1 = cam->dir.y + cam->plane.y;

    if (forest_mode) {
        render_forest_sky(cam, game);
    }

    for (int y = SCREEN_H / 2; y < SCREEN_H; ++y) {
        int p = y - SCREEN_H / 2;
        if (p == 0) {
            for (int x = 0; x < SCREEN_W; ++x) {
                uint32_t horizon_floor = game->generator_mode == GENERATOR_FOREST ? rgb(30, 40, 42) : rgb(18, 17, 16);
                uint32_t horizon_ceil = game->generator_mode == GENERATOR_FOREST ? rgb(16, 24, 34) : rgb(9, 9, 10);
                framebuffer[y * SCREEN_W + x] = apply_game_fog(game, horizon_floor, 9.0, 1.05);
                if (!forest_mode) {
                    framebuffer[(SCREEN_H - y - 1) * SCREEN_W + x] = apply_game_fog(game, horizon_ceil, 9.0, 1.10);
                }
            }
            continue;
        }

        double pos_z = 0.5 * SCREEN_H;
        double row_distance = pos_z / p;
        double floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / SCREEN_W;
        double floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / SCREEN_W;
        double floor_x = cam->pos.x + row_distance * ray_dir_x0;
        double floor_y = cam->pos.y + row_distance * ray_dir_y0;
        double light = game->generator_mode == GENERATOR_FOREST
            ? 0.125 + 0.42 / (1.0 + row_distance * 0.14)
            : 0.045 + 0.30 / (1.0 + row_distance * 0.17);
        double view_facing = fast_render ? 1.0 : clamp01(0.18 + 6.0 / (row_distance + 6.0));
        double fresnel = fast_render ? 0.0 : pow5(1.0 - view_facing);
        double floor_base = light * (game->generator_mode == GENERATOR_FOREST ? 0.86 : 0.48);
        double ceil_base = light * (game->generator_mode == GENERATOR_FOREST ? 0.44 : 0.18);
        uint32_t floor_tint = game->generator_mode == GENERATOR_FOREST ? rgb(132, 154, 142) : rgb(255, 132, 48);
        uint32_t torch_tint = rgb(255, 132, 48);
        int light_stride = (SCREEN_W >= 640 || SCREEN_H >= 400) ? (fast_render ? 4 : 2) : 1;

        for (int x = 0; x < SCREEN_W; x += light_stride) {
            int block_w = light_stride;
            if (x + block_w > SCREEN_W) {
                block_w = SCREEN_W - x;
            }
            double sample_offset = (block_w - 1) * 0.5;
            double sample_floor_x = floor_x + floor_step_x * sample_offset;
            double sample_floor_y = floor_y + floor_step_y * sample_offset;
            double player_light = player_torch_light_at(cam, game, sample_floor_x, sample_floor_y);
            double torch_light = clamp01(torch_light_at(sample_floor_x, sample_floor_y, game->time) + player_light);
            double moon_light = game->generator_mode == GENERATOR_FOREST ? forest_moon_visibility_at(sample_floor_x, sample_floor_y) : 0.0;

            for (int bx = 0; bx < block_w; ++bx) {
            int sx = x + bx;
            int cell_x = (int)floor(floor_x);
            int cell_y = (int)floor(floor_y);
            int tx = (int)(TEX_SIZE * (floor_x - cell_x)) & (TEX_SIZE - 1);
            int ty = (int)(TEX_SIZE * (floor_y - cell_y)) & (TEX_SIZE - 1);
            int texel_idx = ty * TEX_SIZE + tx;

            uint32_t floor_color = textures[floor_tex][texel_idx];
            uint32_t ceil_color = forest_mode ? 0 : textures[ceil_tex][texel_idx];
            uint32_t lit_floor;
            uint32_t lit_ceil = 0;
            if (fast_render) {
                lit_floor = apply_fast_material(
                    floor_color,
                    floor_base + torch_light * 0.70 + moon_light * 0.08,
                    torch_light * 0.76 + moon_light * 0.16);
                if (!forest_mode) {
                    lit_ceil = apply_fast_material(
                        ceil_color,
                        ceil_base + torch_light * 0.26 + moon_light * 0.05,
                        torch_light * 0.26 + moon_light * 0.08);
                }
            } else {
                double floor_bump = texture_bump_light(floor_tex, tx, ty);
                lit_floor = apply_pbr_material(
                    floor_color,
                    &texture_materials[floor_tex],
                    floor_base + torch_light * 0.70 + moon_light * 0.08,
                    torch_light * 0.76 + moon_light * 0.16,
                    floor_bump,
                    fresnel,
                    texture_bright[floor_tex][texel_idx],
                    game->generator_mode == GENERATOR_FOREST ? 0.10 : 0.02,
                    floor_tint);
                if (!forest_mode) {
                    double ceil_bump = texture_bump_light(ceil_tex, tx, ty);
                    lit_ceil = apply_pbr_material(
                        ceil_color,
                        &texture_materials[ceil_tex],
                        ceil_base + torch_light * 0.26 + moon_light * 0.05,
                        torch_light * 0.26 + moon_light * 0.08,
                        ceil_bump,
                        fresnel,
                        texture_bright[ceil_tex][texel_idx],
                        0.0,
                        floor_tint);
                }
            }
            lit_floor = mix_color(lit_floor, torch_tint, clamp01(torch_light * 0.10));
            if (!forest_mode) {
                lit_ceil = mix_color(lit_ceil, torch_tint, clamp01(torch_light * 0.04));
            }
            if (game->generator_mode == GENERATOR_FOREST) {
                lit_floor = mix_color(lit_floor, rgb(118, 150, 136), moon_light * 0.18);
            }
            framebuffer[y * SCREEN_W + sx] = apply_game_fog(game, lit_floor, row_distance, game->generator_mode == GENERATOR_FOREST ? 1.24 : 0.86);
            depth_buffer[y * SCREEN_W + sx] = row_distance;
            add_light(sx, y, torch_light * 0.18 + player_light * 0.10);
            if (!forest_mode) {
                framebuffer[(SCREEN_H - y - 1) * SCREEN_W + sx] = apply_game_fog(game, lit_ceil, row_distance, 0.76);
                depth_buffer[(SCREEN_H - y - 1) * SCREEN_W + sx] = row_distance;
                add_light(sx, SCREEN_H - y - 1, torch_light * 0.08 + player_light * 0.04);
            }
            if (game->generator_mode == GENERATOR_FOREST) {
                add_light(sx, y, moon_light * 0.05);
            }

            floor_x += floor_step_x;
            floor_y += floor_step_y;
            }
        }
    }
}

static void render_forest_moon(const Camera *cam, const GameState *game)
{
    if (game->generator_mode != GENERATOR_FOREST) {
        return;
    }

    Vec2 moon_dir = {0.878, -0.479};
    double forward = cam->dir.x * moon_dir.x + cam->dir.y * moon_dir.y;
    if (forward <= 0.08) {
        return;
    }

    double side = (cam->plane.x * moon_dir.x + cam->plane.y * moon_dir.y) / 0.66;
    int cx = SCREEN_W / 2 + (int)((side / forward) * (SCREEN_W * 0.50));
    int cy = 27 - (int)(forward * 8.0);
    int radius = 11 + (int)(forward * 3.0);

    for (int y = cy - radius * 3; y <= cy + radius * 3; ++y) {
        if (y < 0 || y >= SCREEN_H / 2) {
            continue;
        }
        for (int x = cx - radius * 3; x <= cx + radius * 3; ++x) {
            if (x < 0 || x >= SCREEN_W) {
                continue;
            }
            double dx = (x - cx) / (double)radius;
            double dy = (y - cy) / (double)radius;
            double d = sqrt(dx * dx + dy * dy);
            int idx = y * SCREEN_W + x;
            if (d <= 1.0) {
                uint32_t moon = rgb(198, 218, 218);
                if ((x + y * 3) % 17 < 4) {
                    moon = rgb(154, 176, 184);
                }
                framebuffer[idx] = mix_color(framebuffer[idx], moon, 0.88);
                glow_buffer[idx] = 1.0;
                light_buffer[idx] = 1.0;
            } else if (d <= 3.0) {
                double a = (1.0 - (d - 1.0) / 2.0) * 0.30;
                framebuffer[idx] = mix_color(framebuffer[idx], rgb(110, 144, 168), a);
                add_glow(x, y, a * 0.55);
            }
        }
    }

    for (int ray = -3; ray <= 3; ++ray) {
        int sx = cx + ray * 9;
        for (int y = cy + radius; y < SCREEN_H / 2 + 22; ++y) {
            int x = sx + (int)(sin(y * 0.035 + ray) * 5.0);
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H - 14) {
                double a = 0.055 * (1.0 - (y - cy) / (double)(SCREEN_H / 2 + 24));
                framebuffer[y * SCREEN_W + x] = mix_color(framebuffer[y * SCREEN_W + x], rgb(92, 126, 154), clamp01(a));
                add_glow(x, y, a);
            }
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

static int monster_anim_frame_for_monster(const Monster *monster)
{
    if (monster->attack_anim_timer > 0.0) {
        if (monster->attack_anim_timer > 0.24) {
            return 1;
        }
        if (monster->attack_anim_timer > 0.10) {
            return 2;
        }
        return 3;
    }
    if (monster->pain_timer > 0.0) {
        return 3;
    }
    if (monster->ai_state == 2) {
        return ((int)floor((active_game ? active_game->time : 0.0) * 2.8 + monster->route * 0.35) & 1) ? 1 : 0;
    }
    return ((int)floor((active_game ? active_game->time : 0.0) * 1.2 + monster->route * 0.25) & 1) ? 3 : 0;
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

static void grounded_sprite_bounds(int sprite_h, double scale, int *raw_start_y, int *raw_end_y)
{
    int base_h = (int)(sprite_h / scale);
    if (base_h < 1) {
        base_h = 1;
    }
    *raw_end_y = SCREEN_H / 2 + base_h / 2;
    *raw_start_y = *raw_end_y - sprite_h;
}

static void render_monster(const Camera *cam, const Monster *monster)
{
    if (!monster->active) {
        return;
    }

    int sprite_screen_x;
    int sprite_h;
    double depth;
    double monster_scale = monster->is_boss ? 2.20 :
        (monster->type == MONSTER_GIANT_SKELETON ? 1.50 :
        (monster->type == MONSTER_FLYING_HEAD ? 1.42 : 1.0));
    if (!project_sprite(cam, monster->pos, monster_scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;

    int hover = monster->type == MONSTER_FLYING_HEAD
        ? (int)(sprite_h * 0.18 + sin((active_game ? active_game->time : 0.0) * 4.8 + monster->route) * 3.0)
        : 0;
    int raw_start_y = -sprite_h / 2 + SCREEN_H / 2 - hover;
    int raw_end_y = sprite_h / 2 + SCREEN_H / 2 - hover;
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    int frame = monster_frame_for_camera(cam, monster);
    int anim_frame = monster_anim_frame_for_monster(monster);
    int type = monster->type % MONSTER_TYPES;
    if (type < 0) type = 0;
    int texture_size = monster->is_boss ? BOSS_SPRITE_SIZE :
        (monster->type == MONSTER_GIANT_SKELETON ? GIANT_SKELETON_SPRITE_SIZE : SPRITE_SIZE);
    double light = (monster->is_boss ? 1.22 : 1.0) / (1.0 + depth * 0.055);

    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * texture_size / sprite_w);
        if (tex_x < 0 || tex_x >= texture_size) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - raw_start_y) * texture_size;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= texture_size) {
                continue;
            }

            uint32_t color = monster->is_boss
                ? boss_sprites[frame][anim_frame][tex_y * BOSS_SPRITE_SIZE + tex_x]
                : (monster->type == MONSTER_GIANT_SKELETON
                    ? giant_skeleton_sprites[frame][anim_frame][tex_y * GIANT_SKELETON_SPRITE_SIZE + tex_x]
                    : monster_sprites[type][frame][anim_frame][tex_y * SPRITE_SIZE + tex_x]);
            if (!is_sprite_key(color)) {
                uint32_t lit = shade(color, light);
                if (monster->hit_rim_timer > 0.0) {
                    double rim_t = clamp01(monster->hit_rim_timer / HIT_RIM_TIME);
                    double u = tex_x / (double)(texture_size - 1);
                    double v = tex_y / (double)(texture_size - 1);
                    double edge = fmin(fmin(u, 1.0 - u), fmin(v, 1.0 - v));
                    double rim = 1.0 - clamp01(edge * 8.0);
                    double amount = rim_t * (0.18 + rim * 0.55);
                    lit = mix_color(lit, rgb(255, 250, 230), amount);
                    add_glow(stripe, y, rim_t * (0.10 + rim * 0.24));
                    add_light(stripe, y, rim_t * rim * 0.16);
                }
                if (monster->attack_windup_timer > 0.0) {
                    double windup_time = monster->is_boss ? BOSS_WINDUP_TIME : MONSTER_WINDUP_TIME;
                    double wind_t = clamp01(monster->attack_windup_timer / windup_time);
                    double pulse = 0.45 + 0.55 * sin((1.0 - wind_t) * M_PI * 5.0);
                    double amount = clamp01(0.20 + pulse * 0.36);
                    lit = mix_color(lit, monster->is_boss ? rgb(255, 82, 44) : rgb(255, 192, 112), amount);
                    add_glow(stripe, y, amount * (monster->is_boss ? 0.34 : 0.22));
                    add_light(stripe, y, amount * 0.10);
                }
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
    uint32_t color;
    if (projectile->type == PROJECTILE_EXPLOSION) {
        color = item_sprites[ITEM_SPRITE_EXPLOSION][tex_y * PROJECTILE_SIZE + tex_x];
        return is_sprite_key(color) ? 0 : color;
    }

    if (projectile->type == PROJECTILE_PLAYER_FIREBALL) {
        color = item_sprites[ITEM_SPRITE_FIREBALL][tex_y * PROJECTILE_SIZE + tex_x];
        return is_sprite_key(color) ? 0 : color;
    }

    color = item_sprites[ITEM_SPRITE_BOLT][tex_y * PROJECTILE_SIZE + tex_x];
    return is_sprite_key(color) ? 0 : color;
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
    int raw_start_y;
    int raw_end_y;
    grounded_sprite_bounds(sprite_h, scale, &raw_start_y, &raw_end_y);
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
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
            int d = (y - raw_start_y) * PROJECTILE_SIZE;
            int tex_y = d / sprite_h;
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

static int item_sprite_for_type(const Item *item)
{
    switch (item->type) {
    case ITEM_HEALTH: return ITEM_SPRITE_HEALTH;
    case ITEM_AMMO: return ITEM_SPRITE_AMMO;
    case ITEM_RAPID: return ITEM_SPRITE_RAPID;
    case ITEM_DAMAGE: return ITEM_SPRITE_DAMAGE;
    case ITEM_FIREBALL: return ITEM_SPRITE_FIREBALL;
    case ITEM_PISTOL: return ITEM_SPRITE_PISTOL;
    case ITEM_KEY: return ITEM_SPRITE_KEY;
    case ITEM_GOLD: return ITEM_SPRITE_GOLD;
    case ITEM_SHRINE: return ITEM_SPRITE_SHRINE;
    case ITEM_BONEPILE: return ITEM_SPRITE_BONEPILE;
    default: return ITEM_SPRITE_KEY;
    }
}

static uint32_t item_texel(const Item *item, int tex_x, int tex_y)
{
    if (item->type == ITEM_RELIC) {
        int relic = item->relic_index;
        if (relic < 0 || relic >= RELIC_COUNT) {
            relic = 0;
        }
        uint32_t color = relic_sprites[relic][tex_y * PROJECTILE_SIZE + tex_x];
        return is_sprite_key(color) ? 0 : color;
    }

    uint32_t color = item_sprites[item_sprite_for_type(item)][tex_y * PROJECTILE_SIZE + tex_x];
    return is_sprite_key(color) ? 0 : color;
}

static void render_item(const Camera *cam, const Item *item)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    double scale = item->type == ITEM_RELIC ? 0.54 :
        (item->type == ITEM_SHRINE ? 0.62 :
         (item->type == ITEM_BONEPILE ? 0.48 :
          (item->type == ITEM_GOLD ? 0.34 : 0.42)));
    if (!project_sprite(cam, item->pos, scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;
    int raw_start_y;
    int raw_end_y;
    grounded_sprite_bounds(sprite_h, scale, &raw_start_y, &raw_end_y);
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = (item->type == ITEM_RELIC || item->type == ITEM_SHRINE ? 1.55 :
        (item->type == ITEM_GOLD ? 1.35 : 1.2)) / (1.0 + depth * 0.045);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - raw_start_y) * PROJECTILE_SIZE;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }

            uint32_t color = item_texel(item, tex_x, tex_y);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = apply_fog(shade(color, light), depth, 0.72);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                if (item->type == ITEM_FIREBALL || item->type == ITEM_RAPID || item->type == ITEM_DAMAGE || item->type == ITEM_RELIC || item->type == ITEM_SHRINE || item->type == ITEM_GOLD) {
                    add_glow(stripe, y, item->type == ITEM_RELIC ? 0.74 :
                             (item->type == ITEM_SHRINE ? 0.30 :
                              (item->type == ITEM_GOLD ? 0.16 :
                               (item->type == ITEM_FIREBALL ? 0.22 : 0.12))));
                    if (item->type == ITEM_RELIC || item->type == ITEM_SHRINE) {
                        add_light(stripe, y, 0.36);
                    }
                }
            }
        }
    }
}

static uint32_t prop_texel(const Prop *prop, double u, double v)
{
    int sprite = prop->type;
    if (sprite < 0 || sprite >= FURNITURE_SPRITE_COUNT) {
        sprite = PROP_CRATE;
    }
    int tex_x = (int)(clamp01(u) * (FURNITURE_SIZE - 1));
    int tex_y = (int)(clamp01(v) * (FURNITURE_SIZE - 1));
    return furniture_sprites[sprite][tex_y * FURNITURE_SIZE + tex_x];
}

static uint32_t portal_texel(const Portal *portal, int tex_x, int tex_y)
{
    int boss_open = portal->boss_gate && active_game && active_game->boss_unlocked;
    int sprite = ITEM_SPRITE_PORTAL_DUNGEON;
    if (portal->boss_gate) {
        sprite = boss_open ? ITEM_SPRITE_PORTAL_BOSS_OPEN : ITEM_SPRITE_PORTAL_BOSS_LOCKED;
    } else if (portal->exit_to_forest) {
        sprite = ITEM_SPRITE_PORTAL_FOREST;
    }
    uint32_t color = item_sprites[sprite][tex_y * PROJECTILE_SIZE + tex_x];
    return is_sprite_key(color) ? 0 : color;
}

static void render_portal(const Camera *cam, const Portal *portal)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    Vec2 pos = {portal->x + 0.5, portal->y + 0.5};
    double scale = 0.70;
    if (!project_sprite(cam, pos, scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;
    int raw_start_y;
    int raw_end_y;
    grounded_sprite_bounds(sprite_h, scale, &raw_start_y, &raw_end_y);
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = portal->boss_gate && active_game && active_game->boss_unlocked ? 1.28 : (portal->exit_to_forest ? 1.22 : 0.96);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }
        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }
        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - raw_start_y) * PROJECTILE_SIZE;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }
            uint32_t color = portal_texel(portal, tex_x, tex_y);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = apply_fog(shade(color, light), depth, 0.56);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                add_glow(stripe, y, portal->boss_gate ? (active_game && active_game->boss_unlocked ? 0.18 : 0.08) : (portal->exit_to_forest ? 0.12 : 0.06));
            }
        }
    }
}

static void render_tree(const Camera *cam, const GameState *game, const Tree *tree)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    double tree_scale = 1.70 + (tree->variant % TREE_TYPES) * 0.08;
    if (!project_sprite(cam, tree->pos, tree_scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }
    if (depth < TREE_RENDER_NEAR_CLIP) {
        return;
    }

    int sprite_w = sprite_h;
    int base_h = (int)(sprite_h / tree_scale);
    if (base_h < 1) {
        base_h = 1;
    }
    int raw_end_y = SCREEN_H / 2 + base_h / 2;
    int raw_start_y = raw_end_y - sprite_h;
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = 1.20 / (1.0 + depth * 0.035);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe] + 0.35) {
            continue;
        }
        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * SPRITE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= SPRITE_SIZE) {
            continue;
        }
        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - raw_start_y) * SPRITE_SIZE;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= SPRITE_SIZE) {
                continue;
            }
            uint32_t color = tree_sprites[tree->variant % TREE_TYPES][tex_y * SPRITE_SIZE + tex_x];
            if (!is_sprite_key(color)) {
                framebuffer[y * SCREEN_W + stripe] = apply_game_fog(game, shade(color, light), depth, 0.86);
                depth_buffer[y * SCREEN_W + stripe] = depth;
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

static uint32_t campfire_texel(int tex_x, int tex_y, double time, int index)
{
    double flicker = sin(time * 18.0 + index * 0.83) * 1.4 + sin(time * 31.0 + index * 1.21) * 0.8;
    double cx = PROJECTILE_SIZE / 2.0 + flicker;
    double base_y = PROJECTILE_SIZE * 0.72;

    if (tex_y >= 22 && tex_y <= 28) {
        if ((tex_x >= 8 && tex_x <= 23 && ((tex_x + tex_y) & 3) != 0) ||
            (tex_x >= 11 && tex_x <= 26 && ((tex_x - tex_y) & 3) == 0)) {
            return rgb(62, 40, 26);
        }
    }
    if (tex_y >= 26 && tex_y <= 31 && tex_x >= 6 && tex_x <= 26) {
        return ((tex_x + tex_y + index) & 3) ? rgb(42, 38, 34) : rgb(82, 58, 34);
    }

    double flame_x = fabs(tex_x - cx);
    double flame_y = base_y - tex_y;
    if (flame_y >= 0.0 && flame_y <= 21.0) {
        double width = 8.0 - flame_y * 0.24 + sin(tex_y * 0.55 + time * 14.0) * 0.9;
        double shape = flame_x / width + flame_y / 23.0;
        if (shape < 0.92) {
            if (shape < 0.36) {
                return rgb(255, 240, 128);
            }
            if (((tex_x + tex_y + index) & 2) == 0) {
                return rgb(255, 162, 42);
            }
            return rgb(194, 58, 22);
        }
    }

    return 0;
}

static void render_torch(const Camera *cam, const Torch *torch, int index, double time)
{
    int sprite_screen_x;
    int sprite_h;
    double depth;
    int forest_mode = active_game && active_game->generator_mode == GENERATOR_FOREST;
    double scale = forest_mode ? 0.52 : 0.50;
    if (!project_sprite(cam, torch->pos, scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = forest_mode ? sprite_h : sprite_h * 2 / 3;
    if (sprite_w < 1) {
        return;
    }

    int raw_start_y;
    int raw_end_y;
    grounded_sprite_bounds(sprite_h, scale, &raw_start_y, &raw_end_y);
    int draw_start_y = raw_start_y;
    int draw_end_y = raw_end_y;
    int draw_start_x = -sprite_w / 2 + sprite_screen_x;
    int draw_end_x = sprite_w / 2 + sprite_screen_x;

    if (draw_start_y < 0) draw_start_y = 0;
    if (draw_end_y >= SCREEN_H) draw_end_y = SCREEN_H - 1;
    if (draw_start_x < 0) draw_start_x = 0;
    if (draw_end_x >= SCREEN_W) draw_end_x = SCREEN_W - 1;

    double light = (forest_mode ? 1.55 : 1.90) / (1.0 + depth * 0.020);
    for (int stripe = draw_start_x; stripe <= draw_end_x; ++stripe) {
        if (depth >= z_buffer[stripe]) {
            continue;
        }

        int tex_x = (int)((stripe - (-sprite_w / 2.0 + sprite_screen_x)) * PROJECTILE_SIZE / sprite_w);
        if (tex_x < 0 || tex_x >= PROJECTILE_SIZE) {
            continue;
        }

        for (int y = draw_start_y; y <= draw_end_y; ++y) {
            int d = (y - raw_start_y) * PROJECTILE_SIZE;
            int tex_y = d / sprite_h;
            if (tex_y < 0 || tex_y >= PROJECTILE_SIZE) {
                continue;
            }

            double gx = (tex_x - PROJECTILE_SIZE / 2.0) / 15.0;
            double gy = (tex_y - 12.0) / 18.0;
            double glow = clamp01(1.0 - sqrt(gx * gx + gy * gy));
            if (glow > 0.0) {
                double amount = glow * (forest_mode ? 0.24 : 0.46) / (1.0 + depth * 0.035);
                uint32_t current = framebuffer[y * SCREEN_W + stripe];
                framebuffer[y * SCREEN_W + stripe] = mix_color(current, rgb(255, 146, 42), clamp01(amount));
                add_glow(stripe, y, amount * 0.75);
                add_light(stripe, y, amount * 0.28);
            }

            uint32_t color = forest_mode ? campfire_texel(tex_x, tex_y, time, index) : torch_texel(tex_x, tex_y, time, index);
            if (color != 0) {
                framebuffer[y * SCREEN_W + stripe] = apply_game_fog(active_game, shade(color, light), depth, forest_mode ? 0.34 : 0.20);
                depth_buffer[y * SCREEN_W + stripe] = depth;
                add_glow(stripe, y, forest_mode ? 0.26 : 0.42);
            }
        }
    }
}

static void render_world_sprites(const Camera *cam, const GameState *game)
{
    SpriteDraw draws[MAX_MONSTERS + MAX_PROJECTILES + MAX_ITEMS + MAX_TORCHES + MAX_PARTICLES + MAX_PORTALS + MAX_TREES];
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

    for (int i = 0; i < MAX_PORTALS; ++i) {
        const Portal *portal = &game->portals[i];
        if (!portal->active) {
            continue;
        }
        double dx = portal->x + 0.5 - cam->pos.x;
        double dy = portal->y + 0.5 - cam->pos.y;
        draws[count++] = (SpriteDraw){5, i, dx * dx + dy * dy};
    }

    for (int i = 0; i < MAX_TREES; ++i) {
        const Tree *tree = &game->trees[i];
        if (!tree->active) {
            continue;
        }
        double dx = tree->pos.x - cam->pos.x;
        double dy = tree->pos.y - cam->pos.y;
        draws[count++] = (SpriteDraw){6, i, dx * dx + dy * dy};
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
        } else if (draw.kind == 5) {
            render_portal(cam, &game->portals[draw.index]);
        } else if (draw.kind == 6) {
            render_tree(cam, game, &game->trees[draw.index]);
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
    uint32_t fog = fog_color_for_game(game);
    int fog_stride = (SCREEN_W >= 640 || SCREEN_H >= 400) ? 2 : 1;
    int scatter_steps = fog_stride > 1 ? 2 : 4;

    for (int x = 0; x < SCREEN_W; x += fog_stride) {
        double wall_depth = z_buffer[x];
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;

        for (int y = 0; y < SCREEN_H - 14; y += fog_stride) {
            int idx = y * SCREEN_W + x;
            int from_horizon = abs(y - SCREEN_H / 2);
            double pixel_depth = depth_buffer[idx];
            int forest_mode = game->generator_mode == GENERATOR_FOREST;
            int wall_pixel = forest_mode && pixel_depth > 0.01 && fabs(pixel_depth - wall_depth) < 0.0001;
            if (pixel_depth <= 0.01) {
                pixel_depth = wall_depth;
            }
            if (from_horizon > 0) {
                double row_depth = (0.5 * SCREEN_H) / from_horizon;
                if (row_depth < pixel_depth) {
                    pixel_depth = row_depth;
                }
            }

            double pass_strength = forest_mode ? 0.54 : 0.50;
            double pixel_fog = fog_amount_for_game(game, pixel_depth) * pass_strength;
            if (pixel_fog <= 0.002) {
                continue;
            }

            double dy = fabs((y - SCREEN_H * 0.5) / (SCREEN_H * 0.5));
            double horizon = clamp01(1.0 - dy);
            double lower = y > SCREEN_H / 2 ? (y - SCREEN_H / 2.0) / (SCREEN_H / 2.0) : 0.0;
            double ground = lower * lower;
            double wave_a;
            double wave_b;
            double wave_c;
            if (forest_mode) {
                double sample_depth = pixel_depth;
                if (sample_depth > 8.5) {
                    sample_depth = 8.5;
                }
                double world_x = cam->pos.x + ray_dir_x * sample_depth;
                double world_y = cam->pos.y + ray_dir_y * sample_depth;
                wave_a = sin(world_x * 2.10 + world_y * 0.82 + game->time * 0.26);
                wave_b = sin(world_x * -0.74 + world_y * 2.35 - game->time * 0.18);
                wave_c = sin((world_x + world_y) * 1.48 + game->time * 0.11);
            } else {
                wave_a = sin(x * 0.047 + y * 0.013 + game->time * 0.72);
                wave_b = sin(x * 0.021 - y * 0.096 - game->time * 0.34);
                wave_c = sin(x * 0.111 + y * 0.041 + game->time * 0.19);
            }
            double cloud = clamp01((wave_a + wave_b * 0.85 + wave_c * 0.55 + 1.02) * 0.36);
            cloud = cloud * cloud * (1.45 - cloud * 0.45);
            double wisp = clamp01((cloud - 0.52) / 0.48);
            wisp = wisp * wisp * (3.0 - 2.0 * wisp);
            double forest_extra = forest_mode ? 0.018 + horizon * 0.018 : 0.0;
            double amount = pixel_fog * (0.010 + horizon * 0.018 + ground * 0.090 + forest_extra) * wisp;
            if (forest_mode) {
                double depth_gate = clamp01((pixel_depth - 1.10) / 7.5);
                amount *= depth_gate * (0.72 + ground * 0.38);
                if (wall_pixel) {
                    amount = 0.0;
                }
            }

            if (amount > 0.0015) {
                double mix_amount = clamp01(amount);
                for (int by = 0; by < fog_stride && y + by < SCREEN_H - 14; ++by) {
                    for (int bx = 0; bx < fog_stride && x + bx < SCREEN_W; ++bx) {
                        int block_idx = (y + by) * SCREEN_W + x + bx;
                        framebuffer[block_idx] = mix_color(framebuffer[block_idx], fog, mix_amount);
                    }
                }
            }

            if (pixel_depth > 0.35) {
                double torch_scatter = 0.0;
                for (int sample = 1; sample <= scatter_steps; ++sample) {
                    double t = pixel_depth * (sample / (double)(scatter_steps + 1));
                    Vec2 p = {
                        cam->pos.x + ray_dir_x * t,
                        cam->pos.y + ray_dir_y * t,
                    };
                    torch_scatter += torch_light_at(p.x, p.y, game->time) * (1.0 - sample * 0.10);
                    if (game->generator_mode == GENERATOR_FOREST) {
                        torch_scatter += forest_moon_visibility_at(p.x, p.y) * 0.18 * (1.0 - sample * 0.06);
                    }
                }

                torch_scatter = clamp01(torch_scatter / scatter_steps);
                double warm_amount = torch_scatter * pixel_fog * (0.020 + ground * 0.090 + wisp * 0.050);
                if (forest_mode) {
                    warm_amount *= 0.55;
                    if (wall_pixel) {
                        warm_amount = 0.0;
                    }
                }
                if (warm_amount > 0.003) {
                    double mix_amount = clamp01(warm_amount);
                    uint32_t scatter_color = game->generator_mode == GENERATOR_FOREST ? rgb(104, 132, 112) : rgb(166, 78, 32);
                    for (int by = 0; by < fog_stride && y + by < SCREEN_H - 14; ++by) {
                        for (int bx = 0; bx < fog_stride && x + bx < SCREEN_W; ++bx) {
                            int block_idx = (y + by) * SCREEN_W + x + bx;
                            framebuffer[block_idx] = mix_color(framebuffer[block_idx], scatter_color, mix_amount);
                        }
                    }
                }
            }
        }
    }
}

static int normalize_render_effects(int effects)
{
    if (effects < 0 || effects >= RENDER_EFFECTS_COUNT) {
        return RENDER_EFFECTS_OFF;
    }
    return effects;
}

static const char *render_effects_menu_text(int effects)
{
    switch (normalize_render_effects(effects)) {
    case RENDER_EFFECTS_PRESET1:
        return "POST 1";
    case RENDER_EFFECTS_PRESET2:
        return "POST 2";
    case RENDER_EFFECTS_PRESET3:
        return "POST 3";
    default:
        return "POST OFF";
    }
}

static double bloom_strength_for_effects(int effects)
{
    switch (normalize_render_effects(effects)) {
    case RENDER_EFFECTS_PRESET1:
        return 0.50;
    case RENDER_EFFECTS_PRESET2:
        return 0.34;
    case RENDER_EFFECTS_PRESET3:
        return 0.68;
    default:
        return 0.0;
    }
}

static void render_bloom(int effects)
{
    double strength = bloom_strength_for_effects(effects);
    if (strength <= 0.0) {
        return;
    }

    int limit_y = SCREEN_H - 14;
    for (int y = 0; y < limit_y; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            int idx = y * SCREEN_W + x;
            double bright = smooth01((luminance(framebuffer[idx]) - 0.62) / 0.30) * 0.22;
            glow_blur_buffer[idx] = glow_buffer[idx] + bright;
            bloom_work_buffer[idx] = 0.0;
        }
    }

    for (int y = 1; y < limit_y - 1; ++y) {
        for (int x = 1; x < SCREEN_W - 1; ++x) {
            int idx = y * SCREEN_W + x;
            double s =
                glow_blur_buffer[idx] * 0.34 +
                glow_blur_buffer[idx - 1] * 0.12 +
                glow_blur_buffer[idx + 1] * 0.12 +
                glow_blur_buffer[idx - SCREEN_W] * 0.12 +
                glow_blur_buffer[idx + SCREEN_W] * 0.12 +
                glow_blur_buffer[idx - SCREEN_W - 1] * 0.045 +
                glow_blur_buffer[idx - SCREEN_W + 1] * 0.045 +
                glow_blur_buffer[idx + SCREEN_W - 1] * 0.045 +
                glow_blur_buffer[idx + SCREEN_W + 1] * 0.045;
            bloom_work_buffer[idx] = s;
        }
    }

    for (int y = 3; y < limit_y - 3; ++y) {
        for (int x = 3; x < SCREEN_W - 3; ++x) {
            int idx = y * SCREEN_W + x;
            double s =
                bloom_work_buffer[idx] * 0.36 +
                bloom_work_buffer[idx - 2] * 0.08 +
                bloom_work_buffer[idx + 2] * 0.08 +
                bloom_work_buffer[idx - SCREEN_W * 2] * 0.08 +
                bloom_work_buffer[idx + SCREEN_W * 2] * 0.08 +
                bloom_work_buffer[idx - 1] * 0.08 +
                bloom_work_buffer[idx + 1] * 0.08 +
                bloom_work_buffer[idx - SCREEN_W] * 0.08 +
                bloom_work_buffer[idx + SCREEN_W] * 0.08;
            if (s > 0.004) {
                uint32_t bloom_tint = mix_color(rgb(255, 122, 40), framebuffer[idx], 0.38);
                framebuffer[idx] = add_color(framebuffer[idx], bloom_tint, s * strength);
            }
        }
    }
}

static void render_edge_antialias(void)
{
    int pixels = SCREEN_W * (SCREEN_H - 14);
    for (int i = 0; i < pixels; ++i) {
        post_buffer[i] = framebuffer[i];
        post_luma_buffer[i] = luminance(framebuffer[i]);
    }

    for (int y = 1; y < SCREEN_H - 15; ++y) {
        for (int x = 1; x < SCREEN_W - 1; ++x) {
            int idx = y * SCREEN_W + x;
            uint32_t c = framebuffer[idx];
            double lc = post_luma_buffer[idx];
            double ll = post_luma_buffer[idx - 1];
            double lr = post_luma_buffer[idx + 1];
            double lu = post_luma_buffer[idx - SCREEN_W];
            double ld = post_luma_buffer[idx + SCREEN_W];
            double lmin = lc;
            double lmax = lc;
            if (ll < lmin) lmin = ll;
            if (lr < lmin) lmin = lr;
            if (lu < lmin) lmin = lu;
            if (ld < lmin) lmin = ld;
            if (ll > lmax) lmax = ll;
            if (lr > lmax) lmax = lr;
            if (lu > lmax) lmax = lu;
            if (ld > lmax) lmax = ld;

            double contrast = lmax - lmin;
            if (contrast < 0.11) {
                continue;
            }
            if (lc > 0.84 && contrast > 0.45) {
                continue;
            }

            double horizontal_delta = fabs(ll - lr);
            double vertical_delta = fabs(lu - ld);
            uint32_t target = horizontal_delta < vertical_delta
                ? mix_color(framebuffer[idx - 1], framebuffer[idx + 1], 0.5)
                : mix_color(framebuffer[idx - SCREEN_W], framebuffer[idx + SCREEN_W], 0.5);
            double amount = clamp01((contrast - 0.11) * 1.45);
            if (amount > 0.34) {
                amount = 0.34;
            }
            post_buffer[idx] = mix_color(c, target, amount);
        }
    }

    memcpy(framebuffer, post_buffer, (size_t)pixels * sizeof(framebuffer[0]));
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

static uint8_t lut_channel(double value)
{
    value = clamp01(value);
    return clamp_u8((int)(value * 255.0 + 0.5));
}

static uint32_t apply_color_lut(uint32_t color, int effects)
{
    double r = ((color >> 16) & 0xFFu) / 255.0;
    double g = ((color >> 8) & 0xFFu) / 255.0;
    double b = (color & 0xFFu) / 255.0;
    double l = r * 0.299 + g * 0.587 + b * 0.114;

    switch (normalize_render_effects(effects)) {
    case RENDER_EFFECTS_PRESET1:
        r = pow(clamp01(r * 1.06 + l * 0.025), 0.96);
        g = pow(clamp01(g * 1.02 + l * 0.018), 0.98);
        b = pow(clamp01(b * 0.96 + l * 0.010), 1.03);
        break;
    case RENDER_EFFECTS_PRESET2:
        r = pow(clamp01(r * 0.88 + l * 0.035), 1.08);
        g = pow(clamp01(g * 1.00 + l * 0.025), 1.00);
        b = pow(clamp01(b * 1.14 + l * 0.030), 0.94);
        break;
    case RENDER_EFFECTS_PRESET3:
        r = pow(clamp01(r * 1.18 + l * 0.040), 0.90);
        g = pow(clamp01(g * 0.94 + l * 0.020), 1.02);
        b = pow(clamp01(b * 0.82 + l * 0.010), 1.12);
        break;
    default:
        break;
    }

    return rgb(lut_channel(r), lut_channel(g), lut_channel(b));
}

static void render_muzzle_light(const GameState *game)
{
    if (game->muzzle_light <= 0.0) {
        return;
    }

    double t = clamp01(game->muzzle_light / MUZZLE_LIGHT_TIME);
    int cx = SCREEN_W / 2;
    int cy = SCREEN_H / 2 + 8;
    int mx = SCREEN_W / 2 + 31;
    int my = SCREEN_H - 66;
    int min_x = cx - 220;
    int max_x = cx + 220;
    int min_y = cy - 130;
    int max_y = my + 44;
    if (min_x < 0) min_x = 0;
    if (max_x >= SCREEN_W) max_x = SCREEN_W - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= SCREEN_H - 14) max_y = SCREEN_H - 15;

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double cone_x = fabs((x - cx) / 220.0);
            double cone_y = fabs((y - cy) / 150.0);
            double cone = 1.0 - sqrt(cone_x * cone_x + cone_y * cone_y);
            if (cone <= 0.0) {
                continue;
            }

            double muzzle_x = (x - mx) / 72.0;
            double muzzle_y = (y - my) / 54.0;
            double muzzle = 1.0 - sqrt(muzzle_x * muzzle_x + muzzle_y * muzzle_y);
            if (muzzle < 0.0) {
                muzzle = 0.0;
            }

            int idx = y * SCREEN_W + x;
            double depth = depth_buffer[idx];
            if (depth > 40.0) {
                depth = 8.0;
            }
            double depth_light = clamp01(1.0 - depth / 12.0);
            double amount = t * t * (cone * 0.32 + muzzle * 0.72) * (0.28 + depth_light * 0.72);
            if (amount <= 0.002) {
                continue;
            }
            framebuffer[idx] = mix_color(framebuffer[idx], rgb(255, 154, 58), clamp01(amount * 0.34));
            add_light(x, y, amount);
            add_glow(x, y, amount * 0.24);
        }
    }
}

static void render_forest_weather_overlay(const GameState *game)
{
    if (game->generator_mode != GENERATOR_FOREST) {
        return;
    }

    int tick = (int)(game->time * 60.0);
    int h_limit = SCREEN_H - 32;
    uint32_t rain_haze = rgb(64, 88, 104);
    for (int y = 24; y < h_limit; y += 2) {
        double band = 0.5 + 0.5 * sin(y * 0.045 + tick * 0.055);
        double horizon = 1.0 - clamp01((y - 24) / (double)(h_limit - 24));
        double amount = 0.008 + band * 0.010 + horizon * 0.016;
        for (int x = 0; x < SCREEN_W; x += 2) {
            int idx = y * SCREEN_W + x;
            framebuffer[idx] = mix_color(framebuffer[idx], rain_haze, amount);
            if (x + 1 < SCREEN_W) {
                framebuffer[idx + 1] = mix_color(framebuffer[idx + 1], rain_haze, amount * 0.65);
            }
            if (y + 1 < SCREEN_H - 14) {
                framebuffer[idx + SCREEN_W] = mix_color(framebuffer[idx + SCREEN_W], rain_haze, amount * 0.55);
            }
        }
    }

    uint32_t rain = rgb(130, 166, 190);
    for (int i = 0; i < 86; ++i) {
        uint32_t h = star_hash(i * 47 + tick / 2, i * 31 + 17);
        int x = (int)((h + (uint32_t)(tick * 2)) % SCREEN_W);
        double strength = 0.055 + ((h >> 18) & 31u) / 760.0;
        for (int y = 22; y < h_limit; y += 2) {
            int sway = (int)sin(y * 0.026 + i * 0.71 + tick * 0.030);
            int px = x + sway;
            if (px < 0 || px >= SCREEN_W) {
                continue;
            }
            double horizon = 1.0 - clamp01((y - 22) / (double)(h_limit - 22));
            double wave = 0.55 + 0.45 * sin(y * 0.090 + i * 1.37 + tick * 0.18);
            double amount = strength * (0.50 + horizon * 0.80) * wave;
            int idx = y * SCREEN_W + px;
            framebuffer[idx] = mix_color(framebuffer[idx], rain, amount);
            if (px + 1 < SCREEN_W) {
                framebuffer[idx + 1] = mix_color(framebuffer[idx + 1], rain, amount * 0.35);
            }
        }
    }
}

static void render_color_grade(const GameState *game, int effects)
{
    effects = normalize_render_effects(effects);
    if (effects == RENDER_EFFECTS_OFF) {
        return;
    }

    if (!vignette_ready) {
        for (int y = 0; y < SCREEN_H - 14; ++y) {
            double ny = (y - SCREEN_H * 0.5) / (SCREEN_H * 0.5);
            for (int x = 0; x < SCREEN_W; ++x) {
                double nx = (x - SCREEN_W * 0.5) / (SCREEN_W * 0.5);
                vignette_buffer[y * SCREEN_W + x] = clamp01((nx * nx + ny * ny) * 0.34);
            }
        }
        vignette_ready = 1;
    }

    for (int y = 0; y < SCREEN_H - 14; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            int idx = y * SCREEN_W + x;
            double vignette = vignette_buffer[idx];
            uint32_t c = framebuffer[idx];
            double contrast = 1.12;
            double brightness = -5.0;
            uint32_t vignette_tint = rgb(20, 28, 30);
            double vignette_amount = vignette * 0.45;
            uint32_t wash = rgb(0, 0, 0);
            double wash_amount = 0.0;

            if (effects == RENDER_EFFECTS_PRESET2) {
                contrast = 1.06;
                brightness = -2.0;
                vignette_tint = rgb(14, 24, 38);
                vignette_amount = vignette * 0.38;
                wash = rgb(54, 80, 112);
                wash_amount = 0.045;
            } else if (effects == RENDER_EFFECTS_PRESET3) {
                contrast = 1.18;
                brightness = -7.0;
                vignette_tint = rgb(34, 20, 14);
                vignette_amount = vignette * 0.50;
                wash = rgb(118, 56, 28);
                wash_amount = 0.040;
            } else if (game->generator_mode == GENERATOR_FOREST) {
                contrast = 1.08;
                brightness = 6.0;
                vignette_tint = rgb(22, 34, 48);
                vignette_amount = vignette * 0.30;
                wash = rgb(82, 110, 134);
                wash_amount = 0.055;
            }
            c = contrast_color(c, contrast, brightness);
            c = mix_color(c, vignette_tint, vignette_amount);
            if (wash_amount > 0.0) {
                c = mix_color(c, wash, wash_amount);
            }
            c = apply_color_lut(c, effects);
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

static void render_player_damage_feedback(const Camera *cam, const GameState *game)
{
    if (game->player_damage_flash <= 0.0) {
        return;
    }

    double t = clamp01(game->player_damage_flash / PLAYER_DAMAGE_FLASH_TIME);
    blend_rect(0, 0, SCREEN_W, SCREEN_H - 14, rgb(160, 8, 8), t * 0.18);

    Vec2 damage_dir = {game->damage_dir_x, game->damage_dir_y};
    if (vec_len(damage_dir) <= 0.001) {
        return;
    }

    Vec2 right = {-cam->dir.y, cam->dir.x};
    double side = vec_dot(damage_dir, right);
    double front = vec_dot(damage_dir, cam->dir);
    int cx = SCREEN_W / 2 + (int)(side * 154.0);
    int cy = SCREEN_H / 2 - 8 - (int)(front * 96.0);
    if (cx < 28) cx = 28;
    if (cx > SCREEN_W - 28) cx = SCREEN_W - 28;
    if (cy < 34) cy = 34;
    if (cy > SCREEN_H - 70) cy = SCREEN_H - 70;

    uint32_t hot = mix_color(rgb(180, 18, 14), rgb(255, 180, 122), t);
    int size = 8 + (int)(t * 8.0);
    blend_rect(cx - size, cy - size, size * 2, size * 2, rgb(120, 0, 0), t * 0.18);
    fill_rect(cx - size / 2, cy - 2, size, 4, hot);
    fill_rect(cx - 2, cy - size / 2, 4, size, hot);
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

static void render_crosshair(const GameState *game)
{
    uint32_t c = rgb(232, 226, 196);
    fill_rect(SCREEN_W / 2 - 5, SCREEN_H / 2, 4, 1, c);
    fill_rect(SCREEN_W / 2 + 2, SCREEN_H / 2, 4, 1, c);
    fill_rect(SCREEN_W / 2, SCREEN_H / 2 - 5, 1, 4, c);
    fill_rect(SCREEN_W / 2, SCREEN_H / 2 + 2, 1, 4, c);
    if (game->hit_marker > 0.0) {
        double t = clamp01(game->hit_marker / HIT_MARKER_TIME);
        uint32_t marker = mix_color(rgb(255, 66, 44), rgb(255, 252, 230), t);
        int gap = 8 + (int)((1.0 - t) * 3.0);
        draw_line(SCREEN_W / 2 - gap - 3, SCREEN_H / 2 - gap - 3,
                  SCREEN_W / 2 - gap, SCREEN_H / 2 - gap, marker);
        draw_line(SCREEN_W / 2 + gap + 3, SCREEN_H / 2 - gap - 3,
                  SCREEN_W / 2 + gap, SCREEN_H / 2 - gap, marker);
        draw_line(SCREEN_W / 2 - gap - 3, SCREEN_H / 2 + gap + 3,
                  SCREEN_W / 2 - gap, SCREEN_H / 2 + gap, marker);
        draw_line(SCREEN_W / 2 + gap + 3, SCREEN_H / 2 + gap + 3,
                  SCREEN_W / 2 + gap, SCREEN_H / 2 + gap, marker);
    }
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
    if (game->shot_trace <= 0.0 ||
        (game->selected_weapon != WEAPON_PISTOL && game->selected_weapon != WEAPON_SHOTGUN)) {
        return;
    }

    double progress = 1.0 - clamp01(game->shot_trace / WEAPON_FLASH_TIME);
    int muzzle_x = SCREEN_W / 2 + 31;
    int muzzle_y = SCREEN_H - 66;
    int pellet_count = game->selected_weapon == WEAPON_SHOTGUN ? 3 : 1;
    uint32_t core = mix_color(rgb(255, 132, 34), rgb(255, 244, 154), 0.70 + 0.30 * (1.0 - progress));

    for (int pellet = 0; pellet < pellet_count; ++pellet) {
        int spread = pellet_count == 1 ? 0 : (pellet - 1) * 28;
        int target_x = SCREEN_W / 2 + spread;
        int target_y = SCREEN_H / 2 + 2 + abs(spread) / 5;
        int bullet_x = muzzle_x + (int)((target_x - muzzle_x) * progress);
        int bullet_y = muzzle_y + (int)((target_y - muzzle_y) * progress);
        int tail_x = bullet_x + (int)((muzzle_x - target_x) * 0.08);
        int tail_y = bullet_y + (int)((muzzle_y - target_y) * 0.08);
        draw_line(tail_x, tail_y, bullet_x, bullet_y, rgb(198, 88, 28));
        draw_line(tail_x + 1, tail_y, bullet_x + 1, bullet_y, core);
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                int px = bullet_x + x;
                int py = bullet_y + y;
                if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                    framebuffer[py * SCREEN_W + px] = x == 0 && y == 0 ? rgb(255, 250, 194) : core;
                    add_glow(px, py, 0.28);
                }
            }
        }
    }
}

static void draw_weapon_sprite(int sprite, int x, int y, int size)
{
    if (sprite < 0 || sprite >= WEAPON_SPRITE_COUNT || size <= 0) {
        return;
    }

    for (int yy = 0; yy < size; ++yy) {
        int sy = yy * WEAPON_SPRITE_SIZE / size;
        int dst_y = y + yy;
        if (dst_y < 0 || dst_y >= SCREEN_H) {
            continue;
        }
        for (int xx = 0; xx < size; ++xx) {
            int sx = xx * WEAPON_SPRITE_SIZE / size;
            int dst_x = x + xx;
            if (dst_x < 0 || dst_x >= SCREEN_W) {
                continue;
            }
            uint32_t color = weapon_sprites[sprite][sy * WEAPON_SPRITE_SIZE + sx];
            if (!is_sprite_key(color)) {
                framebuffer[dst_y * SCREEN_W + dst_x] = color;
            }
        }
    }
}

static void render_weapon(const GameState *game)
{
    int bob = (int)(sin(game->time * 7.5) * 2.0);
    double flash_time = game->selected_weapon == WEAPON_FIREBALL ? FIREBALL_FLASH_TIME : WEAPON_FLASH_TIME;
    double fire_t = game->weapon_flash > 0.0 ? game->weapon_flash / flash_time : 0.0;
    int recoil_y = (int)(fire_t * fire_t * 15.0);
    int recoil_x = (int)(sin(game->time * 120.0) * fire_t * 4.0);
    int sprite = WEAPON_SPRITE_PISTOL;
    int size = 86;
    int offset_x = 0;
    int offset_y = 6;

    if (game->selected_weapon == WEAPON_KNIFE) {
        sprite = game->weapon_flash > 0.0 ? WEAPON_SPRITE_KNIFE_SLASH : WEAPON_SPRITE_KNIFE;
        size = 82;
        offset_x = 10;
    } else if (game->selected_weapon == WEAPON_FIREBALL) {
        sprite = game->weapon_flash > 0.0 ? WEAPON_SPRITE_FIREBALL_CAST : WEAPON_SPRITE_FIREBALL;
        size = 96;
        offset_y = 2;
    } else if (game->selected_weapon == WEAPON_SHOTGUN) {
        sprite = game->weapon_flash > 0.0 ? WEAPON_SPRITE_SHOTGUN_FLASH : WEAPON_SPRITE_SHOTGUN;
        size = 92;
        offset_y = 7;
    } else {
        sprite = game->weapon_flash > 0.0 ? WEAPON_SPRITE_PISTOL_FLASH : WEAPON_SPRITE_PISTOL;
        size = 86;
    }

    int x = SCREEN_W / 2 - size / 2 + recoil_x + offset_x;
    int y = SCREEN_H - size + bob + recoil_y + offset_y;
    draw_weapon_sprite(sprite, x, y, size);
}

static void render_hud(const GameState *game)
{
    fill_rect(0, SCREEN_H - 14, SCREEN_W, 14, rgb(20, 20, 22));

    fill_rect(6, SCREEN_H - 10, 74, 6, rgb(70, 18, 18));
    int hp_w = game->player_health * 74 / player_max_health(game);
    if (hp_w > 74) hp_w = 74;
    fill_rect(6, SCREEN_H - 10, hp_w, 6, game->player_health > 30 ? rgb(32, 174, 64) : rgb(210, 42, 32));

    int ammo_pips = game->ammo > 18 ? 18 : game->ammo;
    for (int i = 0; i < ammo_pips; ++i) {
        fill_rect(94 + i * 5, SCREEN_H - 11, 3, 8, rgb(204, 162, 64));
    }

    uint32_t knife_slot = game->selected_weapon == WEAPON_KNIFE ? rgb(210, 180, 90) : rgb(62, 58, 48);
    uint32_t pistol_slot = game->selected_weapon == WEAPON_PISTOL ? rgb(210, 180, 90) : rgb(62, 58, 48);
    uint32_t fire_slot = game->selected_weapon == WEAPON_FIREBALL ? rgb(230, 94, 32) : rgb(64, 42, 32);
    uint32_t shotgun_slot = game->selected_weapon == WEAPON_SHOTGUN ? rgb(210, 180, 90) : rgb(62, 58, 48);
    fill_rect(180, SCREEN_H - 12, 12, 10, knife_slot);
    draw_line(190, SCREEN_H - 11, 183, SCREEN_H - 4, rgb(210, 210, 188));
    fill_rect(194, SCREEN_H - 12, 12, 10, game->pistol_unlocked ? pistol_slot : rgb(34, 30, 28));
    fill_rect(197, SCREEN_H - 9, 6, 4, game->pistol_unlocked ? rgb(24, 24, 24) : rgb(18, 18, 18));
    fill_rect(208, SCREEN_H - 12, 12, 10, game->shotgun_unlocked ? shotgun_slot : rgb(34, 30, 28));
    fill_rect(211, SCREEN_H - 10, 6, 2, game->shotgun_unlocked ? rgb(34, 34, 34) : rgb(18, 18, 18));
    fill_rect(211, SCREEN_H - 6, 6, 2, game->shotgun_unlocked ? rgb(34, 34, 34) : rgb(18, 18, 18));
    fill_rect(222, SCREEN_H - 12, 12, 10, game->fireball_unlocked ? fire_slot : rgb(34, 30, 28));
    fill_rect(225, SCREEN_H - 9, 6, 4, game->fireball_unlocked ? rgb(255, 162, 54) : rgb(18, 18, 18));
    int fire_pips = game->fireball_ammo > 10 ? 10 : game->fireball_ammo;
    for (int i = 0; i < fire_pips; ++i) {
        fill_rect(238 + i * 3, SCREEN_H - 10, 2, 6, rgb(232, 92, 28));
    }
    if (game->gold > 0) {
        fill_rect(252, SCREEN_H - 5, 10, 2, rgb(104, 68, 20));
        int gold_pips = game->gold / 10;
        if (gold_pips < 1) gold_pips = 1;
        if (gold_pips > 6) gold_pips = 6;
        for (int i = 0; i < gold_pips; ++i) {
            fill_rect(252 + i * 2, SCREEN_H - 8 - (i & 1), 2, 2, rgb(238, 178, 54));
        }
    }
    for (int i = 0; i < RELIC_COUNT; ++i) {
        int x = 272 + i * 7;
        int collected = (game->relic_mask & (1 << i)) != 0;
        uint32_t frame = collected ? rgb(170, 104, 192) : rgb(54, 44, 60);
        uint32_t core = collected ? rgb(240, 218, 150) : rgb(26, 24, 30);
        if (!collected && game->relic_flash > 0.0) {
            frame = rgb(118, 70, 138);
        }
        fill_rect(x, SCREEN_H - 12, 5, 10, frame);
        fill_rect(x + 2, SCREEN_H - 9, 1, 4, core);
    }
    for (int i = 0; i < GENERATOR_COUNT; ++i) {
        uint32_t c = i == game->generator_mode ? rgb(92, 190, 160) : rgb(42, 48, 46);
        fill_rect(288 + i * 4, SCREEN_H - 11, 2, 8, c);
    }

    for (int i = 0; i < game->monster_count; ++i) {
        uint32_t c = i < game->kills ? rgb(120, 28, 24) : rgb(52, 132, 52);
        fill_rect(300 + i * 2, SCREEN_H - 10, 1, 6, c);
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

    if (game->game_over) {
        uint32_t c = rgb(160, 28, 28);
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
                if (wall == WALL_DOOR) {
                    c = rgb(126, 82, 40);
                } else if (wall == WALL_LOCKED_DOOR) {
                    c = rgb(132, 102, 34);
                } else if (wall >= 5) {
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
                         item->type == ITEM_PISTOL ? rgb(150, 154, 154) :
                         item->type == ITEM_GOLD ? rgb(238, 178, 54) :
                         item->type == ITEM_SHRINE ? rgb(174, 46, 42) :
                         item->type == ITEM_BONEPILE ? rgb(164, 154, 126) :
                         item->type == ITEM_KEY ? rgb(238, 202, 86) :
                         item->type == ITEM_RELIC ? rgb(232, 210, 146) : rgb(190, 70, 230);
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

    for (int i = 0; i < MAX_PORTALS; ++i) {
        const Portal *portal = &game->portals[i];
        if (portal->active && portal->x >= 0 && portal->x < MAP_W && portal->y >= 0 && portal->y < MAP_H && game->discovered[portal->y][portal->x]) {
            fill_rect(ox + portal->x * cell, oy + portal->y * cell, cell, cell,
                      portal->boss_gate ? (game->boss_unlocked ? rgb(224, 96, 42) : rgb(104, 62, 130)) :
                      (portal->exit_to_forest ? rgb(86, 170, 190) : rgb(70, 150, 72)));
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
                if (wall == WALL_DOOR) c = rgb(126, 82, 40);
                if (wall == WALL_LOCKED_DOOR) c = rgb(132, 102, 34);
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
                         item->type == ITEM_PISTOL ? rgb(150, 154, 154) :
                         item->type == ITEM_GOLD ? rgb(238, 178, 54) :
                         item->type == ITEM_SHRINE ? rgb(174, 46, 42) :
                         item->type == ITEM_BONEPILE ? rgb(164, 154, 126) :
                         item->type == ITEM_RAPID ? rgb(54, 180, 230) :
                         item->type == ITEM_DAMAGE ? rgb(190, 70, 230) :
                         item->type == ITEM_RELIC ? rgb(232, 210, 146) : rgb(220, 170, 64);
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

    for (int i = 0; i < MAX_PORTALS; ++i) {
        const Portal *portal = &game->portals[i];
        if (portal->active && portal->x >= 0 && portal->x < MAP_W && portal->y >= 0 && portal->y < MAP_H && game->discovered[portal->y][portal->x]) {
            uint32_t c = portal->boss_gate ? (game->boss_unlocked ? rgb(230, 100, 44) : rgb(112, 66, 142)) :
                         (portal->exit_to_forest ? rgb(92, 180, 200) : rgb(76, 160, 78));
            fill_rect(ox + portal->x * cell + 1, oy + portal->y * cell + 1, cell - 3, cell - 3, c);
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

static void render_interaction_prompt(const Camera *cam, const GameState *game);
static void render_help_overlay(const GameState *game);
static void render_relic_notice(const GameState *game);
static void render_victory_screen(void);
static int can_buy_merchant_shop_item(const GameState *game, int item);

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
            int scale = monster->is_boss ? 3 : (monster->type == MONSTER_GIANT_SKELETON ? 2 : 1);
            render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2 - 3, sh * scale / 3, sh * scale / 14 + 1, depth, rgb(0, 0, 0), monster->is_boss ? 0.52 : 0.34);
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
    const double horizon = SCREEN_H * 0.5;
    const double pos_z = SCREEN_H * 0.5;

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

        int screen_radius = sh / 2 + 4;
        if (screen_radius < 2) {
            continue;
        }
        if (screen_radius > SCREEN_W) {
            screen_radius = SCREEN_W;
        }

        int center_y = (int)(horizon + pos_z / depth);
        int start_x = sx - screen_radius;
        int end_x = sx + screen_radius;
        int start_y = center_y - screen_radius;
        int end_y = center_y + screen_radius;
        if (start_x < 0) start_x = 0;
        if (end_x >= SCREEN_W) end_x = SCREEN_W - 1;
        if (start_y <= (int)horizon) start_y = (int)horizon + 1;
        if (end_y >= SCREEN_H - 14) end_y = SCREEN_H - 15;
        if (start_y > end_y || start_x > end_x) {
            continue;
        }

        int variant = decal->variant % DECAL_COUNT;
        if (variant < 0) {
            variant = 0;
        }
        double fade = decal->max_life > 0.0 ? clamp01(decal->life / decal->max_life) : clamp01(decal->life / 30.0);
        double ca = cos(decal->angle);
        double sa = sin(decal->angle);
        double half_extent = decal->radius * 0.5;
        if (half_extent < 0.04) {
            half_extent = 0.04;
        }
        for (int y = start_y; y <= end_y; ++y) {
            double row_distance = pos_z / (y - horizon);
            for (int x = start_x; x <= end_x; ++x) {
                if (row_distance >= z_buffer[x] + 0.08) {
                    continue;
                }
                double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
                double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
                double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;
                double world_x = cam->pos.x + row_distance * ray_dir_x;
                double world_y = cam->pos.y + row_distance * ray_dir_y;
                double dx = world_x - decal->pos.x;
                double dy = world_y - decal->pos.y;
                double rx = (dx * ca + dy * sa) / half_extent;
                double ry = (-dx * sa + dy * ca) / half_extent;
                int tx = (int)((rx * 0.5 + 0.5) * DECAL_SIZE);
                int ty = (int)((ry * 0.5 + 0.5) * DECAL_SIZE);
                if (tx < 0 || tx >= DECAL_SIZE || ty < 0 || ty >= DECAL_SIZE) {
                    continue;
                }
                uint32_t color = decal_sprites[variant][ty * DECAL_SIZE + tx];
                if (is_sprite_key(color)) {
                    continue;
                }
                int idx = y * SCREEN_W + x;
                double amount = (0.48 + 0.28 * luminance(color)) * fade;
                framebuffer[idx] = mix_color(framebuffer[idx], apply_fog(color, row_distance, 0.58), clamp01(amount));
            }
        }
    }
}

static void clear_wall_decal_index(GameState *game)
{
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            game->wall_decal_head[y][x][0] = -1;
            game->wall_decal_head[y][x][1] = -1;
        }
    }
}

static void link_wall_decal(GameState *game, int index)
{
    WallDecal *decal = &game->wall_decals[index];
    if (!decal->active ||
        decal->x < 0 || decal->x >= MAP_W ||
        decal->y < 0 || decal->y >= MAP_H ||
        decal->side < 0 || decal->side > 1) {
        decal->next = -1;
        return;
    }

    decal->next = game->wall_decal_head[decal->y][decal->x][decal->side];
    game->wall_decal_head[decal->y][decal->x][decal->side] = index;
}

static uint32_t apply_wall_decals(const GameState *game, int map_x, int map_y, int side, double wall_u, double wall_v, uint32_t lit)
{
    if (map_x < 0 || map_x >= MAP_W || map_y < 0 || map_y >= MAP_H || side < 0 || side > 1) {
        return lit;
    }

    int i = game->wall_decal_head[map_y][map_x][side];
    int guard = 0;
    while (i >= 0 && i < MAX_WALL_DECALS && guard++ < MAX_WALL_DECALS) {
        const WallDecal *decal = &game->wall_decals[i];
        if (!decal->active) {
            break;
        }

        double du = (wall_u - decal->u) / decal->width + 0.5;
        double dv = (wall_v - decal->v) / decal->height + 0.5;
        if (du < 0.0 || du >= 1.0 || dv < 0.0 || dv >= 1.0) {
            i = decal->next;
            continue;
        }

        int variant = decal->variant % WALL_DECAL_COUNT;
        if (variant < 0) {
            variant = 0;
        }
        int tx = (int)(du * WALL_DECAL_SIZE);
        int ty = (int)(dv * WALL_DECAL_SIZE);
        if (tx < 0 || tx >= WALL_DECAL_SIZE || ty < 0 || ty >= WALL_DECAL_SIZE) {
            i = decal->next;
            continue;
        }

        uint32_t color = wall_decal_sprites[variant][ty * WALL_DECAL_SIZE + tx];
        if (is_sprite_key(color)) {
            i = decal->next;
            continue;
        }

        double amount = clamp01(decal->strength * (0.50 + luminance(color) * 0.30));
        lit = mix_color(lit, color, amount);
        i = decal->next;
    }
    return lit;
}

enum {
    HOUSE_TEX_FRONT = 0,
    HOUSE_TEX_SIDE = 1,
    HOUSE_TEX_BACK = 2,
    HOUSE_TEX_ROOF = 3,
};

enum {
    HOUSE_FACE_WEST = 0,
    HOUSE_FACE_EAST = 1,
    HOUSE_FACE_NORTH = 2,
    HOUSE_FACE_SOUTH = 3,
};

typedef struct {
    const House *house;
    double depth;
    double hit_x;
    double hit_y;
    double u;
    int face;
} HouseHit;

static int intersect_house_ray(const House *house, const Camera *cam, double ray_dir_x, double ray_dir_y, HouseHit *hit)
{
    if (!house->active) {
        return 0;
    }

    double t_min = -1e30;
    double t_max = 1e30;
    int face = -1;

    if (fabs(ray_dir_x) < 0.000001) {
        if (cam->pos.x < house_min_x(house) || cam->pos.x > house_max_x(house)) {
            return 0;
        }
    } else {
        double tx1 = (house_min_x(house) - cam->pos.x) / ray_dir_x;
        double tx2 = (house_max_x(house) - cam->pos.x) / ray_dir_x;
        int near_face = ray_dir_x > 0.0 ? HOUSE_FACE_WEST : HOUSE_FACE_EAST;
        if (tx1 > tx2) {
            double t = tx1;
            tx1 = tx2;
            tx2 = t;
        }
        if (tx1 > t_min) {
            t_min = tx1;
            face = near_face;
        }
        if (tx2 < t_max) t_max = tx2;
    }

    if (fabs(ray_dir_y) < 0.000001) {
        if (cam->pos.y < house_min_y(house) || cam->pos.y > house_max_y(house)) {
            return 0;
        }
    } else {
        double ty1 = (house_min_y(house) - cam->pos.y) / ray_dir_y;
        double ty2 = (house_max_y(house) - cam->pos.y) / ray_dir_y;
        int near_face = ray_dir_y > 0.0 ? HOUSE_FACE_NORTH : HOUSE_FACE_SOUTH;
        if (ty1 > ty2) {
            double t = ty1;
            ty1 = ty2;
            ty2 = t;
        }
        if (ty1 > t_min) {
            t_min = ty1;
            face = near_face;
        }
        if (ty2 < t_max) t_max = ty2;
    }

    if (face < 0 || t_min > t_max || t_min <= HOUSE_RENDER_NEAR_CLIP) {
        return 0;
    }

    double hit_x = cam->pos.x + ray_dir_x * t_min;
    double hit_y = cam->pos.y + ray_dir_y * t_min;
    double u;
    if (face == HOUSE_FACE_WEST || face == HOUSE_FACE_EAST) {
        u = (hit_y - house_min_y(house)) / (house_max_y(house) - house_min_y(house));
        if (face == HOUSE_FACE_EAST) {
            u = 1.0 - u;
        }
    } else {
        u = (hit_x - house_min_x(house)) / (house_max_x(house) - house_min_x(house));
        if (face == HOUSE_FACE_NORTH) {
            u = 1.0 - u;
        }
    }

    hit->house = house;
    hit->depth = t_min;
    hit->hit_x = hit_x;
    hit->hit_y = hit_y;
    hit->u = clamp01(u);
    hit->face = face;
    return 1;
}

static uint32_t house_texel_for_face(int face, double u, double v)
{
    double wall_v = 0.16 + clamp01(v) * 0.84;
    if (face == HOUSE_FACE_WEST || face == HOUSE_FACE_EAST) {
        wall_v = 0.34 + clamp01(v) * 0.66;
    }
    if (face == HOUSE_FACE_WEST) {
        int tex_x = (int)(clamp01(u) * (TEX_SIZE - 1));
        int tex_y = (int)(wall_v * (TEX_SIZE - 1));
        return house_textures[HOUSE_TEX_FRONT][tex_y * TEX_SIZE + tex_x];
    }
    int tex = face == HOUSE_FACE_EAST ? HOUSE_TEX_BACK : HOUSE_TEX_SIDE;
    int tex_x = (int)(clamp01(u) * (TEX_SIZE - 1));
    int tex_y = (int)(wall_v * (TEX_SIZE - 1));
    return house_textures[tex][tex_y * TEX_SIZE + tex_x];
}

typedef struct {
    double x;
    double y;
    double depth;
} ProjectedPoint;

typedef struct {
    ProjectedPoint p;
    double u;
    double v;
} TexturedPoint;

static int project_world_point(const Camera *cam, double world_x, double world_y, double z, ProjectedPoint *out)
{
    double rel_x = world_x - cam->pos.x;
    double rel_y = world_y - cam->pos.y;
    double inv_det = 1.0 / (cam->plane.x * cam->dir.y - cam->dir.x * cam->plane.y);
    double transform_x = inv_det * (cam->dir.y * rel_x - cam->dir.x * rel_y);
    double transform_y = inv_det * (-cam->plane.y * rel_x + cam->plane.x * rel_y);

    if (transform_y <= HOUSE_RENDER_NEAR_CLIP) {
        return 0;
    }

    out->x = (SCREEN_W / 2.0) * (1.0 + transform_x / transform_y);
    out->y = SCREEN_H * 0.5 - SCREEN_H * (z - 0.5) / transform_y;
    out->depth = transform_y;
    return 1;
}

static double prop_min_x(const Prop *prop)
{
    return prop->pos.x - prop->half_w;
}

static double prop_max_x(const Prop *prop)
{
    return prop->pos.x + prop->half_w;
}

static double prop_min_y(const Prop *prop)
{
    return prop->pos.y - prop->half_d;
}

static double prop_max_y(const Prop *prop)
{
    return prop->pos.y + prop->half_d;
}

static int prop_is_cylinder(const Prop *prop)
{
    return prop && prop->type == PROP_BARREL;
}

static double prop_footprint_radius(const Prop *prop)
{
    return fmin(prop->half_w, prop->half_d);
}

static double prop_face_light(int face)
{
    switch (face) {
    case HOUSE_FACE_WEST: return 0.58;
    case HOUSE_FACE_SOUTH: return 0.50;
    case HOUSE_FACE_EAST: return 0.46;
    case HOUSE_FACE_NORTH: return 0.40;
    default: return 0.45;
    }
}

static void render_prop_triangle(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, const Prop *prop, double light)
{
    double denom = (b.p.y - c.p.y) * (a.p.x - c.p.x) + (c.p.x - b.p.x) * (a.p.y - c.p.y);
    if (fabs(denom) < 0.0001) {
        return;
    }

    int min_x = (int)floor(fmin(a.p.x, fmin(b.p.x, c.p.x)));
    int max_x = (int)ceil(fmax(a.p.x, fmax(b.p.x, c.p.x)));
    int min_y = (int)floor(fmin(a.p.y, fmin(b.p.y, c.p.y)));
    int max_y = (int)ceil(fmax(a.p.y, fmax(b.p.y, c.p.y)));
    if (min_x < 0) min_x = 0;
    if (max_x >= SCREEN_W) max_x = SCREEN_W - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= SCREEN_H - 14) max_y = SCREEN_H - 15;
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double px = x + 0.5;
            double py = y + 0.5;
            double wa = ((b.p.y - c.p.y) * (px - c.p.x) + (c.p.x - b.p.x) * (py - c.p.y)) / denom;
            double wb = ((c.p.y - a.p.y) * (px - c.p.x) + (a.p.x - c.p.x) * (py - c.p.y)) / denom;
            double wc = 1.0 - wa - wb;
            if (wa < -0.0001 || wb < -0.0001 || wc < -0.0001) {
                continue;
            }

            double inv_depth = wa / a.p.depth + wb / b.p.depth + wc / c.p.depth;
            if (inv_depth <= 0.0) {
                continue;
            }
            double depth = 1.0 / inv_depth;
            int idx = y * SCREEN_W + x;
            if (depth >= depth_buffer[idx] - 0.02) {
                continue;
            }

            double tex_u = (wa * a.u / a.p.depth + wb * b.u / b.p.depth + wc * c.u / c.p.depth) / inv_depth;
            double tex_v = (wa * a.v / a.p.depth + wb * b.v / b.p.depth + wc * c.v / c.p.depth) / inv_depth;
            uint32_t color = prop_texel(prop, tex_u, tex_v);
            uint32_t lit = shade(color, light);
            if (prop->looted && prop->loot_slot >= 0) {
                lit = mix_color(lit, rgb(32, 26, 22), 0.20);
            }
            framebuffer[idx] = apply_game_fog(game, lit, depth, 0.74);
            depth_buffer[idx] = depth;
        }
    }
}

static double prop_surface_light(const Camera *cam, const GameState *game, double world_x, double world_y, double face_light)
{
    double depth = (world_x - cam->pos.x) * cam->dir.x + (world_y - cam->pos.y) * cam->dir.y;
    if (depth < 0.10) {
        depth = 0.10;
    }
    double torch_light = clamp01(torch_light_at(world_x, world_y, game->time) * 0.52 +
                                 player_torch_light_at(cam, game, world_x, world_y) * 0.58);
    return (face_light + torch_light * 0.75) / (1.0 + depth * 0.050);
}

static void render_prop_quad(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, TexturedPoint d, const Prop *prop, double light)
{
    render_prop_triangle(game, a, b, c, prop, light);
    render_prop_triangle(game, a, c, d, prop, light * 0.98);
}

static void render_prop_vertical_quad(const Camera *cam,
                                      const GameState *game,
                                      const Prop *prop,
                                      double ax,
                                      double ay,
                                      double bx,
                                      double by,
                                      double u0,
                                      double u1,
                                      double light)
{
    ProjectedPoint bottom_a;
    ProjectedPoint bottom_b;
    ProjectedPoint top_b;
    ProjectedPoint top_a;
    if (!project_world_point(cam, ax, ay, 0.0, &bottom_a) ||
        !project_world_point(cam, bx, by, 0.0, &bottom_b) ||
        !project_world_point(cam, bx, by, prop->height, &top_b) ||
        !project_world_point(cam, ax, ay, prop->height, &top_a)) {
        return;
    }

    TexturedPoint a = {bottom_a, u0, 1.0};
    TexturedPoint b = {bottom_b, u1, 1.0};
    TexturedPoint c = {top_b, u1, 0.0};
    TexturedPoint d = {top_a, u0, 0.0};
    render_prop_quad(game, a, b, c, d, prop, light);
}

static void render_prop_box_sides(const Camera *cam, const GameState *game, const Prop *prop)
{
    double x0 = prop_min_x(prop);
    double x1 = prop_max_x(prop);
    double y0 = prop_min_y(prop);
    double y1 = prop_max_y(prop);
    double cx = prop->pos.x;
    double cy = prop->pos.y;

    render_prop_vertical_quad(cam, game, prop, x0, y1, x0, y0, 0.0, 1.0,
                              prop_surface_light(cam, game, x0, cy, prop_face_light(HOUSE_FACE_WEST)));
    render_prop_vertical_quad(cam, game, prop, x1, y0, x1, y1, 0.0, 1.0,
                              prop_surface_light(cam, game, x1, cy, prop_face_light(HOUSE_FACE_EAST)));
    render_prop_vertical_quad(cam, game, prop, x0, y0, x1, y0, 0.0, 1.0,
                              prop_surface_light(cam, game, cx, y0, prop_face_light(HOUSE_FACE_NORTH)));
    render_prop_vertical_quad(cam, game, prop, x1, y1, x0, y1, 0.0, 1.0,
                              prop_surface_light(cam, game, cx, y1, prop_face_light(HOUSE_FACE_SOUTH)));
}

static void render_prop_top(const Camera *cam, const GameState *game, const Prop *prop)
{
    ProjectedPoint p00;
    ProjectedPoint p10;
    ProjectedPoint p11;
    ProjectedPoint p01;
    double z = prop->height;
    if (!project_world_point(cam, prop_min_x(prop), prop_min_y(prop), z, &p00) ||
        !project_world_point(cam, prop_max_x(prop), prop_min_y(prop), z, &p10) ||
        !project_world_point(cam, prop_max_x(prop), prop_max_y(prop), z, &p11) ||
        !project_world_point(cam, prop_min_x(prop), prop_max_y(prop), z, &p01)) {
        return;
    }

    double light = 0.72 / (1.0 + fmin(fmin(p00.depth, p10.depth), fmin(p11.depth, p01.depth)) * 0.045);
    TexturedPoint a = {p00, 0.0, 0.0};
    TexturedPoint b = {p10, 1.0, 0.0};
    TexturedPoint c = {p11, 1.0, 1.0};
    TexturedPoint d = {p01, 0.0, 1.0};
    render_prop_triangle(game, a, b, c, prop, light);
    render_prop_triangle(game, a, c, d, prop, light * 0.96);
}

static void render_prop_cylinder_sides(const Camera *cam, const GameState *game, const Prop *prop)
{
    double radius = prop_footprint_radius(prop);
    if (radius <= 0.02) {
        return;
    }

    const int segments = 18;
    for (int i = 0; i < segments; ++i) {
        double a0 = i * (M_PI * 2.0 / segments);
        double a1 = (i + 1) * (M_PI * 2.0 / segments);
        double mid = (a0 + a1) * 0.5;
        double normal_x = cos(mid);
        double normal_y = sin(mid);
        double face_light = fmax(0.38, fmin(0.60, 0.48 - normal_x * 0.07 + normal_y * 0.04));
        double sx = prop->pos.x + normal_x * radius;
        double sy = prop->pos.y + normal_y * radius;
        double light = prop_surface_light(cam, game, sx, sy, face_light);
        render_prop_vertical_quad(cam,
                                  game,
                                  prop,
                                  prop->pos.x + cos(a0) * radius,
                                  prop->pos.y + sin(a0) * radius,
                                  prop->pos.x + cos(a1) * radius,
                                  prop->pos.y + sin(a1) * radius,
                                  i / (double)segments,
                                  (i + 1) / (double)segments,
                                  light);
    }
}

static void render_prop_cylinder_top(const Camera *cam, const GameState *game, const Prop *prop)
{
    ProjectedPoint center;
    double z = prop->height;
    double radius = prop_footprint_radius(prop);
    if (radius <= 0.02 || !project_world_point(cam, prop->pos.x, prop->pos.y, z, &center)) {
        return;
    }

    const int segments = 18;
    for (int i = 0; i < segments; ++i) {
        double a0 = i * (M_PI * 2.0 / segments);
        double a1 = (i + 1) * (M_PI * 2.0 / segments);
        ProjectedPoint p0;
        ProjectedPoint p1;
        if (!project_world_point(cam, prop->pos.x + cos(a0) * radius, prop->pos.y + sin(a0) * radius, z, &p0) ||
            !project_world_point(cam, prop->pos.x + cos(a1) * radius, prop->pos.y + sin(a1) * radius, z, &p1)) {
            continue;
        }
        double light = 0.72 / (1.0 + fmin(center.depth, fmin(p0.depth, p1.depth)) * 0.045);
        TexturedPoint a = {center, 0.5, 0.5};
        TexturedPoint b = {p0, 0.5 + cos(a0) * 0.5, 0.5 + sin(a0) * 0.5};
        TexturedPoint c = {p1, 0.5 + cos(a1) * 0.5, 0.5 + sin(a1) * 0.5};
        render_prop_triangle(game, a, b, c, prop, light);
    }
}

static void render_props_3d(const Camera *cam, const GameState *game)
{
    if (game->generator_mode != GENERATOR_HOUSE) {
        return;
    }

    for (int i = 0; i < MAX_PROPS; ++i) {
        const Prop *prop = &game->props[i];
        if (!prop->active || prop->height <= 0.02) {
            continue;
        }
        if (prop_is_cylinder(prop)) {
            render_prop_cylinder_sides(cam, game, prop);
            render_prop_cylinder_top(cam, game, prop);
        } else {
            render_prop_box_sides(cam, game, prop);
            render_prop_top(cam, game, prop);
        }
    }
}

static void render_roof_triangle(const GameState *game, ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, uint32_t color)
{
    double denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (fabs(denom) < 0.0001) {
        return;
    }

    int min_x = (int)floor(fmin(a.x, fmin(b.x, c.x)));
    int max_x = (int)ceil(fmax(a.x, fmax(b.x, c.x)));
    int min_y = (int)floor(fmin(a.y, fmin(b.y, c.y)));
    int max_y = (int)ceil(fmax(a.y, fmax(b.y, c.y)));
    if (min_x < 0) min_x = 0;
    if (max_x >= SCREEN_W) max_x = SCREEN_W - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= SCREEN_H - 14) max_y = SCREEN_H - 15;
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double px = x + 0.5;
            double py = y + 0.5;
            double wa = ((b.y - c.y) * (px - c.x) + (c.x - b.x) * (py - c.y)) / denom;
            double wb = ((c.y - a.y) * (px - c.x) + (a.x - c.x) * (py - c.y)) / denom;
            double wc = 1.0 - wa - wb;
            if (wa < -0.0001 || wb < -0.0001 || wc < -0.0001) {
                continue;
            }

            double inv_depth = wa / a.depth + wb / b.depth + wc / c.depth;
            if (inv_depth <= 0.0) {
                continue;
            }
            double depth = 1.0 / inv_depth;
            int idx = y * SCREEN_W + x;
            if (depth >= depth_buffer[idx] - 0.02) {
                continue;
            }

            framebuffer[idx] = apply_game_fog(game, color, depth, 0.82);
            depth_buffer[idx] = depth;
        }
    }
}

static void render_gable_triangle(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, int tex, double light)
{
    double denom = (b.p.y - c.p.y) * (a.p.x - c.p.x) + (c.p.x - b.p.x) * (a.p.y - c.p.y);
    if (fabs(denom) < 0.0001) {
        return;
    }

    int min_x = (int)floor(fmin(a.p.x, fmin(b.p.x, c.p.x)));
    int max_x = (int)ceil(fmax(a.p.x, fmax(b.p.x, c.p.x)));
    int min_y = (int)floor(fmin(a.p.y, fmin(b.p.y, c.p.y)));
    int max_y = (int)ceil(fmax(a.p.y, fmax(b.p.y, c.p.y)));
    if (min_x < 0) min_x = 0;
    if (max_x >= SCREEN_W) max_x = SCREEN_W - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= SCREEN_H - 14) max_y = SCREEN_H - 15;
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double px = x + 0.5;
            double py = y + 0.5;
            double wa = ((b.p.y - c.p.y) * (px - c.p.x) + (c.p.x - b.p.x) * (py - c.p.y)) / denom;
            double wb = ((c.p.y - a.p.y) * (px - c.p.x) + (a.p.x - c.p.x) * (py - c.p.y)) / denom;
            double wc = 1.0 - wa - wb;
            if (wa < -0.0001 || wb < -0.0001 || wc < -0.0001) {
                continue;
            }

            double inv_depth = wa / a.p.depth + wb / b.p.depth + wc / c.p.depth;
            if (inv_depth <= 0.0) {
                continue;
            }
            double depth = 1.0 / inv_depth;
            int idx = y * SCREEN_W + x;
            if (depth >= depth_buffer[idx] - 0.02) {
                continue;
            }

            double tex_u = (wa * a.u / a.p.depth + wb * b.u / b.p.depth + wc * c.u / c.p.depth) / inv_depth;
            double tex_v = (wa * a.v / a.p.depth + wb * b.v / b.p.depth + wc * c.v / c.p.depth) / inv_depth;
            int tex_x = (int)(clamp01(tex_u) * (TEX_SIZE - 1));
            int tex_y = (int)(clamp01(tex_v) * (TEX_SIZE - 1));
            uint32_t color = house_textures[tex][tex_y * TEX_SIZE + tex_x];

            uint32_t lit = shade(color, light);
            framebuffer[idx] = apply_game_fog(game, lit, depth, 0.86);
            depth_buffer[idx] = depth;
        }
    }
}

static void render_house_roof_quad(const GameState *game, ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, ProjectedPoint d, uint32_t color)
{
    render_roof_triangle(game, a, b, c, color);
    render_roof_triangle(game, a, c, d, color);
}

static void render_house_roofs_and_gables(const Camera *cam, const GameState *game)
{
    for (int i = 0; i < MAX_HOUSES; ++i) {
        const House *house = &game->houses[i];
        if (!house->active) {
            continue;
        }

        double x0 = house_min_x(house) - HOUSE_ROOF_OVERHANG;
        double x1 = house_max_x(house) + HOUSE_ROOF_OVERHANG;
        double y0 = house_min_y(house) - HOUSE_ROOF_OVERHANG;
        double y1 = house_max_y(house) + HOUSE_ROOF_OVERHANG;
        double ridge_y = house->pos.y;
        double eave_z = HOUSE_WALL_HEIGHT;
        double ridge_z = HOUSE_WALL_HEIGHT + HOUSE_ROOF_RISE;

        ProjectedPoint north_west;
        ProjectedPoint north_east;
        ProjectedPoint ridge_west;
        ProjectedPoint ridge_east;
        ProjectedPoint south_west;
        ProjectedPoint south_east;
        if (!project_world_point(cam, x0, y0, eave_z, &north_west) ||
            !project_world_point(cam, x1, y0, eave_z, &north_east) ||
            !project_world_point(cam, x0, ridge_y, ridge_z, &ridge_west) ||
            !project_world_point(cam, x1, ridge_y, ridge_z, &ridge_east) ||
            !project_world_point(cam, x0, y1, eave_z, &south_west) ||
            !project_world_point(cam, x1, y1, eave_z, &south_east)) {
            continue;
        }

        double torch_light = torch_light_at(house->pos.x, house->pos.y, game->time);
        double moon_light = forest_moon_visibility_at(house->pos.x, house->pos.y);
        uint32_t west_roof = shade(rgb(38, 48, 46), 0.62 + torch_light * 0.20 + moon_light * 0.10);
        uint32_t east_roof = shade(rgb(24, 32, 34), 0.54 + torch_light * 0.16 + moon_light * 0.08);
        west_roof = mix_color(west_roof, rgb(96, 122, 108), moon_light * 0.08);
        east_roof = mix_color(east_roof, rgb(80, 106, 96), moon_light * 0.06);

        ProjectedPoint front_left;
        ProjectedPoint front_right;
        ProjectedPoint front_peak;
        ProjectedPoint back_left;
        ProjectedPoint back_right;
        ProjectedPoint back_peak;
        if (project_world_point(cam, house_min_x(house), house_min_y(house), eave_z, &front_left) &&
            project_world_point(cam, house_min_x(house), house_max_y(house), eave_z, &front_right) &&
            project_world_point(cam, house_min_x(house), house->pos.y, ridge_z, &front_peak)) {
            TexturedPoint a = {front_left, 0.0, 0.34};
            TexturedPoint b = {front_right, 1.0, 0.34};
            TexturedPoint c = {front_peak, 0.5, 0.0};
            render_gable_triangle(game, a, b, c, HOUSE_TEX_FRONT, 0.58 + torch_light * 0.20 + moon_light * 0.09);
        }
        if (project_world_point(cam, house_max_x(house), house_max_y(house), eave_z, &back_left) &&
            project_world_point(cam, house_max_x(house), house_min_y(house), eave_z, &back_right) &&
            project_world_point(cam, house_max_x(house), house->pos.y, ridge_z, &back_peak)) {
            TexturedPoint a = {back_left, 0.0, 0.34};
            TexturedPoint b = {back_right, 1.0, 0.34};
            TexturedPoint c = {back_peak, 0.5, 0.0};
            render_gable_triangle(game, a, b, c, HOUSE_TEX_BACK, 0.48 + torch_light * 0.16 + moon_light * 0.07);
        }

        render_house_roof_quad(game, north_west, north_east, ridge_east, ridge_west, west_roof);
        render_house_roof_quad(game, ridge_west, ridge_east, south_east, south_west, east_roof);
    }
}

static void render_houses(const Camera *cam, const GameState *game)
{
    if (game->generator_mode != GENERATOR_FOREST) {
        return;
    }

    for (int x = 0; x < SCREEN_W; ++x) {
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;
        HouseHit best = {0};
        best.depth = z_buffer[x];

        for (int i = 0; i < MAX_HOUSES; ++i) {
            HouseHit hit;
            if (intersect_house_ray(&game->houses[i], cam, ray_dir_x, ray_dir_y, &hit) && hit.depth < best.depth) {
                best = hit;
            }
        }
        if (!best.house || best.depth >= z_buffer[x]) {
            continue;
        }

        double raw_top = SCREEN_H * 0.5 - SCREEN_H * (HOUSE_WALL_HEIGHT - 0.5) / best.depth;
        double raw_bottom = SCREEN_H * 0.5 + SCREEN_H * 0.5 / best.depth;
        if (raw_bottom <= 0.0 || raw_top >= SCREEN_H - 14) {
            continue;
        }

        int draw_start = (int)floor(raw_top);
        int draw_end = (int)ceil(raw_bottom);
        if (draw_start < 0) draw_start = 0;
        if (draw_end >= SCREEN_H - 14) draw_end = SCREEN_H - 15;
        if (draw_start > draw_end) {
            continue;
        }

        double face_light = best.face == HOUSE_FACE_WEST ? 0.50 :
                            best.face == HOUSE_FACE_SOUTH ? 0.44 :
                            best.face == HOUSE_FACE_EAST ? 0.44 :
                            best.face == HOUSE_FACE_NORTH ? 0.38 : 0.34;
        double torch_light = torch_light_at(best.hit_x, best.hit_y, game->time);
        double moon_light = forest_moon_visibility_at(best.hit_x, best.hit_y) * 0.34;
        double distance_fade = 1.0 / (1.0 + best.depth * 0.055);
        double light = (face_light + torch_light * 0.52 + moon_light * 0.20) * distance_fade;

        for (int y = draw_start; y <= draw_end; ++y) {
            double v = (y + 0.5 - raw_top) / (raw_bottom - raw_top);
            if (v < 0.0 || v > 1.0) {
                continue;
            }
            uint32_t color = house_texel_for_face(best.face, best.u, v);
            uint32_t lit = shade(color, light);
            lit = mix_color(lit, rgb(108, 136, 122), clamp01(moon_light * 0.13));
            lit = mix_color(lit, rgb(255, 132, 48), clamp01(torch_light * 0.10));
            framebuffer[y * SCREEN_W + x] = apply_game_fog(game, lit, best.depth, 0.86);
            depth_buffer[y * SCREEN_W + x] = best.depth;
            add_light(x, y, torch_light * 0.08 + moon_light * 0.025);
        }
        z_buffer[x] = best.depth;
    }
}

static double elapsed_ms(uint64_t start, uint64_t end)
{
    return (double)(end - start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

static void render_scene(const Camera *cam, const GameState *game)
{
    Camera shake_cam = *cam;
    if (game->screen_shake_timer > 0.0 && game->screen_shake_strength > 0.0) {
        double t = clamp01(game->screen_shake_timer / SCREEN_SHAKE_TIME);
        double jitter_x = sin(game->time * 91.0) * game->screen_shake_strength * t;
        double jitter_y = cos(game->time * 117.0) * game->screen_shake_strength * 0.65 * t;
        Vec2 right = {-shake_cam.dir.y, shake_cam.dir.x};
        shake_cam.pos.x += right.x * jitter_x + shake_cam.dir.x * jitter_y;
        shake_cam.pos.y += right.y * jitter_x + shake_cam.dir.y * jitter_y;
        cam = &shake_cam;
    }

    int fast_render = render_quality == RENDER_QUALITY_FAST;
    RenderProfile *profile = active_profile;
    uint64_t total_start = 0;
    uint64_t pass_start = 0;
    if (profile) {
        memset(profile, 0, sizeof(*profile));
        total_start = SDL_GetPerformanceCounter();
        pass_start = total_start;
    }

    prepare_torch_flicker_cache(game->time);
    if (game->generator_mode == GENERATOR_FOREST && !moon_visibility_cache_ready) {
        build_moon_visibility_cache();
    }
    reset_render_buffers();
    render_floor_ceiling(cam, game);
    render_forest_moon(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->floor_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

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
            if (wall > 0) {
                const Door *door = is_door_wall(wall) ? door_at_tile(game, map_x, map_y) : NULL;
                if (door && door->opening) {
                    double door_dist = side == 0
                        ? (side_dist_x - delta_dist_x)
                        : (side_dist_y - delta_dist_y);
                    if (door_dist < 0.001) {
                        door_dist = 0.001;
                    }
                    if (!ray_hits_opening_door(door, ray_wall_u(cam, ray_dir_x, ray_dir_y, door_dist, side))) {
                        continue;
                    }
                }
                hit = 1;
            }
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

        double hit_x = cam->pos.x + ray_dir_x * perp_wall_dist;
        double hit_y = cam->pos.y + ray_dir_y * perp_wall_dist;

        double wall_u = ray_wall_u(cam, ray_dir_x, ray_dir_y, perp_wall_dist, side);
        double tex_u = wall_u;
        if (game->generator_mode == GENERATOR_FOREST) {
            double world_u = side == 0 ? hit_y : hit_x;
            tex_u = world_u * 0.38;
            tex_u -= floor(tex_u);
        }
        const int wall_tex_gutter = 3;
        int tex_x = wall_tex_gutter + (int)(tex_u * (double)(TEX_SIZE - wall_tex_gutter * 2));
        if (tex_x >= TEX_SIZE - wall_tex_gutter) {
            tex_x = TEX_SIZE - wall_tex_gutter - 1;
        }

        int tex_idx = wall_texture_index(wall);
        if (game->generator_mode == GENERATOR_FOREST) {
            tex_idx = 4;
        }
        double step = (double)TEX_SIZE / line_h;
        double tex_pos = (draw_start - SCREEN_H / 2.0 + line_h / 2.0) * step;
        double light = side == 1 ? 0.17 : 0.27;
        if (game->generator_mode == GENERATOR_FOREST) {
            light = side == 1 ? 0.28 : 0.38;
        }
        light *= 1.0 / (1.0 + perp_wall_dist * 0.12);
        double player_light = player_torch_light_at(cam, game, hit_x, hit_y);
        double torch_light = clamp01(torch_light_at(hit_x, hit_y, game->time) * (side == 1 ? 0.92 : 1.08) + player_light);
        double moon_light = game->generator_mode == GENERATOR_FOREST ? 0.72 : 0.0;
        double fresnel = 0.0;
        double bump_light_x = 0.0;
        double bump_light_y = 0.0;
        if (!fast_render) {
            double ray_len = sqrt(ray_dir_x * ray_dir_x + ray_dir_y * ray_dir_y);
            double view_facing = ray_len > 0.001
                ? (side == 0 ? fabs(ray_dir_x) / ray_len : fabs(ray_dir_y) / ray_len)
                : 1.0;
            fresnel = pow5(1.0 - clamp01(view_facing));
            wall_bump_light_vector(hit_x, hit_y, &bump_light_x, &bump_light_y);
        }
        uint32_t tint = game->generator_mode == GENERATOR_FOREST ? rgb(130, 154, 138) : rgb(255, 132, 48);
        uint32_t torch_tint = rgb(255, 132, 48);
        z_buffer[x] = perp_wall_dist;

        for (int y = draw_start; y <= draw_end; ++y) {
            int tex_y = ((int)tex_pos) & (TEX_SIZE - 1);
            tex_pos += step;
            int texel_idx = tex_y * TEX_SIZE + tex_x;
            uint32_t color = game->generator_mode == GENERATOR_FOREST
                ? forest_wall_textures[tex_idx][tex_y * TEX_SIZE]
                : textures[tex_idx][texel_idx];
            uint32_t lit;
            if (game->generator_mode == GENERATOR_FOREST) {
                double wall_v = tex_y / (double)(TEX_SIZE - 1);
                double height_shadow = 0.92 - smooth01(wall_v) * 0.10;
                lit = shade(
                    color,
                    height_shadow * (light * 1.04 + torch_light * 0.34 + moon_light * 0.08));
                lit = mix_color(lit, rgb(56, 74, 64), moon_light * 0.08);
            } else if (fast_render) {
                double bump = 0.5;
                lit = apply_fast_material(
                    color,
                    light * (0.84 + bump * 0.22) + torch_light * (0.62 + bump * 0.30) + moon_light * 0.08,
                    torch_light * (0.72 + bump * 0.50) + moon_light * 0.13);
            } else {
                double bump = wall_bump_light(tex_idx, tex_x, tex_y, bump_light_x, bump_light_y);
                lit = apply_pbr_material(
                    color,
                    &texture_materials[tex_idx],
                    light * (0.84 + bump * 0.22) + torch_light * (0.62 + bump * 0.30) + moon_light * 0.08,
                    torch_light * (0.72 + bump * 0.50) + moon_light * 0.13,
                    bump,
                    fresnel,
                    texture_bright[tex_idx][texel_idx],
                    game->generator_mode == GENERATOR_FOREST ? 0.06 : 0.0,
                    tint);
            }
            lit = mix_color(lit, torch_tint, clamp01(torch_light * 0.10));
            if (game->generator_mode == GENERATOR_FOREST) {
                lit = mix_color(lit, rgb(106, 136, 120), moon_light * 0.13);
            }
            if (wall != WALL_DOOR && wall != WALL_LOCKED_DOOR) {
                lit = apply_wall_decals(game, map_x, map_y, side, wall_u, (tex_y + 0.5) / TEX_SIZE, lit);
            }
            framebuffer[y * SCREEN_W + x] = apply_game_fog(game, lit, perp_wall_dist, game->generator_mode == GENERATOR_FOREST ? 1.22 : 1.0);
            depth_buffer[y * SCREEN_W + x] = perp_wall_dist;
            add_light(x, y, torch_light * 0.10 + player_light * 0.05);
            if (game->generator_mode == GENERATOR_FOREST) {
                add_light(x, y, moon_light * 0.035);
            }
        }
    }
    render_houses(cam, game);
    render_props_3d(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->wall_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    render_decals(cam, game);
    render_dynamic_shadows(cam, game);
    render_world_sprites(cam, game);
    render_house_roofs_and_gables(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->sprite_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    render_volumetric_fog(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->fog_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    render_forest_weather_overlay(game);
    render_muzzle_light(game);
    if (render_effects != RENDER_EFFECTS_OFF) {
        render_light_buffer();
        render_bloom(render_effects);
    }
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->bloom_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    if (render_effects != RENDER_EFFECTS_OFF) {
        render_edge_antialias();
        render_color_grade(game, render_effects);
    }
    render_hit_flash(game);
    render_player_damage_feedback(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->post_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    render_crosshair(game);
    render_shot_trace(game);
    render_weapon(game);
    render_relic_notice(game);
    render_interaction_prompt(cam, game);
    render_help_overlay(game);
    render_hud(game);
    render_minimap(cam, game);
    if (game->show_automap) {
        render_full_automap(cam, game);
        render_hud(game);
    }
    if (game->victory) {
        render_victory_screen();
    }
    if (profile) {
        profile->total_ms = elapsed_ms(total_start, SDL_GetPerformanceCounter());
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

static double vec_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
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

static double raycast_house_distance(Vec2 origin, Vec2 dir)
{
    if (!active_game || active_game->generator_mode != GENERATOR_FOREST) {
        return 1e30;
    }

    double best = 1e30;
    for (int i = 0; i < MAX_HOUSES; ++i) {
        const House *house = &active_game->houses[i];
        if (!house->active) {
            continue;
        }

        double t_min = -1e30;
        double t_max = 1e30;
        if (fabs(dir.x) < 0.000001) {
            if (origin.x < house_min_x(house) || origin.x > house_max_x(house)) {
                continue;
            }
        } else {
            double tx1 = (house_min_x(house) - origin.x) / dir.x;
            double tx2 = (house_max_x(house) - origin.x) / dir.x;
            if (tx1 > tx2) {
                double t = tx1;
                tx1 = tx2;
                tx2 = t;
            }
            if (tx1 > t_min) t_min = tx1;
            if (tx2 < t_max) t_max = tx2;
        }
        if (fabs(dir.y) < 0.000001) {
            if (origin.y < house_min_y(house) || origin.y > house_max_y(house)) {
                continue;
            }
        } else {
            double ty1 = (house_min_y(house) - origin.y) / dir.y;
            double ty2 = (house_max_y(house) - origin.y) / dir.y;
            if (ty1 > ty2) {
                double t = ty1;
                ty1 = ty2;
                ty2 = t;
            }
            if (ty1 > t_min) t_min = ty1;
            if (ty2 < t_max) t_max = ty2;
        }

        if (t_min <= t_max && t_max > HOUSE_RENDER_NEAR_CLIP && t_min > HOUSE_RENDER_NEAR_CLIP && t_min < best) {
            best = t_min;
        }
    }
    return best;
}

static int has_line_of_sight(Vec2 from, Vec2 to)
{
    Vec2 diff = {to.x - from.x, to.y - from.y};
    double dist = vec_len(diff);
    Vec2 dir = vec_norm(diff);
    double wall_dist = raycast_wall_distance(from, dir);
    double house_dist = raycast_house_distance(from, dir);
    double block_dist = wall_dist < house_dist ? wall_dist : house_dist;
    return block_dist + 0.12 >= dist;
}

static int monster_max_hp(int type)
{
    switch (type) {
    case 0: return 7;
    case 1: return 4;
    case 2: return 5;
    case 3: return 4;
    case MONSTER_FLYING_HEAD: return 10;
    case MONSTER_GIANT_SKELETON: return 18;
    default: return 4;
    }
}

static double monster_patrol_speed(int type)
{
    switch (type) {
    case 0: return 0.82;
    case 1: return 1.05;
    case 2: return 1.28;
    case 3: return 1.10;
    case MONSTER_FLYING_HEAD: return 0.88;
    case MONSTER_GIANT_SKELETON: return 0.58;
    default: return 1.05;
    }
}

static double monster_chase_speed(int type)
{
    switch (type) {
    case 0: return 1.15;
    case 1: return 1.55;
    case 2: return 1.95;
    case 3: return 1.35;
    case MONSTER_FLYING_HEAD: return 1.30;
    case MONSTER_GIANT_SKELETON: return 0.92;
    default: return 1.45;
    }
}

static double monster_attack_speed(int type)
{
    switch (type) {
    case 0: return 1.05;
    case 1: return 1.65;
    case 2: return 2.10;
    case 3: return 1.45;
    case MONSTER_FLYING_HEAD: return 1.25;
    case MONSTER_GIANT_SKELETON: return 0.92;
    default: return 1.55;
    }
}

static double monster_retreat_speed(int type)
{
    switch (type) {
    case 0: return 0.75;
    case 1: return 1.25;
    case 2: return 1.45;
    case 3: return 1.05;
    case MONSTER_FLYING_HEAD: return 1.18;
    case MONSTER_GIANT_SKELETON: return 0.55;
    default: return 1.0;
    }
}

static double monster_preferred_distance(int type)
{
    switch (type) {
    case 0: return 1.25;
    case 1: return 4.8;
    case 2: return 1.15;
    case 3: return 1.05;
    case MONSTER_FLYING_HEAD: return 5.4;
    case MONSTER_GIANT_SKELETON: return 1.45;
    default: return 1.2;
    }
}

static int monster_uses_projectile(int type)
{
    return type == 1 || type == MONSTER_FLYING_HEAD;
}

static double monster_melee_range(int type)
{
    switch (type) {
    case 1: return 0.78;
    case 2: return 0.90;
    case 3: return 0.82;
    case MONSTER_GIANT_SKELETON: return 1.08;
    default: return 0.80;
    }
}

static int monster_melee_damage(int type)
{
    switch (type) {
    case 0: return 13;
    case 2: return 11;
    case 3: return 8;
    case MONSTER_GIANT_SKELETON: return 19;
    default: return 7;
    }
}

static double monster_front_notice_distance(int type)
{
    switch (type) {
    case 0: return 6.6;
    case 1: return 7.4;
    case 2: return 6.2;
    case 3: return 6.0;
    case MONSTER_FLYING_HEAD: return 8.4;
    case MONSTER_GIANT_SKELETON: return 7.2;
    default: return 7.4;
    }
}

static double monster_close_notice_distance(int type)
{
    switch (type) {
    case 0: return 2.25;
    case 1: return 2.55;
    case 2: return 2.85;
    case 3: return 2.20;
    case MONSTER_FLYING_HEAD: return 3.0;
    case MONSTER_GIANT_SKELETON: return 3.15;
    default: return 2.55;
    }
}

static double monster_fov_dot(int type)
{
    switch (type) {
    case 0: return 0.18;
    case 1: return 0.30;
    case 2: return 0.42;
    case 3: return 0.34;
    case MONSTER_FLYING_HEAD: return 0.24;
    case MONSTER_GIANT_SKELETON: return 0.22;
    default: return 0.30;
    }
}

static int monster_is_behind_player(const Monster *monster, const Camera *cam)
{
    Vec2 player_to_monster = {
        monster->pos.x - cam->pos.x,
        monster->pos.y - cam->pos.y,
    };
    Vec2 dir = vec_norm(player_to_monster);
    return vec_dot(cam->dir, dir) < -0.25;
}

static int monster_can_directly_see_player(const Monster *monster, const Camera *cam, double *out_dist)
{
    Vec2 to_player = {
        cam->pos.x - monster->pos.x,
        cam->pos.y - monster->pos.y,
    };
    double player_dist = vec_len(to_player);
    if (out_dist) {
        *out_dist = player_dist;
    }
    double front_notice = monster->is_boss ? 8.5 : monster_front_notice_distance(monster->type);
    double close_notice = monster->is_boss ? 3.2 : monster_close_notice_distance(monster->type);
    if (player_dist > front_notice || !has_line_of_sight(monster->pos, cam->pos)) {
        if (player_dist > close_notice || !has_line_of_sight(monster->pos, cam->pos)) {
            return 0;
        }
    }

    Vec2 dir_to_player = vec_norm(to_player);
    double facing_dot = vec_dot(vec_norm(monster->facing), dir_to_player);
    if (player_dist <= close_notice) {
        return 1;
    }
    return facing_dot >= (monster->is_boss ? 0.10 : monster_fov_dot(monster->type));
}

static int monster_has_nearby_witness(const GameState *game, int monster_index, const Camera *cam)
{
    const Monster *monster = &game->monsters[monster_index];
    for (int i = 0; i < game->monster_count; ++i) {
        if (i == monster_index) {
            continue;
        }
        const Monster *other = &game->monsters[i];
        if (!other->active || other->ai_state != 2) {
            continue;
        }
        double player_dist = 0.0;
        if (!monster_can_directly_see_player(other, cam, &player_dist)) {
            continue;
        }
        Vec2 diff = {
            other->pos.x - monster->pos.x,
            other->pos.y - monster->pos.y,
        };
        if (vec_len(diff) < 4.8 && has_line_of_sight(monster->pos, other->pos)) {
            return 1;
        }
    }
    return 0;
}

static double monster_shot_cooldown(int type, int index)
{
    double offset = (index % 3) * 0.14;
    switch (type) {
    case 0: return 1.05 + offset * 0.45;
    case 1: return 1.15 + offset;
    case 2: return 0.82 + offset * 0.35;
    case 3: return 0.92 + offset * 0.40;
    case MONSTER_FLYING_HEAD: return 1.35 + offset * 0.50;
    default: return 1.0 + offset;
    }
}

static int monster_projectile_damage(int type)
{
    switch (type) {
    case 1: return 9;
    case MONSTER_FLYING_HEAD: return 12;
    default: return 0;
    }
}

static int count_relics(int mask)
{
    int count = 0;
    for (int i = 0; i < RELIC_COUNT; ++i) {
        if (mask & (1 << i)) {
            count++;
        }
    }
    return count;
}

static void sync_relic_progress(GameState *game)
{
    game->relic_mask &= RELIC_MASK_ALL;
    game->relic_count = count_relics(game->relic_mask);
    game->boss_unlocked = game->relic_mask == RELIC_MASK_ALL;
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

static void level_fill_forest(uint32_t seed)
{
    (void)seed;
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            level_map[y][x] = (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1) ? 8 : 0;
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

static double house_min_x(const House *house)
{
    return house->pos.x - house->half_w;
}

static double house_max_x(const House *house)
{
    return house->pos.x + house->half_w;
}

static double house_min_y(const House *house)
{
    return house->pos.y - house->half_d;
}

static double house_max_y(const House *house)
{
    return house->pos.y + house->half_d;
}

static int house_blocks_area(const House *house, double x, double y, double radius)
{
    if (!house->active) {
        return 0;
    }
    double padding = radius + HOUSE_COLLISION_PADDING;
    return x + padding > house_min_x(house) &&
           x - padding < house_max_x(house) &&
           y + padding > house_min_y(house) &&
           y - padding < house_max_y(house);
}

static int houses_block_area(const GameState *game, double x, double y, double radius)
{
    if (!game || game->generator_mode != GENERATOR_FOREST) {
        return 0;
    }
    for (int i = 0; i < MAX_HOUSES; ++i) {
        if (house_blocks_area(&game->houses[i], x, y, radius)) {
            return 1;
        }
    }
    return 0;
}

static int props_block_area(const GameState *game, double x, double y, double radius)
{
    if (!game) {
        return 0;
    }
    for (int i = 0; i < MAX_PROPS; ++i) {
        const Prop *prop = &game->props[i];
        if (!prop->active || prop->half_w <= 0.02 || prop->half_d <= 0.02) {
            continue;
        }
        if (prop_is_cylinder(prop)) {
            double dx = x - prop->pos.x;
            double dy = y - prop->pos.y;
            double blocked_radius = prop_footprint_radius(prop) + radius;
            if (dx * dx + dy * dy < blocked_radius * blocked_radius) {
                return 1;
            }
        } else if (x + radius > prop->pos.x - prop->half_w &&
            x - radius < prop->pos.x + prop->half_w &&
            y + radius > prop->pos.y - prop->half_d &&
            y - radius < prop->pos.y + prop->half_d) {
            return 1;
        }
    }
    return 0;
}

static int house_overlaps_tile(const House *house, int x, int y, double margin)
{
    if (!house->active) {
        return 0;
    }
    double tile_min_x = x - margin;
    double tile_max_x = x + 1.0 + margin;
    double tile_min_y = y - margin;
    double tile_max_y = y + 1.0 + margin;
    return tile_max_x > house_min_x(house) &&
           tile_min_x < house_max_x(house) &&
           tile_max_y > house_min_y(house) &&
           tile_min_y < house_max_y(house);
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
    for (int i = 0; i < MAX_PORTALS; ++i) {
        if (game->portals[i].active && game->portals[i].x == x && game->portals[i].y == y) {
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
    for (int i = 0; i < MAX_TREES; ++i) {
        if (game->trees[i].active && (int)game->trees[i].pos.x == x && (int)game->trees[i].pos.y == y) {
            return 1;
        }
    }
    for (int i = 0; i < MAX_HOUSES; ++i) {
        if (house_overlaps_tile(&game->houses[i], x, y, 0.12)) {
            return 1;
        }
    }
    for (int i = 0; i < MAX_PROPS; ++i) {
        if (game->props[i].active && (int)game->props[i].pos.x == x && (int)game->props[i].pos.y == y) {
            return 1;
        }
    }
    for (int i = 0; i < MAX_TORCHES; ++i) {
        if (torches[i].pos.x > 0.0 && torches[i].pos.y > 0.0 &&
            (int)torches[i].pos.x == x && (int)torches[i].pos.y == y) {
            return 1;
        }
    }
    return 0;
}

static int dungeon_route_tile(const GameState *game, int x, int y)
{
    if (x <= 0 || x >= MAP_W - 1 || y <= 0 || y >= MAP_H - 1) {
        return 0;
    }
    for (int i = 0; i < MAX_DOORS; ++i) {
        if (game->doors[i].x == x && game->doors[i].y == y) {
            return !game->doors[i].locked;
        }
    }
    for (int i = 0; i < MAX_SECRETS; ++i) {
        if (game->secrets[i].x == x && game->secrets[i].y == y) {
            return 1;
        }
    }
    if (level_map[y][x] == 0) {
        return 1;
    }
    return 0;
}

static void mark_dungeon_reachable_tiles(const GameState *game, unsigned char reachable[MAP_H][MAP_W])
{
    int qx[MAP_W * MAP_H];
    int qy[MAP_W * MAP_H];
    int head = 0;
    int tail = 0;
    memset(reachable, 0, MAP_W * MAP_H);

    int sx = 2;
    int sy = 22;
    if (!dungeon_route_tile(game, sx, sy)) {
        return;
    }

    reachable[sy][sx] = 1;
    qx[tail] = sx;
    qy[tail] = sy;
    tail++;

    while (head < tail) {
        int x = qx[head];
        int y = qy[head];
        head++;
        static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (int i = 0; i < 4; ++i) {
            int nx = x + dirs[i][0];
            int ny = y + dirs[i][1];
            if (nx <= 0 || nx >= MAP_W - 1 || ny <= 0 || ny >= MAP_H - 1 || reachable[ny][nx]) {
                continue;
            }
            if (!dungeon_route_tile(game, nx, ny)) {
                continue;
            }
            reachable[ny][nx] = 1;
            qx[tail] = nx;
            qy[tail] = ny;
            tail++;
        }
    }
}

static int dungeon_tile_reachable_from_entrance(const GameState *game, int x, int y)
{
    unsigned char reachable[MAP_H][MAP_W];
    if (x <= 0 || x >= MAP_W - 1 || y <= 0 || y >= MAP_H - 1) {
        return 0;
    }
    mark_dungeon_reachable_tiles(game, reachable);
    return reachable[y][x] != 0;
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

    if (game->generator_mode == GENERATOR_FOREST) {
        static const Vec2 campfires[] = {
            {6.5, 18.5},
            {11.5, 12.5},
            {17.5, 7.5},
            {19.5, 18.5},
            {8.5, 5.5},
        };
        for (int i = 0; i < (int)(sizeof(campfires) / sizeof(campfires[0])) && count < MAX_TORCHES; ++i) {
            int x = (int)campfires[i].x;
            int y = (int)campfires[i].y;
            if (generated_floor(x, y) && !occupied_spawn_tile(game, x, y)) {
                torches[count++].pos = campfires[i];
            }
        }
        return;
    }

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

static void place_forest_dungeon_portals(GameState *game, LevelRng *rng)
{
    static const int targets[RELIC_COUNT] = {
        GENERATOR_ROOMS,
        GENERATOR_TIGHT,
        GENERATOR_ROOMS,
        GENERATOR_TIGHT,
    };

    for (int i = 0; i < RELIC_COUNT; ++i) {
        Vec2 pos = pick_floor_spot(rng, game, 44.0 + i * 18.0);
        game->portals[i] = (Portal){
            .active = 1,
            .x = (int)pos.x,
            .y = (int)pos.y,
            .target_mode = targets[i],
            .exit_to_forest = 0,
            .relic_index = i,
            .boss_gate = 0,
        };
    }
    Vec2 boss_pos = pick_floor_spot(rng, game, 112.0);
    game->portals[RELIC_COUNT] = (Portal){
        .active = 1,
        .x = (int)boss_pos.x,
        .y = (int)boss_pos.y,
        .target_mode = GENERATOR_BOSS,
        .exit_to_forest = 0,
        .relic_index = -1,
        .boss_gate = 1,
    };
}

static int house_site_clear(const GameState *game, const House *candidate)
{
    if (house_min_x(candidate) < 1.10 || house_max_x(candidate) > MAP_W - 1.10 ||
        house_min_y(candidate) < 1.10 || house_max_y(candidate) > MAP_H - 1.10) {
        return 0;
    }

    double dx = candidate->pos.x - 2.5;
    double dy = candidate->pos.y - 22.5;
    if (dx * dx + dy * dy < 34.0) {
        return 0;
    }

    int min_x = (int)floor(house_min_x(candidate) - 0.08);
    int max_x = (int)floor(house_max_x(candidate) + 0.08);
    int min_y = (int)floor(house_min_y(candidate) - 0.08);
    int max_y = (int)floor(house_max_y(candidate) + 0.08);
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (!generated_floor(x, y) || reserved_dynamic_tile(game, x, y)) {
                return 0;
            }
        }
    }

    for (int i = 0; i < MAX_TORCHES; ++i) {
        if (torches[i].pos.x <= 0.0 && torches[i].pos.y <= 0.0) {
            continue;
        }
        if (house_blocks_area(candidate, torches[i].pos.x, torches[i].pos.y, 0.42)) {
            return 0;
        }
    }

    for (int i = 0; i < MAX_HOUSES; ++i) {
        const House *other = &game->houses[i];
        if (!other->active) {
            continue;
        }
        if (fabs(candidate->pos.x - other->pos.x) < candidate->half_w + other->half_w + 3.2 &&
            fabs(candidate->pos.y - other->pos.y) < candidate->half_d + other->half_d + 3.2) {
            return 0;
        }
    }

    return 1;
}

static void place_forest_houses(GameState *game, LevelRng *rng)
{
    static const Vec2 preferred[] = {
        {8.5, 20.5},
        {16.5, 16.5},
        {20.5, 6.5},
        {10.5, 8.5},
        {19.5, 12.5},
    };
    int count = 0;
    memset(game->houses, 0, sizeof(game->houses));

    for (int i = 0; i < (int)(sizeof(preferred) / sizeof(preferred[0])) && count < MAX_HOUSES; ++i) {
        House candidate = {
            .active = 1,
            .variant = count,
            .pos = preferred[i],
            .half_w = 0.78 + (count % 2) * 0.06,
            .half_d = 0.60 + ((count + 1) % 2) * 0.05,
        };
        if (house_site_clear(game, &candidate)) {
            game->houses[count++] = candidate;
        }
    }

    for (int attempt = 0; attempt < 360 && count < MAX_HOUSES; ++attempt) {
        int x = rng_range(rng, 3, MAP_W - 4);
        int y = rng_range(rng, 3, MAP_H - 4);
        House candidate = {
            .active = 1,
            .variant = count,
            .pos = {x + 0.5, y + 0.5},
            .half_w = 0.76 + (rng_next(rng) % 9u) / 100.0,
            .half_d = 0.56 + (rng_next(rng) % 9u) / 100.0,
        };
        if (house_site_clear(game, &candidate)) {
            game->houses[count++] = candidate;
        }
    }
}

static void place_forest_trees(GameState *game, LevelRng *rng)
{
    int count = 0;
    memset(game->trees, 0, sizeof(game->trees));

    for (int attempt = 0; attempt < 900 && count < 64; ++attempt) {
        int x = rng_range(rng, 2, MAP_W - 3);
        int y = rng_range(rng, 2, MAP_H - 3);
        if (!generated_floor(x, y) || reserved_dynamic_tile(game, x, y) || start_dist2(x, y) < 18.0) {
            continue;
        }

        Vec2 pos = {
            x + 0.24 + (rng_next(rng) % 53u) / 100.0,
            y + 0.24 + (rng_next(rng) % 53u) / 100.0,
        };
        int too_close = 0;
        for (int i = 0; i < count; ++i) {
            double dx = pos.x - game->trees[i].pos.x;
            double dy = pos.y - game->trees[i].pos.y;
            if (dx * dx + dy * dy < 1.05 * 1.05) {
                too_close = 1;
                break;
            }
        }
        if (too_close) {
            continue;
        }

        game->trees[count++] = (Tree){
            .active = 1,
            .variant = (int)(rng_next(rng) % TREE_TYPES),
            .pos = pos,
        };
    }
}

static void place_house_prop(GameState *game,
                             int *count,
                             int type,
                             Vec2 pos,
                             double half_w,
                             double half_d,
                             double height,
                             int loot_type,
                             int loot_amount,
                             int loot_slot,
                             uint32_t loot_mask)
{
    if (*count < 0 || *count >= MAX_PROPS) {
        return;
    }
    game->props[*count] = (Prop){
        .active = 1,
        .type = type,
        .loot_type = loot_type,
        .loot_amount = loot_amount,
        .loot_slot = loot_slot,
        .looted = loot_slot >= 0 && (loot_mask & (1u << loot_slot)),
        .pos = pos,
        .half_w = half_w,
        .half_d = half_d,
        .height = height,
    };
    *count += 1;
}

static void place_house_exit_portal(GameState *game)
{
    level_map[12][7] = WALL_DOOR;
    game->portals[0] = (Portal){
        .active = 1,
        .x = 7,
        .y = 12,
        .target_mode = GENERATOR_FOREST,
        .exit_to_forest = 1,
        .relic_index = -1,
        .boss_gate = 0,
    };
}

static void place_house_torches(int variant)
{
    memset(torches, 0, sizeof(torches));
    torches[0].pos = (Vec2){10.0, 8.12};
    torches[1].pos = (Vec2){14.0, 8.12};
    torches[2].pos = (Vec2){8.12, variant == 1 ? 11.5 : 14.5};
    torches[3].pos = (Vec2){15.88, variant == 2 ? 11.5 : 14.5};
}

static void generate_house_level(GameState *game, uint32_t seed)
{
    (void)seed;
    int variant = game->current_house_variant % 3;
    if (variant < 0) {
        variant = 0;
    }
    uint32_t loot_mask = game->current_house_loot_mask;

    memset(game->props, 0, sizeof(game->props));
    memset(game->items, 0, sizeof(game->items));
    memset(game->doors, 0, sizeof(game->doors));
    memset(game->secrets, 0, sizeof(game->secrets));
    memset(game->portals, 0, sizeof(game->portals));
    memset(game->trees, 0, sizeof(game->trees));
    memset(game->houses, 0, sizeof(game->houses));
    memset(game->projectiles, 0, sizeof(game->projectiles));
    memset(game->particles, 0, sizeof(game->particles));
    memset(game->decals, 0, sizeof(game->decals));
    memset(game->wall_decals, 0, sizeof(game->wall_decals));
    clear_wall_decal_index(game);
    game->monster_count = 0;

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            level_map[y][x] = 4 + ((x + y + variant) & 1);
        }
    }
    carve_room((LevelRoom){8, 8, 8, 9});
    place_house_exit_portal(game);
    place_house_torches(variant);

    int count = 0;
    if (variant == 1) {
        place_house_prop(game, &count, PROP_BED, (Vec2){14.7, 9.0}, 0.58, 0.34, 0.34, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_TABLE, (Vec2){10.2, 12.0}, 0.42, 0.30, 0.42, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CHAIR, (Vec2){11.1, 12.4}, 0.24, 0.22, 0.58, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CHEST, (Vec2){8.7, 8.7}, 0.36, 0.26, 0.42, ITEM_GOLD, 24, 0, loot_mask);
        place_house_prop(game, &count, PROP_CABINET, (Vec2){14.9, 13.8}, 0.34, 0.26, 0.92, ITEM_AMMO, 0, 1, loot_mask);
        place_house_prop(game, &count, PROP_CRATE, (Vec2){12.5, 15.4}, 0.28, 0.28, 0.48, ITEM_HEALTH, 0, 2, loot_mask);
        place_house_prop(game, &count, PROP_BARREL, (Vec2){9.0, 15.6}, 0.26, 0.26, 0.55, ITEM_GOLD, 12, 3, loot_mask);
        place_house_prop(game, &count, PROP_STASH, (Vec2){13.1, 10.1}, 0.36, 0.20, 0.10, ITEM_RAPID, 0, 4, loot_mask);
    } else if (variant == 2) {
        place_house_prop(game, &count, PROP_BED, (Vec2){9.0, 9.0}, 0.58, 0.34, 0.34, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_TABLE, (Vec2){12.6, 12.7}, 0.42, 0.30, 0.42, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CHAIR, (Vec2){11.7, 13.1}, 0.24, 0.22, 0.58, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CABINET, (Vec2){8.7, 14.4}, 0.34, 0.26, 0.92, ITEM_DAMAGE, 0, 0, loot_mask);
        place_house_prop(game, &count, PROP_CHEST, (Vec2){14.8, 8.7}, 0.36, 0.26, 0.42, ITEM_GOLD, 30, 1, loot_mask);
        place_house_prop(game, &count, PROP_CRATE, (Vec2){14.8, 15.4}, 0.28, 0.28, 0.48, ITEM_AMMO, 0, 2, loot_mask);
        place_house_prop(game, &count, PROP_BARREL, (Vec2){10.1, 15.5}, 0.26, 0.26, 0.55, ITEM_HEALTH, 0, 3, loot_mask);
        place_house_prop(game, &count, PROP_STASH, (Vec2){12.0, 9.6}, 0.36, 0.20, 0.10, ITEM_FIREBALL, 0, 4, loot_mask);
    } else {
        place_house_prop(game, &count, PROP_BED, (Vec2){9.0, 9.0}, 0.58, 0.34, 0.34, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_TABLE, (Vec2){12.4, 12.2}, 0.42, 0.30, 0.42, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CHAIR, (Vec2){13.4, 12.6}, 0.24, 0.22, 0.58, -1, 0, -1, loot_mask);
        place_house_prop(game, &count, PROP_CHEST, (Vec2){14.8, 8.7}, 0.36, 0.26, 0.42, ITEM_GOLD, 18, 0, loot_mask);
        place_house_prop(game, &count, PROP_CABINET, (Vec2){8.7, 14.2}, 0.34, 0.26, 0.92, ITEM_AMMO, 0, 1, loot_mask);
        place_house_prop(game, &count, PROP_CRATE, (Vec2){10.4, 15.4}, 0.28, 0.28, 0.48, ITEM_HEALTH, 0, 2, loot_mask);
        place_house_prop(game, &count, PROP_BARREL, (Vec2){14.8, 15.5}, 0.26, 0.26, 0.55, ITEM_GOLD, 10, 3, loot_mask);
        place_house_prop(game, &count, PROP_STASH, (Vec2){11.3, 10.0}, 0.36, 0.20, 0.10, ITEM_DAMAGE, 0, 4, loot_mask);
    }
}

static void place_dungeon_exit_portal(GameState *game)
{
    game->portals[0] = (Portal){
        .active = 1,
        .x = 3,
        .y = 22,
        .target_mode = GENERATOR_FOREST,
        .exit_to_forest = 1,
        .relic_index = -1,
        .boss_gate = 0,
    };
}

static Vec2 pick_relic_guard_spot(const GameState *game, Vec2 relic_pos)
{
    int cx = (int)relic_pos.x;
    int cy = (int)relic_pos.y;
    Vec2 fallback = relic_pos;
    double best_dist = -1.0;

    for (int radius = 1; radius <= 5; ++radius) {
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                if (x < 1 || x >= MAP_W - 1 || y < 1 || y >= MAP_H - 1 || !generated_floor(x, y)) {
                    continue;
                }
                if (occupied_spawn_tile(game, x, y)) {
                    continue;
                }
                double dx = x + 0.5 - relic_pos.x;
                double dy = y + 0.5 - relic_pos.y;
                double dist2 = dx * dx + dy * dy;
                if (dist2 < 1.0 || dist2 > 18.0) {
                    continue;
                }
                if (dist2 > best_dist) {
                    best_dist = dist2;
                    fallback = (Vec2){x + 0.5, y + 0.5};
                }
            }
        }
        if (best_dist > 0.0) {
            return fallback;
        }
    }

    return fallback;
}

static void place_relic_guardian(GameState *game, Vec2 relic_pos)
{
    int index = game->monster_count > 0 ? game->monster_count - 1 : 0;
    if (game->monster_count <= index) {
        game->monster_count = index + 1;
    }

    Monster *monster = &game->monsters[index];
    Vec2 pos = pick_relic_guard_spot(game, relic_pos);
    *monster = (Monster){
        .active = 1,
        .pos = pos,
        .facing = {0.0, -1.0},
        .hp = scale_monster_hp_for_difficulty(monster_max_hp(MONSTER_GIANT_SKELETON), game->difficulty),
        .type = MONSTER_GIANT_SKELETON,
        .shoot_timer = 0.8,
        .target_waypoint = 1,
        .route = index,
        .patrol = {pos, relic_pos},
        .patrol_count = 2,
        .strafe_timer = 0.7,
        .strafe_dir = 1,
        .last_seen = pos,
    };
}

static void place_dungeon_relic(GameState *game)
{
    int relic = game->dungeon_relic_index;
    if (relic < 0 || relic >= RELIC_COUNT || (game->relic_mask & (1 << relic))) {
        return;
    }

    unsigned char reachable[MAP_H][MAP_W];
    mark_dungeon_reachable_tiles(game, reachable);
    Vec2 best = {20.5, 2.5};
    double best_dist = -1.0;
    for (int y = 1; y < MAP_H - 1; ++y) {
        for (int x = 1; x < MAP_W - 1; ++x) {
            if (!generated_floor(x, y) || !reachable[y][x] || occupied_spawn_tile(game, x, y)) {
                continue;
            }
            double dx = x + 0.5 - 2.5;
            double dy = y + 0.5 - 22.5;
            double dist2 = dx * dx + dy * dy;
            if (dist2 > best_dist) {
                best_dist = dist2;
                best = (Vec2){x + 0.5, y + 0.5};
            }
        }
    }
    if (best_dist < 0.0) {
        best = (Vec2){2.5, 22.5};
    }

    game->items[MAX_ITEMS - 1] = (Item){
        .active = 1,
        .type = ITEM_RELIC,
        .relic_index = relic,
        .pos = best,
    };
    place_relic_guardian(game, best);
}

static void spawn_decal(GameState *game, Vec2 pos, int variant, double radius, double life, double angle);

static void place_generated_items(GameState *game, LevelRng *rng)
{
    static const int item_types[MAX_ITEMS] = {
        ITEM_KEY, ITEM_PISTOL, ITEM_RAPID, ITEM_FIREBALL, ITEM_AMMO,
        ITEM_HEALTH, ITEM_RAPID, ITEM_DAMAGE, ITEM_AMMO, ITEM_HEALTH,
        ITEM_AMMO, ITEM_HEALTH, ITEM_FIREBALL, ITEM_AMMO, ITEM_HEALTH,
        ITEM_DAMAGE, ITEM_FIREBALL, ITEM_RAPID, ITEM_AMMO, ITEM_HEALTH,
        ITEM_GOLD, ITEM_SHRINE, ITEM_BONEPILE, ITEM_GOLD, ITEM_BONEPILE,
        ITEM_SHRINE, ITEM_GOLD, ITEM_BONEPILE,
    };
    for (int i = 0; i < MAX_ITEMS; ++i) {
        double min_dist = i <= 1 ? 2.0 : 24.0;
        Vec2 pos = i == 0 ? (Vec2){4.5, 22.5} :
            (i == 1 ? (Vec2){5.5, 22.5} : pick_floor_spot(rng, game, min_dist));
        int payload = -1;
        if (item_types[i] == ITEM_GOLD) {
            payload = 6 + (i % 4) * 4;
        } else if (item_types[i] == ITEM_SHRINE) {
            payload = i % 3;
        }
        game->items[i] = (Item){1, item_types[i], payload, pos};
    }
}

static void place_generated_decals(GameState *game, LevelRng *rng)
{
    int target = game->generator_mode == GENERATOR_FOREST ? 22 : 34;
    int placed = 0;
    for (int attempt = 0; attempt < 320 && placed < target; ++attempt) {
        int x = rng_range(rng, 1, MAP_W - 2);
        int y = rng_range(rng, 1, MAP_H - 2);
        if (!generated_floor(x, y) || occupied_spawn_tile(game, x, y) || start_dist2(x, y) < 10.0) {
            continue;
        }
        Vec2 pos = {
            x + 0.18 + (rng_next(rng) % 65u) / 100.0,
            y + 0.18 + (rng_next(rng) % 65u) / 100.0,
        };
        int variant = (int)(rng_next(rng) % DECAL_COUNT);
        double radius = 0.34 + (rng_next(rng) % 62u) / 100.0;
        if (variant == 3 || variant == 13) {
            radius += 0.18;
        }
        spawn_decal(game, pos, variant, radius, 1200.0, (rng_next(rng) % 628u) / 100.0);
        placed++;
    }
}

static void place_generated_wall_decals(GameState *game, LevelRng *rng)
{
    int target = game->generator_mode == GENERATOR_FOREST ? 14 : 42;
    int count = 0;
    memset(game->wall_decals, 0, sizeof(game->wall_decals));
    clear_wall_decal_index(game);

    for (int attempt = 0; attempt < 700 && count < target && count < MAX_WALL_DECALS; ++attempt) {
        int x = rng_range(rng, 1, MAP_W - 2);
        int y = rng_range(rng, 1, MAP_H - 2);
        if (map_at(x, y) <= 0) {
            continue;
        }

        int side = -1;
        int vertical = generated_floor(x - 1, y) || generated_floor(x + 1, y);
        int horizontal = generated_floor(x, y - 1) || generated_floor(x, y + 1);
        if (vertical && horizontal) {
            side = (rng_next(rng) & 1u) ? 0 : 1;
        } else if (vertical) {
            side = 0;
        } else if (horizontal) {
            side = 1;
        } else {
            continue;
        }
        if (start_dist2(x, y) < 8.0) {
            continue;
        }

        WallDecal *decal = &game->wall_decals[count++];
        decal->active = 1;
        decal->x = x;
        decal->y = y;
        decal->side = side;
        decal->variant = (int)(rng_next(rng) % WALL_DECAL_COUNT);
        decal->u = 0.22 + (rng_next(rng) % 57u) / 100.0;
        decal->v = 0.26 + (rng_next(rng) % 43u) / 100.0;
        decal->width = 0.34 + (rng_next(rng) % 38u) / 100.0;
        decal->height = 0.30 + (rng_next(rng) % 42u) / 100.0;
        decal->strength = 0.42 + (rng_next(rng) % 36u) / 100.0;
        link_wall_decal(game, count - 1);
    }
}

static void apply_forest_relic_escalation(GameState *game)
{
    if (!game || game->generator_mode != GENERATOR_FOREST || game->relic_count <= 0) {
        return;
    }

    int escalation = game->relic_count;
    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }
        int min_hp = scale_monster_hp_for_difficulty(monster_max_hp(monster->type), game->difficulty) + escalation * 2;
        if (monster->hp < min_hp) {
            monster->hp = min_hp;
        }
        monster->shoot_timer *= 0.92;
    }

    LevelRng rng = {LEVEL_TEST_SEED ^ (uint32_t)(game->relic_count * 977u + game->kills * 31u)};
    int to_spawn = escalation + (escalation >= 3 ? 1 : 0);
    for (int i = 0; i < game->monster_count && to_spawn > 0; ++i) {
        Monster *monster = &game->monsters[i];
        if (monster->active) {
            continue;
        }
        memset(monster, 0, sizeof(*monster));
        monster->active = 1;
        monster->pos = pick_floor_spot(&rng, game, 42.0);
        monster->type = escalation >= 3 && (to_spawn & 1) ? MONSTER_FLYING_HEAD : 3;
        monster->hp = scale_monster_hp_for_difficulty(monster_max_hp(monster->type), game->difficulty) + escalation * 2;
        monster->shoot_timer = 0.35 + to_spawn * 0.19;
        monster->target_waypoint = 1;
        monster->route = i;
        monster->facing = (Vec2){0.0, -1.0};
        monster->last_seen = monster->pos;
        monster->patrol[0] = monster->pos;
        monster->patrol[1] = pick_floor_spot(&rng, game, 30.0);
        monster->patrol_count = 2;
        monster->strafe_timer = 0.35 + i * 0.07;
        monster->strafe_dir = (i & 1) ? 1 : -1;
        to_spawn--;
    }
}

static void place_generated_monsters(GameState *game, LevelRng *rng, int boss_room, LevelRoom room)
{
    game->monster_count = MAX_MONSTERS;
    static const int dungeon_types[MAX_MONSTERS] = {1, MONSTER_FLYING_HEAD, 1, 0, 1, 2, 3, MONSTER_FLYING_HEAD, 0, 2};
    static const int forest_types[MAX_MONSTERS] = {2, 3, 1, MONSTER_FLYING_HEAD, 3, 2, 1, 3, MONSTER_FLYING_HEAD, 2};
    const int *monster_types = game->generator_mode == GENERATOR_FOREST ? forest_types : dungeon_types;
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
        monster->hp = scale_monster_hp_for_difficulty(monster_max_hp(monster->type), game->difficulty);
        if (game->generator_mode == GENERATOR_FOREST && game->relic_count > 0) {
            monster->hp += game->relic_count * 2;
        }
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
        monster->attack_anim_timer = 0.0;
        monster->is_boss = boss_room && i == game->monster_count - 1;
        if (monster->is_boss) {
            monster->type = MONSTER_BOSS_BUTCHER;
            monster->hp = scale_monster_hp_for_difficulty(BOSS_HP, game->difficulty);
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

static void carve_forest_level(LevelRng *rng, uint32_t seed)
{
    level_fill_forest(seed);
    (void)rng;
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
    memset(torches, 0, sizeof(torches));
    memset(game->houses, 0, sizeof(game->houses));
    memset(game->props, 0, sizeof(game->props));
    level_fill_walls(seed);

    if (mode == GENERATOR_HOUSE) {
        generate_house_level(game, seed);
        return;
    } else if (mode == GENERATOR_FOREST) {
        carve_forest_level(&rng, seed);
    } else if (mode == GENERATOR_TIGHT) {
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
    } else if (mode != GENERATOR_FOREST) {
        place_generated_doors(game, &rng);
    }
    if (mode == GENERATOR_FOREST) {
        place_forest_dungeon_portals(game, &rng);
        place_generated_torches(game);
        place_forest_houses(game, &rng);
        place_forest_trees(game, &rng);
    } else {
        place_generated_secrets(game, &rng);
        place_generated_torches(game);
    }
    place_generated_items(game, &rng);
    place_generated_decals(game, &rng);
    place_generated_wall_decals(game, &rng);
    place_generated_monsters(game, &rng, mode == GENERATOR_BOSS, boss_room);
    apply_forest_relic_escalation(game);
}

static void init_game_seed(GameState *game, uint32_t seed, int mode)
{
    int house_index = -1;
    int house_variant = 0;
    uint32_t house_loot_mask = 0;
    if (mode == GENERATOR_HOUSE) {
        house_index = game->current_house_index;
        house_variant = game->current_house_variant;
        house_loot_mask = game->current_house_loot_mask;
    }
    memset(game, 0, sizeof(*game));
    active_game = game;
    clear_wall_decal_index(game);
    moon_visibility_cache_ready = 0;
    torch_flicker_cache_ready = 0;
    game->generator_mode = mode;
    game->current_house_index = mode == GENERATOR_HOUSE ? house_index : -1;
    game->current_house_variant = mode == GENERATOR_HOUSE ? house_variant : 0;
    game->current_house_loot_mask = mode == GENERATOR_HOUSE ? house_loot_mask : 0;
    game->difficulty = normalize_difficulty(runtime_difficulty);
    game->trainer = runtime_trainer ? 1 : 0;
    game->player_health = PLAYER_MAX_HEALTH;
    game->ammo = game->trainer ? MAX_PISTOL_AMMO : START_AMMO;
    game->fireball_ammo = game->trainer ? MAX_FIREBALL_AMMO : START_FIREBALL_AMMO;
    game->selected_weapon = WEAPON_KNIFE;
    game->pistol_unlocked = 0;
    game->fireball_unlocked = 0;
    game->dungeon_relic_index = -1;
    game->help_timer = mode == GENERATOR_HOUSE ? 0.0 : 7.0;
    sync_relic_progress(game);
    generate_level(game, seed, mode);
    if (game->trainer) {
        game->relic_mask = RELIC_MASK_ALL;
        game->shotgun_unlocked = 1;
        game->pistol_unlocked = 1;
        game->fireball_unlocked = 1;
        sync_relic_progress(game);
    }
    set_active_music_track(mode == GENERATOR_FOREST ? MUSIC_TRACK_FOREST :
                           (mode == GENERATOR_BOSS ? MUSIC_TRACK_TOCCATA : MUSIC_TRACK_DIES_IRAE));
}

static void init_game(GameState *game)
{
    init_game_seed(game, LEVEL_TEST_SEED, GENERATOR_ROOMS);
}

static int can_occupy(double x, double y, double radius)
{
    if (map_at((int)(x - radius), (int)(y - radius)) != 0 ||
        map_at((int)(x + radius), (int)(y - radius)) != 0 ||
        map_at((int)(x - radius), (int)(y + radius)) != 0 ||
        map_at((int)(x + radius), (int)(y + radius)) != 0) {
        return 0;
    }
    if (houses_block_area(active_game, x, y, radius)) {
        return 0;
    }
    if (props_block_area(active_game, x, y, radius)) {
        return 0;
    }
    return 1;
}

static int can_move(double x, double y)
{
    if (!can_occupy(x, y, 0.18)) {
        return 0;
    }
    for (int i = 0; i < MAX_TORCHES; ++i) {
        if (torches[i].pos.x <= 0.0 && torches[i].pos.y <= 0.0) {
            continue;
        }
        double dx = x - torches[i].pos.x;
        double dy = y - torches[i].pos.y;
        if (dx * dx + dy * dy < 0.34 * 0.34) {
            return 0;
        }
    }
    if (active_game && active_game->generator_mode == GENERATOR_FOREST) {
        for (int i = 0; i < MAX_TREES; ++i) {
            const Tree *tree = &active_game->trees[i];
            if (!tree->active) {
                continue;
            }
            double dx = x - tree->pos.x;
            double dy = y - tree->pos.y;
            if (dx * dx + dy * dy < TREE_COLLISION_RADIUS * TREE_COLLISION_RADIUS) {
                return 0;
            }
        }
    }
    return 1;
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

static void update_monster(GameState *game, int monster_index, const Camera *cam, double dt)
{
    Monster *monster = &game->monsters[monster_index];
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
    double player_dist = 0.0;
    int sees_player = monster_can_directly_see_player(monster, cam, &player_dist);
    int nearby_witness = !sees_player && monster_has_nearby_witness(game, monster_index, cam);

    if (sees_player) {
        monster->ai_state = player_dist < 6.5 ? 2 : 1;
        monster->last_seen = cam->pos;
        monster->alert_timer = 3.2;
        if (monster->facing_lock <= 0.0) {
            monster->facing = vec_norm(to_player);
        }
    } else if (nearby_witness) {
        monster->ai_state = 1;
        monster->last_seen = cam->pos;
        if (monster->alert_timer < 1.8) {
            monster->alert_timer = 1.8;
        }
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
        double preferred = monster->is_boss ? 3.4 : monster_preferred_distance(monster->type);
        double speed_scale = monster_is_behind_player(monster, cam) ? 0.46 : 1.0;
        if (player_dist > preferred + 0.7) {
            double attack_speed = monster->is_boss ? 0.95 : monster_attack_speed(monster->type);
            move_monster_toward(monster, cam->pos, attack_speed * speed_scale, dt);
        } else if (player_dist < preferred - 0.8) {
            Vec2 away = {
                monster->pos.x - dir_to_player.x,
                monster->pos.y - dir_to_player.y,
            };
            move_monster_toward(monster, away, monster_retreat_speed(monster->type) * speed_scale, dt);
        } else {
            double strafe_speed = (monster->is_boss ? 0.50 : (monster->type == 2 ? 1.55 : (monster->type == 0 ? 0.72 : 1.05))) * speed_scale;
            Vec2 side = {
                -dir_to_player.y * monster->strafe_dir * strafe_speed * dt,
                dir_to_player.x * monster->strafe_dir * strafe_speed * dt,
            };
            move_monster_by(monster, side);
        }
        if (monster->facing_lock <= 0.0) {
            monster->facing = dir_to_player;
        }
        return;
    }

    if (monster->ai_state == 1) {
        double speed_scale = monster_is_behind_player(monster, cam) ? 0.42 : (nearby_witness ? 0.58 : 0.72);
        if (move_monster_toward(monster, monster->last_seen, monster_chase_speed(monster->type) * speed_scale, dt)) {
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

static void spawn_decal(GameState *game, Vec2 pos, int variant, double radius, double life, double angle)
{
    int slot = -1;
    double weakest_life = 1e30;
    for (int i = 0; i < MAX_DECALS; ++i) {
        Decal *decal = &game->decals[i];
        if (!decal->active) {
            slot = i;
            break;
        }
        if (decal->life < weakest_life) {
            weakest_life = decal->life;
            slot = i;
        }
    }
    if (slot < 0) {
        return;
    }
    if (radius < 0.08) {
        radius = 0.08;
    }
    if (life < 0.25) {
        life = 0.25;
    }

    Decal *decal = &game->decals[slot];
    memset(decal, 0, sizeof(*decal));
    decal->active = 1;
    decal->type = variant;
    decal->variant = variant % DECAL_COUNT;
    if (decal->variant < 0) {
        decal->variant = 0;
    }
    decal->pos = pos;
    decal->radius = radius;
    decal->life = life;
    decal->max_life = life;
    decal->angle = angle;
}

static void spawn_explosion(GameState *game, Vec2 pos, double radius)
{
    play_sfx(SFX_EXPLOSION, 0.52);
    game->screen_shake_timer = SCREEN_SHAKE_TIME;
    double shake = clamp01(radius * 0.10) * 0.20;
    if (shake > game->screen_shake_strength) {
        game->screen_shake_strength = shake;
    }
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

    spawn_decal(game, pos, 2 + (game->kills % 2) * 10, radius * 0.92, 42.0, game->time * 0.73);

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < MAX_PARTICLES; ++j) {
            Particle *p = &game->particles[j];
            if (!p->active) {
                double a = i * (M_PI * 2.0 / 10.0);
                double speed = 0.28 + (i % 3) * 0.10;
                p->active = 1;
                p->type = PARTICLE_SMOKE;
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

static void spawn_gold_drop(GameState *game, Vec2 pos, int amount)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            item->active = 1;
            item->type = ITEM_GOLD;
            item->relic_index = amount;
            item->pos = pos;
            return;
        }
    }
}

static void damage_monster(GameState *game, Monster *monster, int damage, Vec2 source)
{
    if (!monster->active) {
        return;
    }

    monster->hp -= damage;
    monster->pain_timer = monster->is_boss ? 0.32 : 0.18;
    monster->alert_timer = 5.0;
    Vec2 push = vec_norm((Vec2){
        monster->pos.x - source.x,
        monster->pos.y - source.y,
    });
    if (push.x != 0.0 || push.y != 0.0) {
        move_monster_by(monster, (Vec2){push.x * 0.08, push.y * 0.08});
    }
    spawn_decal(
        game,
        monster->pos,
        monster->hp <= 0 ? 15 : (damage > 3 ? 0 : 1),
        monster->is_boss ? 1.15 : (0.38 + damage * 0.05),
        monster->hp <= 0 ? 55.0 : 34.0,
        atan2(push.y, push.x));

    if (monster->hp <= 0) {
        int boss_killed = monster->is_boss && game->generator_mode == GENERATOR_BOSS;
        monster->active = 0;
        game->kills += 1;
        play_sfx(SFX_DEATH, 0.55);
        spawn_explosion(game, monster->pos, boss_killed ? 1.35 : 0.80);
        spawn_gold_drop(game, monster->pos, monster->is_boss ? 70 : (6 + monster->type * 4));
        if (boss_killed) {
            game->victory = 1;
        }
    } else {
        play_sfx(SFX_HURT, 0.42);
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
    if (!monster_uses_projectile(monster->type) && !monster->is_boss) {
        return;
    }

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
            p->damage = monster->is_boss ? 18 : monster_projectile_damage(monster->type);
            p->radius = monster->is_boss ? 1.0 : 0.0;
            p->pos = (Vec2){
                monster->pos.x + dir.x * 0.55,
                monster->pos.y + dir.y * 0.55,
            };
            double speed = monster->is_boss ? 3.15 : 4.2;
            p->vel = (Vec2){dir.x * speed, dir.y * speed};
            p->life = monster->is_boss ? 3.0 : 2.2;
            play_sfx(SFX_FIREBALL, monster->is_boss ? 0.38 : 0.24);
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

        if (p->life <= 0.0 ||
            map_at((int)p->pos.x, (int)p->pos.y) != 0 ||
            houses_block_area(game, p->pos.x, p->pos.y, 0.08) ||
            props_block_area(game, p->pos.x, p->pos.y, 0.08)) {
            if (p->type == PROJECTILE_PLAYER_FIREBALL) {
                explode_player_fireball(game, old_pos, p->damage, p->radius);
            }
            p->active = 0;
            continue;
        }

        if (p->owner == PROJECTILE_OWNER_ENEMY && !game->victory && !game->game_over) {
            double dx = p->pos.x - cam->pos.x;
            double dy = p->pos.y - cam->pos.y;
            if (dx * dx + dy * dy < 0.18 && game->hit_flash <= 0.0) {
                p->active = 0;
                apply_player_damage_from(game, p->damage, p->pos, cam->pos, p->radius > 0.5 ? 0.22 : 0.10);
                play_sfx(SFX_HURT, 0.44);
            }
        } else if (p->owner == PROJECTILE_OWNER_PLAYER) {
            for (int m = 0; m < game->monster_count; ++m) {
                Monster *monster = &game->monsters[m];
                if (!monster->active) {
                    continue;
                }
                double dx = p->pos.x - monster->pos.x;
                double dy = p->pos.y - monster->pos.y;
                double hit_radius2 = monster->is_boss ? 1.05 * 1.05 : 0.36;
                if (dx * dx + dy * dy < hit_radius2) {
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
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        break;
    case ITEM_AMMO:
        game->ammo += 18;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
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
    case ITEM_PISTOL:
        game->pistol_unlocked = 1;
        game->selected_weapon = WEAPON_PISTOL;
        game->ammo += 12;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        break;
    case ITEM_GOLD:
        game->gold += item->relic_index > 0 ? item->relic_index : 5;
        if (game->gold > 999) {
            game->gold = 999;
        }
        break;
    case ITEM_SHRINE:
        if (item->relic_index == 0) {
            game->player_health += 70;
            if (game->player_health > player_max_health(game)) {
                game->player_health = player_max_health(game);
            }
        } else if (item->relic_index == 1) {
            game->damage_timer = 18.0;
            game->rapid_timer = 10.0;
        } else {
            game->fireball_unlocked = 1;
            game->fireball_ammo += 4;
            if (game->fireball_ammo > MAX_FIREBALL_AMMO) {
                game->fireball_ammo = MAX_FIREBALL_AMMO;
            }
        }
        play_sfx(SFX_SHRINE, 0.44);
        break;
    case ITEM_RELIC:
        if (item->relic_index >= 0 && item->relic_index < RELIC_COUNT) {
            game->relic_mask |= 1 << item->relic_index;
            sync_relic_progress(game);
            if (saved_forest.valid) {
                saved_forest.game.relic_mask = game->relic_mask;
                saved_forest.game.relic_count = game->relic_count;
                saved_forest.game.boss_unlocked = game->boss_unlocked;
                saved_forest.game.relic_flash = 1.45;
                saved_forest.game.relic_notice_count = game->relic_count;
            }
            game->relic_flash = 1.45;
            game->relic_notice_count = game->relic_count;
        }
        break;
    default:
        game->keys += 1;
        break;
    }

    game->pickup_flash = 0.22;
    if (item->type == ITEM_RELIC) {
        play_sfx(SFX_RELIC, 0.72);
    } else {
        play_sfx(SFX_PICKUP, item->type == ITEM_KEY ? 0.52 : 0.44);
    }
    item->active = 0;
}

static void update_items(GameState *game, const Camera *cam)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }
        if (item->type == ITEM_BONEPILE) {
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
    if (weapon == WEAPON_PISTOL && !game->pistol_unlocked) {
        return;
    }
    if (weapon == WEAPON_FIREBALL && !game->fireball_unlocked) {
        return;
    }
    if (weapon == WEAPON_SHOTGUN && !game->shotgun_unlocked) {
        return;
    }
    game->selected_weapon = weapon;
}

static int weapon_available(const GameState *game, int weapon)
{
    if (weapon == WEAPON_KNIFE) return 1;
    if (weapon == WEAPON_PISTOL) return game->pistol_unlocked;
    if (weapon == WEAPON_FIREBALL) return game->fireball_unlocked;
    if (weapon == WEAPON_SHOTGUN) return game->shotgun_unlocked;
    return 0;
}

static void cycle_weapon(GameState *game, int dir)
{
    static const int order[] = {WEAPON_KNIFE, WEAPON_PISTOL, WEAPON_SHOTGUN, WEAPON_FIREBALL};
    int count = (int)(sizeof(order) / sizeof(order[0]));
    int current = 0;
    for (int i = 0; i < count; ++i) {
        if (order[i] == game->selected_weapon) {
            current = i;
            break;
        }
    }
    for (int step = 1; step <= count; ++step) {
        int next = (current + dir * step + count * 4) % count;
        if (weapon_available(game, order[next])) {
            game->selected_weapon = order[next];
            return;
        }
    }
}

static int portal_matches(const Portal *portal, int tx, int ty, int px, int py)
{
    if (!portal->active) {
        return 0;
    }
    if ((portal->x == tx && portal->y == ty) ||
        (portal->x == px && portal->y == py)) {
        return 1;
    }

    double dx = px + 0.5 - (portal->x + 0.5);
    double dy = py + 0.5 - (portal->y + 0.5);
    return dx * dx + dy * dy <= 1.75 * 1.75;
}

static uint8_t prompt_glyph(char c, int row)
{
    return prompt_font_glyph(c, row);
}

static void draw_prompt_text(int x, int y, const char *text, uint32_t color)
{
    int cx = x;
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            cx += 6;
            continue;
        }
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = prompt_glyph(*p, row);
            for (int col = 0; col < 5; ++col) {
                if (bits & (1u << (4 - col))) {
                    fill_rect(cx + col * 2, y + row * 2, 2, 2, color);
                }
            }
        }
        cx += 12;
    }
}

static int prompt_text_width(const char *text)
{
    int width = 0;
    for (const char *p = text; *p; ++p) {
        width += *p == ' ' ? 6 : 12;
    }
    return width > 0 ? width - 2 : 0;
}

static void blend_rect(int x, int y, int w, int h, uint32_t color, double amount)
{
    amount = clamp01(amount);
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            if (xx >= 0 && xx < SCREEN_W && yy >= 0 && yy < SCREEN_H) {
                int idx = yy * SCREEN_W + xx;
                framebuffer[idx] = mix_color(framebuffer[idx], color, amount);
            }
        }
    }
}

static void render_fps_overlay(double fps, double frame_ms, int quality, int effects)
{
    char fps_text[16];
    char ms_text[16];
    const char *quality_text = quality == RENDER_QUALITY_FAST ? "FAST" : "PBR";
    const char *effects_text = render_effects_menu_text(effects);
    int fps_int = (int)(fps + 0.5);
    int ms_int = (int)(frame_ms + 0.5);
    if (fps_int < 0) fps_int = 0;
    if (fps_int > 999) fps_int = 999;
    if (ms_int < 0) ms_int = 0;
    if (ms_int > 999) ms_int = 999;
    snprintf(fps_text, sizeof(fps_text), "FPS %d", fps_int);
    snprintf(ms_text, sizeof(ms_text), "MS %d", ms_int);

    int w = prompt_text_width(fps_text);
    int ms_w = prompt_text_width(ms_text);
    if (ms_w > w) {
        w = ms_w;
    }
    int quality_w = prompt_text_width(quality_text);
    if (quality_w > w) {
        w = quality_w;
    }
    int effects_w = prompt_text_width(effects_text);
    if (effects_w > w) {
        w = effects_w;
    }
    blend_rect(4, 4, w + 10, 70, rgb(0, 0, 0), 0.52);
    draw_prompt_text(9, 9, fps_text, rgb(230, 226, 190));
    draw_prompt_text(9, 25, ms_text, rgb(176, 196, 174));
    draw_prompt_text(9, 41, quality_text, quality == RENDER_QUALITY_FAST ? rgb(238, 178, 54) : rgb(196, 188, 176));
    draw_prompt_text(9, 57, effects_text, effects != RENDER_EFFECTS_OFF ? rgb(238, 178, 54) : rgb(156, 166, 154));
}

static void draw_timing_line(int y, const char *label, double ms, uint32_t color)
{
    char text[32];
    if (ms > 99.9) {
        ms = 99.9;
    }
    snprintf(text, sizeof(text), "%s %.1f", label, ms);
    draw_prompt_text(9, y, text, color);
}

static void render_timing_overlay(const RenderProfile *profile)
{
    blend_rect(4, 78, 142, 116, rgb(0, 0, 0), 0.54);
    draw_timing_line(83, "FLOOR", profile->floor_ms, rgb(230, 226, 190));
    draw_timing_line(99, "WALL", profile->wall_ms, rgb(230, 226, 190));
    draw_timing_line(115, "SPRITE", profile->sprite_ms, rgb(210, 218, 184));
    draw_timing_line(131, "FOG", profile->fog_ms, rgb(176, 196, 174));
    draw_timing_line(147, "BLOOM", profile->bloom_ms, rgb(218, 176, 130));
    draw_timing_line(163, "POST", profile->post_ms, rgb(196, 188, 176));
    draw_timing_line(179, "TOTAL", profile->total_ms, rgb(238, 210, 146));
}

static const Portal *active_portal_prompt(const GameState *game, const Camera *cam)
{
    int tx = (int)(cam->pos.x + cam->dir.x * 0.95);
    int ty = (int)(cam->pos.y + cam->dir.y * 0.95);
    int px = (int)cam->pos.x;
    int py = (int)cam->pos.y;

    for (int i = 0; i < MAX_PORTALS; ++i) {
        const Portal *portal = &game->portals[i];
        if (portal_matches(portal, tx, ty, px, py)) {
            return portal;
        }
    }
    return NULL;
}

static const Door *active_door_prompt(const GameState *game, const Camera *cam)
{
    int tx = (int)(cam->pos.x + cam->dir.x * 0.95);
    int ty = (int)(cam->pos.y + cam->dir.y * 0.95);

    for (int i = 0; i < MAX_DOORS; ++i) {
        const Door *door = &game->doors[i];
        if (door->x == tx && door->y == ty && !door->open && !door->opening) {
            return door;
        }
    }
    return NULL;
}

static const House *active_house_prompt(const GameState *game, const Camera *cam, int *out_index)
{
    if (!game || game->generator_mode != GENERATOR_FOREST) {
        return NULL;
    }
    Vec2 focus = {
        cam->pos.x + cam->dir.x * 0.82,
        cam->pos.y + cam->dir.y * 0.82,
    };
    const House *best = NULL;
    int best_index = -1;
    double best_dist = 1e30;
    for (int i = 0; i < MAX_HOUSES; ++i) {
        const House *house = &game->houses[i];
        if (!house->active) {
            continue;
        }
        double door_x = house_min_x(house) - 0.12;
        double door_y = house->pos.y;
        double dx = focus.x - door_x;
        double dy = focus.y - door_y;
        double dist2 = dx * dx + dy * dy;
        if (dist2 > 1.05 * 1.05) {
            continue;
        }
        if (cam->pos.x > house_min_x(house) - 0.05 || cam->dir.x < 0.18) {
            continue;
        }
        if (dist2 < best_dist) {
            best_dist = dist2;
            best = house;
            best_index = i;
        }
    }
    if (out_index) {
        *out_index = best_index;
    }
    return best;
}

static int is_merchant_house_index(int house_index)
{
    return house_index == 0;
}

static const House *active_merchant_house_prompt(const GameState *game, const Camera *cam)
{
    int house_index = -1;
    const House *house = active_house_prompt(game, cam, &house_index);
    if (!house || !is_merchant_house_index(house_index)) {
        return NULL;
    }
    return house;
}

static int active_prop_index(const GameState *game, const Camera *cam)
{
    if (!game || game->generator_mode != GENERATOR_HOUSE) {
        return -1;
    }
    Vec2 focus = {
        cam->pos.x + cam->dir.x * 0.88,
        cam->pos.y + cam->dir.y * 0.88,
    };
    int best = -1;
    double best_dist = 1e30;
    for (int i = 0; i < MAX_PROPS; ++i) {
        const Prop *prop = &game->props[i];
        if (!prop->active || prop->loot_slot < 0 || prop->looted) {
            continue;
        }
        double dist2;
        if (prop_is_cylinder(prop)) {
            double dx = focus.x - prop->pos.x;
            double dy = focus.y - prop->pos.y;
            double dist = fmax(sqrt(dx * dx + dy * dy) - prop_footprint_radius(prop), 0.0);
            dist2 = dist * dist;
        } else {
            double dx = fmax(fabs(focus.x - prop->pos.x) - prop->half_w, 0.0);
            double dy = fmax(fabs(focus.y - prop->pos.y) - prop->half_d, 0.0);
            dist2 = dx * dx + dy * dy;
        }
        if (dist2 > 0.72 * 0.72) {
            continue;
        }
        Vec2 to_prop = {prop->pos.x - cam->pos.x, prop->pos.y - cam->pos.y};
        if (vec_dot(vec_norm(to_prop), cam->dir) < 0.08) {
            continue;
        }
        if (dist2 < best_dist) {
            best_dist = dist2;
            best = i;
        }
    }
    return best;
}

static void render_interaction_label(const char *label)
{
    int w = prompt_text_width(label);
    int x = SCREEN_W / 2 - w / 2;
    int y = SCREEN_H - 40;
    blend_rect(x - 7, y - 5, w + 14, 24, rgb(4, 4, 5), 0.34);
    blend_rect(x - 5, y - 3, w + 10, 20, rgb(62, 44, 24), 0.20);
    draw_prompt_text(x + 1, y + 1, label, rgb(74, 56, 36));
    draw_prompt_text(x, y, label, rgb(220, 202, 150));
}

static void render_interaction_prompt(const Camera *cam, const GameState *game)
{
    const Portal *portal = active_portal_prompt(game, cam);
    if (portal) {
        if (portal->boss_gate && !game->boss_unlocked) {
            render_interaction_label("BRAK RELIKWII");
        } else {
            render_interaction_label(portal->exit_to_forest ? "F WYJSCIE" : "F WEJSCIE");
        }
        return;
    }

    int house_index = -1;
    if (active_house_prompt(game, cam, &house_index)) {
        if (is_merchant_house_index(house_index)) {
            render_interaction_label("E HANDEL");
            return;
        }
        render_interaction_label("F WEJDZ");
        return;
    }

    int prop_index = active_prop_index(game, cam);
    if (prop_index >= 0) {
        render_interaction_label("F PRZESZUKAJ");
        return;
    }

    const Door *door = active_door_prompt(game, cam);
    if (door) {
        if (door->locked && game->keys <= 0) {
            render_interaction_label("BRAK KLUCZA");
        } else if (door->locked) {
            render_interaction_label("F OTWORZ KLUCZEM");
        } else {
            render_interaction_label("F OTWORZ");
        }
    }
}

static void render_centered_prompt_line_styled(int y, const char *text, uint32_t color, int underline, int strike)
{
    int text_w = prompt_text_width(text);
    int x = SCREEN_W / 2 - text_w / 2;
    draw_prompt_text(x + 1, y + 1, text, rgb(18, 14, 12));
    draw_prompt_text(x, y, text, color);
    if (underline) {
        fill_rect(x, y + 16, text_w, 1, color);
    }
    if (strike) {
        fill_rect(x, y + 8, text_w, 1, rgb(126, 92, 78));
    }
}

static void render_centered_prompt_line(int y, const char *text, uint32_t color)
{
    render_centered_prompt_line_styled(y, text, color, 0, 0);
}

static const char *merchant_shop_item_name(int item)
{
    switch (item) {
    case SHOP_ITEM_AMMO: return "AMUNICJA";
    case SHOP_ITEM_HEALTH: return "APTECZKA";
    case SHOP_ITEM_MAX_HP: return "MAX HP";
    case SHOP_ITEM_DAMAGE: return "OBRAZENIA";
    case SHOP_ITEM_AMMO_CAP: return "LADOWNICA";
    case SHOP_ITEM_SHOTGUN: return "SHOTGUN";
    case SHOP_ITEM_EXIT: return "WYJDZ";
    default: return "";
    }
}

static int merchant_shop_item_price(int item)
{
    switch (item) {
    case SHOP_ITEM_AMMO: return SHOP_AMMO_PRICE;
    case SHOP_ITEM_HEALTH: return SHOP_HEALTH_PRICE;
    case SHOP_ITEM_MAX_HP: return SHOP_MAX_HP_PRICE;
    case SHOP_ITEM_DAMAGE: return SHOP_DAMAGE_PRICE;
    case SHOP_ITEM_AMMO_CAP: return SHOP_AMMO_CAP_PRICE;
    case SHOP_ITEM_SHOTGUN: return SHOP_SHOTGUN_PRICE;
    default: return 0;
    }
}

static int merchant_shop_item_full(const GameState *game, int item)
{
    switch (item) {
    case SHOP_ITEM_AMMO: return game->ammo >= pistol_ammo_cap(game);
    case SHOP_ITEM_HEALTH: return game->player_health >= player_max_health(game);
    case SHOP_ITEM_MAX_HP: return game->max_health_upgrades >= MAX_HEALTH_UPGRADES;
    case SHOP_ITEM_DAMAGE: return game->damage_upgrades >= MAX_DAMAGE_UPGRADES;
    case SHOP_ITEM_AMMO_CAP: return game->ammo_cap_upgrades >= MAX_AMMO_CAP_UPGRADES;
    case SHOP_ITEM_SHOTGUN: return game->shotgun_unlocked;
    default: return 0;
    }
}

static void render_merchant_shop_row(const GameState *game, int item, int selected, int y)
{
    char line[64];
    uint32_t color = selected ? rgb(246, 224, 158) : rgb(204, 188, 142);
    int disabled = 0;

    if (item == SHOP_ITEM_EXIT) {
        snprintf(line, sizeof(line), "%s", merchant_shop_item_name(item));
    } else {
        int price = merchant_shop_item_price(item);
        disabled = !can_buy_merchant_shop_item(game, item);
        if (merchant_shop_item_full(game, item)) {
            snprintf(line, sizeof(line), "%s  PELNE", merchant_shop_item_name(item));
        } else {
            snprintf(line, sizeof(line), "%s  %d ZL", merchant_shop_item_name(item), price);
        }
    }

    if (disabled) {
        color = selected ? rgb(168, 124, 92) : rgb(112, 96, 78);
    }

    int x = SCREEN_W / 2 - 118;
    int w = 236;
    if (selected) {
        fill_rect(x, y - 6, w, 24, rgb(88, 58, 30));
        fill_rect(x + 2, y - 4, w - 4, 20, rgb(38, 28, 18));
    }
    draw_prompt_text(x + 14, y + 1, line, rgb(18, 12, 8));
    draw_prompt_text(x + 13, y, line, color);
}

static void render_merchant_shop_screen(const GameState *game, int selected)
{
    char gold_text[32];
    char ammo_text[32];
    char health_text[32];
    int x = SCREEN_W / 2 - 158;
    int y = SCREEN_H / 2 - 184;
    int w = 316;
    int h = 348;

    blend_rect(0, 0, SCREEN_W, SCREEN_H, rgb(0, 0, 0), 0.64);
    fill_rect(x, y, w, h, rgb(12, 9, 7));
    fill_rect(x + 3, y + 3, w - 6, h - 6, rgb(58, 38, 22));
    fill_rect(x + 9, y + 9, w - 18, h - 18, rgb(18, 14, 10));
    fill_rect(x + 18, y + 50, w - 36, 1, rgb(160, 106, 42));
    fill_rect(x + 18, y + h - 46, w - 36, 1, rgb(86, 58, 34));

    render_centered_prompt_line(y + 17, "HANDLARZ", rgb(246, 224, 158));
    snprintf(gold_text, sizeof(gold_text), "ZLOTO %d", game->gold);
    snprintf(ammo_text, sizeof(ammo_text), "AMMO %d/%d", game->ammo, pistol_ammo_cap(game));
    snprintf(health_text, sizeof(health_text), "HP %d/%d", game->player_health, player_max_health(game));
    draw_prompt_text(x + 28, y + 62, gold_text, rgb(238, 178, 54));
    draw_prompt_text(x + 28, y + 82, ammo_text, rgb(206, 190, 150));
    draw_prompt_text(x + 168, y + 82, health_text, rgb(206, 190, 150));

    for (int item = 0; item < SHOP_ITEM_COUNT; ++item) {
        render_merchant_shop_row(game, item, selected == item, y + 114 + item * 30);
    }
    render_centered_prompt_line(y + h - 28, "ENTER KUP  ESC WYJDZ", rgb(142, 126, 94));
}

static void render_victory_screen(void)
{
    int w = 304;
    int h = 132;
    int x = SCREEN_W / 2 - w / 2;
    int y = SCREEN_H / 2 - h / 2 - 8;
    blend_rect(0, 0, SCREEN_W, SCREEN_H, rgb(0, 0, 0), 0.42);
    blend_rect(x, y, w, h, rgb(6, 5, 4), 0.86);
    blend_rect(x + 3, y + 3, w - 6, h - 6, rgb(78, 46, 24), 0.32);
    fill_rect(x + 20, y + 36, w - 40, 1, rgb(188, 130, 48));
    fill_rect(x + 20, y + h - 34, w - 40, 1, rgb(104, 72, 42));
    render_centered_prompt_line(y + 14, "GRATULACJE", rgb(246, 224, 158));
    render_centered_prompt_line(y + 48, "BOSS POKONANY", rgb(238, 178, 54));
    render_centered_prompt_line(y + 72, "DIOOM UKONCZONY", rgb(218, 202, 160));
    render_centered_prompt_line(y + 102, "R RESTART ESC MENU", rgb(150, 132, 100));
}

static void render_help_overlay(const GameState *game)
{
    if (game->help_timer <= 0.0 && !game->show_help) {
        return;
    }

    int x = SCREEN_W / 2 - 74;
    int y = 20;
    int w = 148;
    int h = 92;
    blend_rect(x, y, w, h, rgb(4, 5, 6), 0.42);
    blend_rect(x + 2, y + 2, w - 4, h - 4, rgb(42, 30, 24), 0.18);
    int relics_done = game->relic_mask == RELIC_MASK_ALL;
    int boss_done = game->victory;
    render_centered_prompt_line(y + 8, "CEL", rgb(238, 214, 146));
    render_centered_prompt_line(y + 26, "SZUKAJ", rgb(218, 202, 160));
    render_centered_prompt_line_styled(y + 44, "4 RELIKWIE", relics_done ? rgb(128, 116, 96) : rgb(238, 218, 156), !relics_done, relics_done);
    render_centered_prompt_line_styled(y + 62, "BRAMA BOSSA", boss_done ? rgb(128, 116, 96) : (relics_done ? rgb(238, 218, 156) : rgb(160, 142, 116)), relics_done && !boss_done, boss_done);
    render_centered_prompt_line(y + 78, "H HELP", rgb(158, 142, 108));
}

enum {
    MENU_PAGE_MAIN = 0,
    MENU_PAGE_SETTINGS,
    MENU_PAGE_SAVE,
    MENU_PAGE_LOAD
};

enum {
    MAIN_MENU_ITEM_PLAY = 0,
    MAIN_MENU_ITEM_RESTART,
    MAIN_MENU_ITEM_SAVE,
    MAIN_MENU_ITEM_LOAD,
    MAIN_MENU_ITEM_SETTINGS,
    MAIN_MENU_ITEM_EXIT,
    MAIN_MENU_ITEM_COUNT
};

enum {
    SETTINGS_MENU_ITEM_DIFFICULTY = 0,
    SETTINGS_MENU_ITEM_QUALITY,
    SETTINGS_MENU_ITEM_POST,
    SETTINGS_MENU_ITEM_SFX_VOLUME,
    SETTINGS_MENU_ITEM_MUSIC_VOLUME,
    SETTINGS_MENU_ITEM_FULLSCREEN,
    SETTINGS_MENU_ITEM_BACK,
    SETTINGS_MENU_ITEM_COUNT
};

#define SLOT_MENU_ITEM_BACK SAVEGAME_SLOT_COUNT
#define SLOT_MENU_ITEM_COUNT (SAVEGAME_SLOT_COUNT + 1)

static void save_slot_menu_label(int slot, char *out, size_t out_size);

static void render_menu_item(int y, const char *text, int selected, uint32_t color)
{
    int item_w = 252;
    int item_h = 26;
    int item_x = SCREEN_W / 2 - item_w / 2;
    int text_w = prompt_text_width(text);
    int text_x = SCREEN_W / 2 - text_w / 2;
    if (selected) {
        blend_rect(item_x, y - 5, item_w, item_h, rgb(92, 66, 34), 0.56);
        blend_rect(item_x + 2, y - 3, item_w - 4, item_h - 4, rgb(176, 108, 42), 0.20);
        fill_rect(item_x - 10, y + 2, 4, 12, rgb(238, 178, 54));
        fill_rect(item_x + item_w + 6, y + 2, 4, 12, rgb(238, 178, 54));
    }
    draw_prompt_text(text_x + 1, y + 1, text, rgb(16, 12, 10));
    draw_prompt_text(text_x, y, text, color);
}

static void render_menu_slider(int y, const char *label, int value, int selected, uint32_t color)
{
    int item_w = 252;
    int item_h = 26;
    int item_x = SCREEN_W / 2 - item_w / 2;
    int label_x = item_x + 18;
    int bar_x = item_x + 112;
    int bar_y = y + 6;
    int slot_w = 8;
    int slot_gap = 3;
    char value_text[4];

    if (selected) {
        blend_rect(item_x, y - 5, item_w, item_h, rgb(92, 66, 34), 0.56);
        blend_rect(item_x + 2, y - 3, item_w - 4, item_h - 4, rgb(176, 108, 42), 0.20);
        fill_rect(item_x - 10, y + 2, 4, 12, rgb(238, 178, 54));
        fill_rect(item_x + item_w + 6, y + 2, 4, 12, rgb(238, 178, 54));
    }

    value = clamp_volume_step(value);
    draw_prompt_text(label_x + 1, y + 1, label, rgb(16, 12, 10));
    draw_prompt_text(label_x, y, label, color);
    for (int i = 0; i < AUDIO_VOLUME_STEPS; ++i) {
        uint32_t slot_color = i < value ? color : rgb(66, 54, 42);
        fill_rect(bar_x + i * (slot_w + slot_gap), bar_y, slot_w, 7, slot_color);
        if (selected && i < value) {
            fill_rect(bar_x + i * (slot_w + slot_gap), bar_y - 2, slot_w, 1, rgb(255, 218, 126));
        }
    }
    snprintf(value_text, sizeof(value_text), "%02d", value);
    draw_prompt_text(item_x + item_w - 38, y + 1, value_text, rgb(16, 12, 10));
    draw_prompt_text(item_x + item_w - 39, y, value_text, color);
}

static int menu_item_count_for_page(int page)
{
    if (page == MENU_PAGE_SETTINGS) {
        return SETTINGS_MENU_ITEM_COUNT;
    }
    if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        return SLOT_MENU_ITEM_COUNT;
    }
    return MAIN_MENU_ITEM_COUNT;
}

static const char *menu_item_description(int page, int item)
{
    if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        if (item == SLOT_MENU_ITEM_BACK) {
            return "WROC DO MENU";
        }
        return page == MENU_PAGE_SAVE ? "ZAPISZ W SLOCIE" : "WCZYTAJ SLOT";
    }

    if (page == MENU_PAGE_SETTINGS) {
        switch (item) {
        case SETTINGS_MENU_ITEM_DIFFICULTY:
            return "HP I OBRAZENIA WROGOW";
        case SETTINGS_MENU_ITEM_QUALITY:
            return "PBR LADNIE FAST SZYBKO";
        case SETTINGS_MENU_ITEM_POST:
            return "BLOOM I LUT KOLORU";
        case SETTINGS_MENU_ITEM_SFX_VOLUME:
            return "GLOSNOSC EFEKTOW";
        case SETTINGS_MENU_ITEM_MUSIC_VOLUME:
            return "GLOSNOSC MUZYKI";
        case SETTINGS_MENU_ITEM_FULLSCREEN:
            return "PELNY EKRAN";
        case SETTINGS_MENU_ITEM_BACK:
            return "WROC DO MENU";
        default:
            return "";
        }
    }

    switch (item) {
    case MAIN_MENU_ITEM_PLAY:
        return "START LUB WROC DO GRY";
    case MAIN_MENU_ITEM_RESTART:
        return "NOWY RUN I NOWY SEED";
    case MAIN_MENU_ITEM_SAVE:
        return "WYBIERZ SLOT ZAPISU";
    case MAIN_MENU_ITEM_LOAD:
        return "WYBIERZ SLOT ODCZYTU";
    case MAIN_MENU_ITEM_SETTINGS:
        return "OPCJE GRY";
    case MAIN_MENU_ITEM_EXIT:
        return "ZAMKNIJ GRE";
    default:
        return "";
    }
}

static void render_game_menu(int page,
                             int selected,
                             int game_started,
                             int difficulty,
                             int quality,
                             int effects,
                             int sfx_volume_value,
                             int music_volume_value,
                             int fullscreen)
{
    char slot_items[SLOT_MENU_ITEM_COUNT][32];
    const char *slot_item_ptrs[SLOT_MENU_ITEM_COUNT];
    const char *main_items[MAIN_MENU_ITEM_COUNT] = {
        game_started ? "WZNOW GRE" : "START GRY",
        "RESTART",
        "ZAPISZ GRE",
        "WCZYTAJ GRE",
        "USTAWIENIA",
        "WYJSCIE",
    };
    const char *settings_items[SETTINGS_MENU_ITEM_COUNT] = {
        difficulty_menu_text(difficulty),
        quality == RENDER_QUALITY_FAST ? "JAKOSC FAST" : "JAKOSC PBR",
        render_effects_menu_text(effects),
        "EFEKTY",
        "MUZYKA",
        fullscreen ? "FULLSCREEN ON" : "FULLSCREEN OFF",
        "WROC",
    };
    const char **items = page == MENU_PAGE_SETTINGS ? settings_items : main_items;
    int item_count = menu_item_count_for_page(page);
    int w = 328;
    int h = page == MENU_PAGE_SETTINGS ? 344 : (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD ? 408 : 336);
    int x = SCREEN_W / 2 - w / 2;
    int y = SCREEN_H / 2 - h / 2;
    if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        for (int i = 0; i < SAVEGAME_SLOT_COUNT; ++i) {
            save_slot_menu_label(i, slot_items[i], sizeof(slot_items[i]));
            slot_item_ptrs[i] = slot_items[i];
        }
        snprintf(slot_items[SLOT_MENU_ITEM_BACK], sizeof(slot_items[SLOT_MENU_ITEM_BACK]), "WROC");
        slot_item_ptrs[SLOT_MENU_ITEM_BACK] = slot_items[SLOT_MENU_ITEM_BACK];
        items = slot_item_ptrs;
    }
    if (y < 8) {
        y = 8;
    }
    if (selected < 0) {
        selected = 0;
    } else if (selected >= item_count) {
        selected = item_count - 1;
    }

    blend_rect(0, 0, SCREEN_W, SCREEN_H, rgb(0, 0, 0), 0.55);
    blend_rect(x, y, w, h, rgb(4, 5, 6), 0.80);
    blend_rect(x + 4, y + 4, w - 8, h - 8, rgb(50, 34, 22), 0.26);
    fill_rect(x + 18, y + 34, w - 36, 1, rgb(112, 82, 42));
    fill_rect(x + 18, y + h - 45, w - 36, 1, rgb(78, 58, 36));

    const char *title = "DIOOM";
    if (page == MENU_PAGE_SETTINGS) {
        title = "USTAWIENIA";
    } else if (page == MENU_PAGE_SAVE) {
        title = "ZAPIS GRY";
    } else if (page == MENU_PAGE_LOAD) {
        title = "ODCZYT GRY";
    }
    render_centered_prompt_line(y + 14, title, rgb(238, 214, 146));
    for (int i = 0; i < item_count; ++i) {
        uint32_t color = selected == i ? rgb(246, 224, 158) : rgb(198, 184, 146);
        if (page == MENU_PAGE_SETTINGS && i != SETTINGS_MENU_ITEM_BACK) {
            color = selected == i ? rgb(255, 196, 74) : rgb(210, 156, 66);
        } else if ((page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) && i != SLOT_MENU_ITEM_BACK) {
            color = selected == i ? rgb(255, 196, 74) : rgb(210, 156, 66);
        } else if (page == MENU_PAGE_MAIN && (i == MAIN_MENU_ITEM_SAVE || i == MAIN_MENU_ITEM_LOAD)) {
            color = selected == i ? rgb(255, 196, 74) : rgb(210, 156, 66);
        } else if ((page == MENU_PAGE_MAIN && i == MAIN_MENU_ITEM_EXIT) ||
                   (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_BACK) ||
                   ((page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) && i == SLOT_MENU_ITEM_BACK)) {
            color = selected == i ? rgb(238, 142, 112) : rgb(184, 116, 94);
        }
        int item_y = y + 58 + i * 32;
        if (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_SFX_VOLUME) {
            render_menu_slider(item_y, items[i], sfx_volume_value, selected == i, color);
        } else if (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_MUSIC_VOLUME) {
            render_menu_slider(item_y, items[i], music_volume_value, selected == i, color);
        } else {
            render_menu_item(item_y, items[i], selected == i, color);
        }
    }

    render_centered_prompt_line(y + h - 62, menu_item_description(page, selected), rgb(174, 156, 112));
    render_centered_prompt_line(y + h - 34, "W/S WYBOR ENTER AKCJA", rgb(148, 132, 100));
    const char *footer = game_started ? "ESC WROC" : "ESC WYJSCIE";
    if (page == MENU_PAGE_SETTINGS) {
        footer = "A/D ZMIANA ESC WROC";
    } else if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        footer = "ENTER SLOT ESC WROC";
    }
    render_centered_prompt_line(y + h - 18, footer, rgb(116, 106, 86));
}

static void render_relic_notice(const GameState *game)
{
    if (game->relic_flash <= 0.0) {
        return;
    }

    int count = game->relic_notice_count > 0 ? game->relic_notice_count : game->relic_count;
    if (count > RELIC_COUNT) {
        count = RELIC_COUNT;
    }
    char progress[4] = {
        (char)('0' + count),
        '/',
        (char)('0' + RELIC_COUNT),
        '\0',
    };
    if (game->relic_notice_count <= 0) {
        int x = SCREEN_W / 2 - 98;
        int y = 104;
        int w = 196;
        int h = 62;
        double fade = clamp01(game->relic_flash / 1.45);
        blend_rect(x, y, w, h, rgb(8, 4, 12), 0.38 * fade);
        blend_rect(x + 2, y + 2, w - 4, h - 4, rgb(74, 42, 94), 0.24 * fade);
        render_centered_prompt_line(y + 8, "BRAMA BOSSA", rgb(242, 220, 154));
        render_centered_prompt_line(y + 26, "ZBIERZ RELIKWIE", rgb(238, 210, 146));
        render_centered_prompt_line(y + 44, progress, rgb(190, 168, 124));
        return;
    }

    int x = SCREEN_W / 2 - 58;
    int y = 112;
    int w = 116;
    int h = 44;
    double fade = clamp01(game->relic_flash / 1.45);
    blend_rect(x, y, w, h, rgb(8, 4, 12), 0.34 * fade);
    blend_rect(x + 2, y + 2, w - 4, h - 4, rgb(74, 42, 94), 0.22 * fade);
    render_centered_prompt_line(y + 8, "RELIKWIA", rgb(242, 220, 154));
    render_centered_prompt_line(y + 26, progress, rgb(238, 210, 146));
}

static int close_help_on_key(GameState *game)
{
    if (game->help_timer <= 0.0 && !game->show_help) {
        return 0;
    }
    game->help_timer = 0.0;
    game->show_help = 0;
    return 1;
}

static void copy_player_progress(GameState *dst, const GameState *src)
{
    dst->player_health = src->player_health;
    dst->ammo = src->ammo;
    dst->fireball_ammo = src->fireball_ammo;
    dst->selected_weapon = src->selected_weapon;
    dst->pistol_unlocked = src->pistol_unlocked;
    dst->fireball_unlocked = src->fireball_unlocked;
    dst->shotgun_unlocked = src->shotgun_unlocked;
    dst->max_health_upgrades = src->max_health_upgrades;
    dst->damage_upgrades = src->damage_upgrades;
    dst->ammo_cap_upgrades = src->ammo_cap_upgrades;
    dst->gold = src->gold;
    dst->rapid_timer = src->rapid_timer;
    dst->damage_timer = src->damage_timer;
    dst->difficulty = src->difficulty;
    dst->trainer = src->trainer;
}

static void enter_dungeon_from_forest(GameState *game, Camera *cam, const Portal *portal)
{
    GameState player_progress = *game;
    int relic_mask = game->relic_mask;
    int relic_count = game->relic_count;
    int boss_unlocked = game->boss_unlocked;
    int relic_index = portal->boss_gate ? -1 : portal->relic_index;

    saved_forest.valid = 1;
    saved_forest.game = *game;
    saved_forest.camera = *cam;
    memcpy(saved_forest.map, level_map, sizeof(level_map));
    memcpy(saved_forest.torches, torches, sizeof(torches));

    init_game_seed(game, runtime_level_seed++, portal->target_mode);
    copy_player_progress(game, &player_progress);
    game->in_dungeon = 1;
    game->relic_mask = relic_mask;
    game->relic_count = relic_count;
    game->boss_unlocked = boss_unlocked;
    game->dungeon_relic_index = relic_index;
    sync_relic_progress(game);
    place_dungeon_exit_portal(game);
    place_dungeon_relic(game);
    set_active_music_track(music_track_for_relic(relic_index));
    *cam = (Camera){
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    reveal_fog(game, cam);
    play_sfx(SFX_PORTAL, 0.58);
}

static void enter_house_from_forest(GameState *game, Camera *cam, int house_index)
{
    if (house_index < 0 || house_index >= MAX_HOUSES || !game->houses[house_index].active) {
        return;
    }

    GameState player_progress = *game;
    int relic_mask = game->relic_mask;
    int relic_count = game->relic_count;
    int boss_unlocked = game->boss_unlocked;
    int variant = game->houses[house_index].variant;
    uint32_t loot_mask = game->houses[house_index].loot_mask;

    saved_forest.valid = 1;
    saved_forest.game = *game;
    saved_forest.camera = *cam;
    memcpy(saved_forest.map, level_map, sizeof(level_map));
    memcpy(saved_forest.torches, torches, sizeof(torches));
    saved_forest.game.houses[house_index].visited = 1;

    game->current_house_index = house_index;
    game->current_house_variant = variant;
    game->current_house_loot_mask = loot_mask;
    init_game_seed(game, runtime_level_seed++, GENERATOR_HOUSE);
    copy_player_progress(game, &player_progress);
    game->in_dungeon = 1;
    game->relic_mask = relic_mask;
    game->relic_count = relic_count;
    game->boss_unlocked = boss_unlocked;
    game->dungeon_relic_index = -1;
    game->help_timer = 0.0;
    sync_relic_progress(game);
    set_active_music_track(MUSIC_TRACK_DIES_IRAE);
    *cam = (Camera){
        .pos = {8.35, 12.50},
        .dir = {-1.0, 0.0},
        .plane = {0.0, -0.66},
    };
    reveal_fog(game, cam);
    play_sfx(SFX_DOOR, 0.48);
}

static void return_to_saved_forest(GameState *game, Camera *cam)
{
    if (!saved_forest.valid) {
        return;
    }

    copy_player_progress(&saved_forest.game, game);
    *game = saved_forest.game;
    *cam = saved_forest.camera;
    memcpy(level_map, saved_forest.map, sizeof(level_map));
    memcpy(torches, saved_forest.torches, sizeof(torches));
    active_game = game;
    sync_relic_progress(game);
    apply_forest_relic_escalation(game);
    reveal_fog(game, cam);
    saved_forest.valid = 0;
    set_active_music_track(MUSIC_TRACK_FOREST);
    play_sfx(SFX_PORTAL, 0.52);
}

static void mark_current_house_looted(GameState *game, int loot_slot)
{
    if (!saved_forest.valid || loot_slot < 0 || loot_slot >= 31) {
        return;
    }
    int house_index = game->current_house_index;
    if (house_index < 0 || house_index >= MAX_HOUSES) {
        return;
    }
    uint32_t bit = 1u << loot_slot;
    game->current_house_loot_mask |= bit;
    saved_forest.game.houses[house_index].loot_mask |= bit;
}

static void loot_prop(GameState *game, Prop *prop)
{
    if (!prop->active || prop->loot_slot < 0 || prop->looted) {
        return;
    }

    prop->looted = 1;
    mark_current_house_looted(game, prop->loot_slot);
    Item loot = {
        .active = 1,
        .type = prop->loot_type,
        .relic_index = prop->loot_amount,
        .pos = prop->pos,
    };
    pickup_item(game, &loot);
}

static int can_buy_merchant_shop_item(const GameState *game, int item)
{
    if (!game) {
        return 0;
    }
    if (item == SHOP_ITEM_AMMO) {
        return game->gold >= SHOP_AMMO_PRICE && game->ammo < pistol_ammo_cap(game);
    }
    if (item == SHOP_ITEM_HEALTH) {
        return game->gold >= SHOP_HEALTH_PRICE && game->player_health < player_max_health(game);
    }
    if (item == SHOP_ITEM_MAX_HP) {
        return game->gold >= SHOP_MAX_HP_PRICE && game->max_health_upgrades < MAX_HEALTH_UPGRADES;
    }
    if (item == SHOP_ITEM_DAMAGE) {
        return game->gold >= SHOP_DAMAGE_PRICE && game->damage_upgrades < MAX_DAMAGE_UPGRADES;
    }
    if (item == SHOP_ITEM_AMMO_CAP) {
        return game->gold >= SHOP_AMMO_CAP_PRICE && game->ammo_cap_upgrades < MAX_AMMO_CAP_UPGRADES;
    }
    if (item == SHOP_ITEM_SHOTGUN) {
        return game->gold >= SHOP_SHOTGUN_PRICE && !game->shotgun_unlocked;
    }
    return 0;
}

static int buy_merchant_shop_item(GameState *game, int item)
{
    if (!can_buy_merchant_shop_item(game, item)) {
        return 0;
    }
    if (item == SHOP_ITEM_AMMO) {
        game->gold -= SHOP_AMMO_PRICE;
        game->ammo += SHOP_AMMO_AMOUNT;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_HEALTH) {
        game->gold -= SHOP_HEALTH_PRICE;
        game->player_health += SHOP_HEALTH_AMOUNT;
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_MAX_HP) {
        game->gold -= SHOP_MAX_HP_PRICE;
        game->max_health_upgrades += 1;
        game->player_health += HEALTH_UPGRADE_AMOUNT;
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_DAMAGE) {
        game->gold -= SHOP_DAMAGE_PRICE;
        game->damage_upgrades += 1;
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_AMMO_CAP) {
        game->gold -= SHOP_AMMO_CAP_PRICE;
        game->ammo_cap_upgrades += 1;
        game->ammo += AMMO_CAP_UPGRADE_AMOUNT;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_SHOTGUN) {
        game->gold -= SHOP_SHOTGUN_PRICE;
        game->shotgun_unlocked = 1;
        game->selected_weapon = WEAPON_SHOTGUN;
        if (game->ammo < SHOTGUN_AMMO_COST * 2) {
            game->ammo = SHOTGUN_AMMO_COST * 2;
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    return 0;
}

static void interact_world(GameState *game, Camera *cam)
{
    int tx = (int)(cam->pos.x + cam->dir.x * 0.95);
    int ty = (int)(cam->pos.y + cam->dir.y * 0.95);
    int px = (int)cam->pos.x;
    int py = (int)cam->pos.y;

    for (int i = 0; i < MAX_PORTALS; ++i) {
        Portal *portal = &game->portals[i];
        if (!portal_matches(portal, tx, ty, px, py)) {
            continue;
        }
        if (portal->exit_to_forest) {
            return_to_saved_forest(game, cam);
        } else if (game->generator_mode == GENERATOR_FOREST) {
            if (portal->boss_gate && !game->boss_unlocked) {
                game->relic_flash = 1.45;
                game->relic_notice_count = 0;
                play_sfx(SFX_LOCKED, 0.42);
                return;
            }
            enter_dungeon_from_forest(game, cam, portal);
        }
        return;
    }

    int house_index = -1;
    if (active_house_prompt(game, cam, &house_index)) {
        if (is_merchant_house_index(house_index)) {
            play_sfx(SFX_LOCKED, 0.28);
            return;
        }
        enter_house_from_forest(game, cam, house_index);
        return;
    }

    int prop_index = active_prop_index(game, cam);
    if (prop_index >= 0) {
        loot_prop(game, &game->props[prop_index]);
        return;
    }

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->x == tx && door->y == ty && !door->open && !door->opening) {
            if (!door->locked || game->keys > 0) {
                door->opening = 1;
                door->locked = 0;
                play_sfx(SFX_DOOR, 0.45);
            } else {
                play_sfx(SFX_LOCKED, 0.42);
            }
            return;
        }
    }

    for (int i = 0; i < MAX_SECRETS; ++i) {
        Secret *secret = &game->secrets[i];
        if (secret->x == tx && secret->y == ty && !secret->open) {
            secret->opening = 1;
            play_sfx(SFX_DOOR, 0.38);
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
        if (!game->fireball_unlocked || (!game->trainer && game->fireball_ammo <= 0)) {
            return;
        }
        if (!spawn_player_fireball(game, cam)) {
            return;
        }
        play_sfx(SFX_FIREBALL, 0.62);
        if (!game->trainer) {
            game->fireball_ammo -= 1;
        }
        game->shot_cooldown = game->rapid_timer > 0.0 ? FIREBALL_COOLDOWN_TIME * 0.70 : FIREBALL_COOLDOWN_TIME;
        game->weapon_flash = FIREBALL_FLASH_TIME;
        game->muzzle_light = MUZZLE_LIGHT_TIME;
        game->shot_trace = 0.0;
        return;
    }

    if (game->selected_weapon == WEAPON_KNIFE) {
        play_sfx(SFX_MELEE, 0.48);
        game->shot_cooldown = game->rapid_timer > 0.0 ? KNIFE_COOLDOWN_TIME * 0.62 : KNIFE_COOLDOWN_TIME;
        game->weapon_flash = WEAPON_FLASH_TIME;
        game->shot_trace = 0.0;

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
            if (depth > KNIFE_RANGE) {
                continue;
            }

            int aim_window = sprite_h / 2;
            if (aim_window < 16) aim_window = 16;
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
            int damage = KNIFE_DAMAGE + weapon_damage_bonus(game) + (game->damage_timer > 0.0 ? 2 : 0);
            damage_monster(game, monster, damage, cam->pos);
        }
        return;
    }

    if (game->selected_weapon == WEAPON_SHOTGUN) {
        if (!game->shotgun_unlocked) {
            return;
        }
        if (!game->trainer && game->ammo < SHOTGUN_AMMO_COST) {
            return;
        }

        if (!game->trainer) {
            game->ammo -= SHOTGUN_AMMO_COST;
        }
        play_sfx(SFX_PISTOL, 0.68);
        game->shot_cooldown = game->rapid_timer > 0.0 ? SHOTGUN_COOLDOWN_TIME * 0.55 : SHOTGUN_COOLDOWN_TIME;
        game->weapon_flash = WEAPON_FLASH_TIME;
        game->muzzle_light = MUZZLE_LIGHT_TIME * 1.25;
        game->shot_trace = WEAPON_FLASH_TIME;

        int hits = 0;
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
            if (depth > SHOTGUN_RANGE) {
                continue;
            }

            int aim_window = sprite_h / 2 + 18;
            if (aim_window < 34) aim_window = 34;
            if (abs(screen_x - SCREEN_W / 2) > aim_window) {
                continue;
            }
            if (!has_line_of_sight(cam->pos, monster->pos)) {
                continue;
            }

            int damage = SHOTGUN_DAMAGE + (depth < 2.6 ? 2 : 0) + (game->damage_timer > 0.0 ? 2 : 0);
            damage_monster(game, monster, damage, cam->pos);
            monster->hit_rim_timer = HIT_RIM_TIME;
            hits++;
        }
        if (hits > 0) {
            game->hit_marker = HIT_MARKER_TIME;
        }
        return;
    }

    if (!game->pistol_unlocked) {
        return;
    }
    if (!game->trainer && game->ammo <= 0) {
        return;
    }

    if (!game->trainer) {
        game->ammo -= 1;
    }
    play_sfx(SFX_PISTOL, 0.50);
    game->shot_cooldown = game->rapid_timer > 0.0 ? SHOT_COOLDOWN_TIME * 0.48 : SHOT_COOLDOWN_TIME;
    game->weapon_flash = WEAPON_FLASH_TIME;
    game->muzzle_light = MUZZLE_LIGHT_TIME;
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
        int damage = PLAYER_DAMAGE + weapon_damage_bonus(game) + (game->damage_timer > 0.0 ? 2 : 0);
        damage_monster(game, monster, damage, cam->pos);
        monster->hit_rim_timer = HIT_RIM_TIME;
        game->hit_marker = HIT_MARKER_TIME;
    }
}

static void execute_monster_attack(GameState *game, Monster *monster, const Camera *cam, double player_dist)
{
    if ((monster_uses_projectile(monster->type) || monster->is_boss) &&
        player_dist < (monster->is_boss ? 7.2 : 8.0) &&
        (!monster->is_boss || player_dist > 1.35)) {
        spawn_monster_shot(game, monster, cam);
    } else if (!monster_uses_projectile(monster->type) &&
               player_dist < (monster->is_boss ? 1.25 : monster_melee_range(monster->type))) {
        apply_player_damage_from(
            game,
            monster->is_boss ? 24 : monster_melee_damage(monster->type),
            monster->pos,
            cam->pos,
            monster->is_boss ? 0.24 : 0.12);
        play_sfx(SFX_MELEE, 0.46);
        play_sfx(SFX_HURT, 0.34);
    }
}

static void update_game(GameState *game, const Camera *cam, double dt)
{
    game->time += dt;
    reveal_fog(game, cam);
    int combat_active = !game->victory && !game->game_over;

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->opening && !door->open) {
            door->open_amount += dt * DOOR_OPEN_SPEED;
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
        if (combat_active) {
            update_monster(game, i, cam, dt);
        }
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
        if (monster->hit_rim_timer > 0.0) {
            monster->hit_rim_timer -= dt;
            if (monster->hit_rim_timer < 0.0) {
                monster->hit_rim_timer = 0.0;
            }
        }
        if (monster->attack_anim_timer > 0.0) {
            monster->attack_anim_timer -= dt;
            if (monster->attack_anim_timer < 0.0) {
                monster->attack_anim_timer = 0.0;
            }
        }
        int windup_ready = 0;
        if (monster->attack_windup_timer > 0.0) {
            monster->attack_windup_timer -= dt;
            if (monster->attack_windup_timer <= 0.0) {
                monster->attack_windup_timer = 0.0;
                windup_ready = 1;
            }
        }

        if (!monster->active) {
            continue;
        }
        if (!combat_active) {
            continue;
        }
        Vec2 to_player = {
            cam->pos.x - monster->pos.x,
            cam->pos.y - monster->pos.y,
        };
        double player_dist = 0.0;
        int can_attack = monster->ai_state == 2 &&
                         monster_can_directly_see_player(monster, cam, &player_dist) &&
                         vec_dot(vec_norm(monster->facing), vec_norm(to_player)) > 0.55;

        if (windup_ready) {
            if (can_attack) {
                execute_monster_attack(game, monster, cam, player_dist);
            }
            continue;
        }
        if (monster->attack_windup_timer > 0.0) {
            continue;
        }

        monster->shoot_timer -= dt;
        if (monster->shoot_timer <= 0.0 && can_attack) {
            monster->facing = vec_norm((Vec2){
                cam->pos.x - monster->pos.x,
                cam->pos.y - monster->pos.y,
            });
            double windup = monster->is_boss ? BOSS_WINDUP_TIME : MONSTER_WINDUP_TIME;
            monster->facing_lock = windup + 0.12;
            monster->attack_anim_timer = windup;
            monster->attack_windup_timer = windup;
            monster->shoot_timer = monster->is_boss ? 1.65 : monster_shot_cooldown(monster->type, i);
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
    if (game->muzzle_light > 0.0) {
        game->muzzle_light -= dt;
        if (game->muzzle_light < 0.0) game->muzzle_light = 0.0;
    }
    if (game->shot_trace > 0.0) {
        game->shot_trace -= dt;
        if (game->shot_trace < 0.0) game->shot_trace = 0.0;
    }
    if (game->hit_marker > 0.0) {
        game->hit_marker -= dt;
        if (game->hit_marker < 0.0) game->hit_marker = 0.0;
    }
    if (game->pickup_flash > 0.0) {
        game->pickup_flash -= dt;
        if (game->pickup_flash < 0.0) game->pickup_flash = 0.0;
    }
    if (game->relic_flash > 0.0) {
        game->relic_flash -= dt;
        if (game->relic_flash < 0.0) game->relic_flash = 0.0;
    }
    if (game->help_timer > 0.0 && !game->show_help) {
        game->help_timer -= dt;
        if (game->help_timer < 0.0) game->help_timer = 0.0;
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
    if (game->player_damage_flash > 0.0) {
        game->player_damage_flash -= dt;
        if (game->player_damage_flash < 0.0) {
            game->player_damage_flash = 0.0;
        }
    }
    if (game->screen_shake_timer > 0.0) {
        game->screen_shake_timer -= dt;
        if (game->screen_shake_timer <= 0.0) {
            game->screen_shake_timer = 0.0;
            game->screen_shake_strength = 0.0;
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

    if (game.selected_weapon != WEAPON_KNIFE || game.pistol_unlocked || game.fireball_unlocked) {
        fprintf(stderr, "error: player did not start with locked weapons and knife selected\n");
        return 0;
    }
    select_weapon(&game, WEAPON_PISTOL);
    if (game.selected_weapon != WEAPON_KNIFE) {
        fprintf(stderr, "error: locked pistol weapon was selected\n");
        return 0;
    }
    select_weapon(&game, WEAPON_SHOTGUN);
    if (game.selected_weapon != WEAPON_KNIFE) {
        fprintf(stderr, "error: locked shotgun weapon was selected\n");
        return 0;
    }

    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 3,
        .pos = {3.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{3.5, 22.5}},
    };

    player_fire(&game, &cam);
    if (game.ammo != START_AMMO || game.monsters[0].active || game.kills != 1) {
        fprintf(stderr, "error: knife weapon failed close hit verification\n");
        return 0;
    }

    int pistol_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_PISTOL) {
            pistol_item = i;
            break;
        }
    }
    if (pistol_item < 0) {
        fprintf(stderr, "error: no pistol pickup available for verification\n");
        return 0;
    }
    int ammo_after_knife = game.ammo;
    cam.pos = game.items[pistol_item].pos;
    update_items(&game, &cam);
    if (game.items[pistol_item].active || !game.pistol_unlocked || game.selected_weapon != WEAPON_PISTOL || game.ammo <= ammo_after_knife) {
        fprintf(stderr, "error: pistol pickup verification failed\n");
        return 0;
    }

    cam.pos = (Vec2){2.5, 22.5};
    game.kills = 0;
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
    int ammo_before_pistol = game.ammo;
    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != ammo_before_pistol - 1 || game.monsters[0].hp != 2 || !game.monsters[0].active) {
        fprintf(stderr, "error: pistol weapon failed first hit verification\n");
        return 0;
    }
    if (game.muzzle_light <= 0.0 || game.hit_marker <= 0.0 || game.monsters[0].hit_rim_timer <= 0.0) {
        fprintf(stderr, "error: pistol hit feedback timers were not armed\n");
        return 0;
    }

    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != ammo_before_pistol - 2 || game.monsters[0].active || game.kills != 1) {
        fprintf(stderr, "error: pistol weapon failed kill verification\n");
        return 0;
    }

    game.shotgun_unlocked = 1;
    game.selected_weapon = WEAPON_SHOTGUN;
    game.ammo = SHOTGUN_AMMO_COST * 2;
    game.kills = 0;
    game.shot_cooldown = 0.0;
    game.monster_count = 2;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 8,
        .pos = {4.5, 22.22},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{4.5, 22.22}},
    };
    game.monsters[1] = (Monster){
        .active = 1,
        .hp = 8,
        .pos = {4.5, 22.78},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{4.5, 22.78}},
    };
    player_fire(&game, &cam);
    if (game.ammo != SHOTGUN_AMMO_COST ||
        (game.monsters[0].hp >= 8 && game.monsters[1].hp >= 8) ||
        game.hit_marker <= 0.0) {
        fprintf(stderr, "error: shotgun spread verification failed\n");
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
        .type = 0,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    Monster *monster = &game.monsters[0];

    update_monster(&game, 0, &cam, 1.0 / 60.0);
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

static int verify_monster_safety_rules(void)
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
        .pos = {11.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{11.5, 22.5}},
    };
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state != 0) {
        fprintf(stderr, "error: monster saw the player from too far away\n");
        return 0;
    }

    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {6.0, 22.5},
        .type = 1,
        .facing = {1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{6.0, 22.5}},
    };
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state != 0) {
        fprintf(stderr, "error: monster saw the player behind its back from medium range\n");
        return 0;
    }

    game.monsters[0].pos = (Vec2){4.5, 22.5};
    game.monsters[0].facing = (Vec2){1.0, 0.0};
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state == 0) {
        fprintf(stderr, "error: close monster did not notice the player behind it\n");
        return 0;
    }

    game.monster_count = 2;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.0, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .ai_state = 2,
        .patrol_count = 1,
        .patrol = {{5.0, 22.5}},
    };
    game.monsters[1] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.0, 20.5},
        .type = 1,
        .facing = {0.0, -1.0},
        .patrol_count = 1,
        .patrol = {{5.0, 20.5}},
    };
    update_monster(&game, 1, &cam, 1.0 / 60.0);
    if (game.monsters[1].ai_state != 1 || game.monsters[1].alert_timer <= 0.0) {
        fprintf(stderr, "error: nearby witness did not alert another monster\n");
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

    int gold_item = -1;
    int shrine_item = -1;
    int bonepile_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_GOLD && gold_item < 0) {
            gold_item = i;
        } else if (game.items[i].active && game.items[i].type == ITEM_SHRINE && shrine_item < 0) {
            shrine_item = i;
        } else if (game.items[i].active && game.items[i].type == ITEM_BONEPILE && bonepile_item < 0) {
            bonepile_item = i;
        }
    }
    if (gold_item < 0 || shrine_item < 0 || bonepile_item < 0) {
        fprintf(stderr, "error: generated level is missing Diablo-style pickups or props\n");
        return 0;
    }

    int gold_before = game.gold;
    cam.pos = game.items[gold_item].pos;
    update_items(&game, &cam);
    if (game.items[gold_item].active || game.gold <= gold_before) {
        fprintf(stderr, "error: gold pickup verification failed\n");
        return 0;
    }

    game.player_health = 50;
    game.rapid_timer = 0.0;
    game.damage_timer = 0.0;
    int fireball_before = game.fireball_ammo;
    int shrine_kind = game.items[shrine_item].relic_index;
    cam.pos = game.items[shrine_item].pos;
    update_items(&game, &cam);
    if (game.items[shrine_item].active ||
        (shrine_kind == 0 && game.player_health <= 50) ||
        (shrine_kind == 1 && (game.rapid_timer <= 0.0 || game.damage_timer <= 0.0)) ||
        (shrine_kind == 2 && (!game.fireball_unlocked || game.fireball_ammo <= fireball_before))) {
        fprintf(stderr, "error: shrine pickup verification failed\n");
        return 0;
    }

    game.pickup_flash = 0.0;
    cam.pos = game.items[bonepile_item].pos;
    update_items(&game, &cam);
    if (!game.items[bonepile_item].active || game.pickup_flash > 0.0) {
        fprintf(stderr, "error: bone pile prop should not be picked up\n");
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
    if (game.selected_weapon != WEAPON_KNIFE) {
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

static int verify_monster_melee_attack(void)
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
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {3.15, 22.5},
        .type = 0,
        .facing = {-1.0, 0.0},
        .ai_state = 2,
        .shoot_timer = 0.0,
        .patrol_count = 1,
        .patrol = {{3.15, 22.5}},
    };

    int health = game.player_health;
    update_game(&game, &cam, 1.0 / 60.0);
    if (game.player_health != health || game.monsters[0].attack_windup_timer <= 0.0) {
        fprintf(stderr, "error: melee monster did not telegraph before damage\n");
        return 0;
    }
    for (int i = 0; i < 30; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (game.player_health >= health || game.player_damage_flash <= 0.0 || game.screen_shake_timer <= 0.0) {
        fprintf(stderr, "error: melee monster did not damage the player\n");
        return 0;
    }
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active && game.projectiles[i].owner == PROJECTILE_OWNER_ENEMY) {
            fprintf(stderr, "error: melee monster spawned an enemy projectile\n");
            return 0;
        }
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
            if (monster->type != MONSTER_BOSS_BUTCHER || monster->hp != scale_monster_hp_for_difficulty(BOSS_HP, game.difficulty)) {
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
          monster_chase_speed(1) > monster_chase_speed(3))) {
        fprintf(stderr, "error: monster role speeds are not ordered as expected\n");
        return 0;
    }
    if (!(monster_uses_projectile(1) &&
          monster_uses_projectile(MONSTER_FLYING_HEAD) &&
          !monster_uses_projectile(0) &&
          !monster_uses_projectile(2) &&
          !monster_uses_projectile(3) &&
          monster_projectile_damage(1) > 0 &&
          monster_projectile_damage(MONSTER_FLYING_HEAD) > monster_projectile_damage(1))) {
        fprintf(stderr, "error: monster attack roles are not deterministic\n");
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
    for (int i = 0; i < 12; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.doors[door_index].opening || game.doors[door_index].open ||
        game.doors[door_index].open_amount <= 0.0 || map_at(game.doors[door_index].x, game.doors[door_index].y) == 0) {
        fprintf(stderr, "error: keyed door has no visible opening phase\n");
        return 0;
    }
    for (int i = 0; i < 64; ++i) {
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

        double entrance_x = mode == GENERATOR_HOUSE ? 8.35 : 2.5;
        double entrance_y = mode == GENERATOR_HOUSE ? 12.50 : 22.5;
        double check_x = mode == GENERATOR_HOUSE ? 8.5 : 4.5;
        double check_y = mode == GENERATOR_HOUSE ? 12.5 : 22.5;
        if (game.generator_mode != mode || !can_move(entrance_x, entrance_y) || !can_occupy(check_x, check_y, 0.12)) {
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
        if (mode == GENERATOR_HOUSE) {
            int props = 0;
            int lootable = 0;
            for (int i = 0; i < MAX_PROPS; ++i) {
                if (!game.props[i].active) {
                    continue;
                }
                props++;
                if (!generated_floor((int)game.props[i].pos.x, (int)game.props[i].pos.y)) {
                    fprintf(stderr, "error: house generator placed prop %d outside floor\n", i);
                    return 0;
                }
                if (game.props[i].loot_slot >= 0) {
                    lootable++;
                }
            }
            if (game.monster_count != 0 || props < 7 || lootable < 4 || !game.portals[0].exit_to_forest) {
                fprintf(stderr, "error: house generator did not create a quiet lootable interior\n");
                return 0;
            }
        }

        if (mode == GENERATOR_TIGHT && !generated_floor(21, 2)) {
            fprintf(stderr, "error: tight generator did not create a far exit room\n");
            return 0;
        }
        if (mode == GENERATOR_FOREST && !generated_floor(20, 2)) {
            fprintf(stderr, "error: forest generator did not create a far clearing\n");
            return 0;
        }
        if (mode == GENERATOR_FOREST) {
            int portals = 0;
            int boss_gates = 0;
            int relic_bits = 0;
            int trees = 0;
            int houses = 0;
            for (int i = 0; i < MAX_PORTALS; ++i) {
                if (!game.portals[i].active || game.portals[i].exit_to_forest || !generated_floor(game.portals[i].x, game.portals[i].y)) {
                    continue;
                }
                if (game.portals[i].boss_gate) {
                    boss_gates++;
                } else {
                    if (game.portals[i].relic_index < 0 || game.portals[i].relic_index >= RELIC_COUNT) {
                        fprintf(stderr, "error: forest portal has invalid relic index\n");
                        return 0;
                    }
                    relic_bits |= 1 << game.portals[i].relic_index;
                    portals++;
                }
            }
            for (int i = 0; i < MAX_TREES; ++i) {
                if (!game.trees[i].active) {
                    continue;
                }
                trees++;
                if (!generated_floor((int)game.trees[i].pos.x, (int)game.trees[i].pos.y)) {
                    fprintf(stderr, "error: forest generator placed tree %d outside floor\n", i);
                    return 0;
                }
            }
            for (int i = 0; i < MAX_HOUSES; ++i) {
                if (!game.houses[i].active) {
                    continue;
                }
                houses++;
                if (!generated_floor((int)game.houses[i].pos.x, (int)game.houses[i].pos.y)) {
                    fprintf(stderr, "error: forest generator placed house %d outside floor\n", i);
                    return 0;
                }
                if (can_occupy(game.houses[i].pos.x, game.houses[i].pos.y, 0.12)) {
                    fprintf(stderr, "error: forest house %d does not block movement\n", i);
                    return 0;
                }
            }
            if (portals != RELIC_COUNT || relic_bits != RELIC_MASK_ALL || boss_gates != 1) {
                fprintf(stderr, "error: forest generator did not create relic entrances and boss gate\n");
                return 0;
            }
            if (trees < 24) {
                fprintf(stderr, "error: forest generator did not create enough tree billboards\n");
                return 0;
            }
            if (houses < 2) {
                fprintf(stderr, "error: forest generator did not create enough blocking houses\n");
                return 0;
            }
        }
        if (mode == GENERATOR_BOSS) {
            Monster *boss = &game.monsters[game.monster_count - 1];
            if (!boss->is_boss || boss->type != MONSTER_BOSS_BUTCHER || boss->hp != BOSS_HP ||
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

static int verify_forest_dungeon_transition(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 404u, GENERATOR_FOREST);

    int portal_index = -1;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        if (game.portals[i].active && !game.portals[i].exit_to_forest && !game.portals[i].boss_gate) {
            portal_index = i;
            break;
        }
    }
    if (portal_index < 0 || !setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: forest dungeon entrance is not interactable\n");
        return 0;
    }

    Vec2 return_pos = cam.pos;
    game.pistol_unlocked = 1;
    game.selected_weapon = WEAPON_PISTOL;
    game.ammo = 37;
    game.shotgun_unlocked = 1;
    game.max_health_upgrades = 1;
    game.damage_upgrades = 1;
    game.ammo_cap_upgrades = 1;
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.generator_mode == GENERATOR_FOREST || !game.portals[0].exit_to_forest) {
        fprintf(stderr, "error: forest entrance did not create a dungeon with an exit\n");
        return 0;
    }
    if (!game.pistol_unlocked || !game.shotgun_unlocked || game.selected_weapon != WEAPON_PISTOL ||
        game.ammo != 37 || game.max_health_upgrades != 1 || game.damage_upgrades != 1 || game.ammo_cap_upgrades != 1) {
        fprintf(stderr, "error: dungeon entry did not preserve player weapon state\n");
        return 0;
    }
    int screen_x = 0;
    int sprite_h = 0;
    double depth = 0.0;
    Vec2 exit_pos = {game.portals[0].x + 0.5, game.portals[0].y + 0.5};
    if (!generated_floor(game.portals[0].x, game.portals[0].y) ||
        !project_sprite(&cam, exit_pos, 0.70, &screen_x, &sprite_h, &depth) ||
        screen_x < SCREEN_W / 4 || screen_x > SCREEN_W * 3 / 4) {
        fprintf(stderr, "error: dungeon exit is not visible from the crypt entrance\n");
        return 0;
    }

    game.ammo = 29;
    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: dungeon exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    double dx = cam.pos.x - return_pos.x;
    double dy = cam.pos.y - return_pos.y;
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || game.in_dungeon || dx * dx + dy * dy > 0.001) {
        fprintf(stderr, "error: dungeon exit did not restore the forest state and position\n");
        return 0;
    }
    if (!game.pistol_unlocked || !game.shotgun_unlocked || game.selected_weapon != WEAPON_PISTOL ||
        game.ammo != 29 || game.max_health_upgrades != 1 || game.damage_upgrades != 1 || game.ammo_cap_upgrades != 1) {
        fprintf(stderr, "error: dungeon exit did not preserve player weapon progress\n");
        return 0;
    }

    return 1;
}

static int setup_prop_interaction_camera(const GameState *game, Camera *cam, const Prop *prop)
{
    static const double dirs[4][2] = {{-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}};
    for (int i = 0; i < 4; ++i) {
        double half_w = prop_is_cylinder(prop) ? prop_footprint_radius(prop) : prop->half_w;
        double half_d = prop_is_cylinder(prop) ? prop_footprint_radius(prop) : prop->half_d;
        double px = prop->pos.x + dirs[i][0] * (half_w + 0.62);
        double py = prop->pos.y + dirs[i][1] * (half_d + 0.62);
        if (!generated_floor((int)px, (int)py) || !can_move(px, py)) {
            continue;
        }
        cam->pos = (Vec2){px, py};
        cam->dir = vec_norm((Vec2){prop->pos.x - px, prop->pos.y - py});
        cam->plane = (Vec2){-cam->dir.y * 0.66, cam->dir.x * 0.66};
        (void)game;
        return 1;
    }
    return 0;
}

static int verify_forest_house_transition(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 606u, GENERATOR_FOREST);

    int house_index = -1;
    for (int i = 0; i < MAX_HOUSES; ++i) {
        if (game.houses[i].active && !is_merchant_house_index(i)) {
            house_index = i;
            break;
        }
    }
    if (house_index < 0) {
        fprintf(stderr, "error: forest has no house to enter\n");
        return 0;
    }

    const House *house = &game.houses[house_index];
    cam = (Camera){
        .pos = {house_min_x(house) - 0.70, house->pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!active_house_prompt(&game, &cam, NULL)) {
        fprintf(stderr, "error: forest house entrance is not interactable\n");
        return 0;
    }

    Vec2 return_pos = cam.pos;
    game.gold = 0;
    interact_world(&game, &cam);
    if (!saved_forest.valid || game.generator_mode != GENERATOR_HOUSE || !game.in_dungeon || game.current_house_index != house_index || !game.portals[0].exit_to_forest) {
        fprintf(stderr, "error: house entrance did not create an interior level\n");
        return 0;
    }
    if (!active_portal_prompt(&game, &cam) ||
        map_at(game.portals[0].x, game.portals[0].y) != WALL_DOOR) {
        fprintf(stderr, "error: house exit door is not visible and usable from the entrance\n");
        return 0;
    }

    int prop_index = -1;
    for (int i = 0; i < MAX_PROPS; ++i) {
        if (game.props[i].active && game.props[i].loot_slot >= 0 && !game.props[i].looted) {
            prop_index = i;
            break;
        }
    }
    if (prop_index < 0 || !setup_prop_interaction_camera(&game, &cam, &game.props[prop_index])) {
        fprintf(stderr, "error: house interior has no interactable loot prop\n");
        return 0;
    }
    int loot_slot = game.props[prop_index].loot_slot;
    int gold_before = game.gold;
    int ammo_before = game.ammo;
    int health_before = game.player_health;
    int fireball_before = game.fireball_unlocked;
    interact_world(&game, &cam);
    if (!game.props[prop_index].looted ||
        !(saved_forest.game.houses[house_index].loot_mask & (1u << loot_slot)) ||
        (game.gold == gold_before && game.ammo == ammo_before && game.player_health == health_before && game.fireball_unlocked == fireball_before && game.pickup_flash <= 0.0)) {
        fprintf(stderr, "error: house loot prop did not grant and persist loot\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: house exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    double dx = cam.pos.x - return_pos.x;
    double dy = cam.pos.y - return_pos.y;
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || game.in_dungeon || dx * dx + dy * dy > 0.001 ||
        !(game.houses[house_index].loot_mask & (1u << loot_slot))) {
        fprintf(stderr, "error: house exit did not restore forest with looted state\n");
        return 0;
    }

    cam = (Camera){
        .pos = {house_min_x(&game.houses[house_index]) - 0.70, game.houses[house_index].pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    interact_world(&game, &cam);
    if (!saved_forest.valid || game.generator_mode != GENERATOR_HOUSE) {
        fprintf(stderr, "error: house re-entry failed after looting\n");
        return 0;
    }
    int persisted = 0;
    for (int i = 0; i < MAX_PROPS; ++i) {
        if (game.props[i].active && game.props[i].loot_slot == loot_slot && game.props[i].looted) {
            persisted = 1;
            break;
        }
    }
    if (!persisted) {
        fprintf(stderr, "error: looted house prop reset after re-entry\n");
        return 0;
    }

    saved_forest.valid = 0;
    return 1;
}

static int verify_merchant_shop(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 616u, GENERATOR_FOREST);

    if (!game.houses[0].active) {
        fprintf(stderr, "error: forest merchant house was not generated\n");
        return 0;
    }

    const House *house = &game.houses[0];
    cam = (Camera){
        .pos = {house_min_x(house) - 0.70, house->pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!active_merchant_house_prompt(&game, &cam)) {
        fprintf(stderr, "error: merchant house is not interactable\n");
        return 0;
    }

    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST) {
        fprintf(stderr, "error: merchant house entered an interior instead of using the shop screen\n");
        return 0;
    }

    game.gold = SHOP_AMMO_PRICE + SHOP_HEALTH_PRICE;
    game.ammo = 80;
    game.player_health = 100;
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != SHOP_HEALTH_PRICE ||
        game.ammo != 98) {
        fprintf(stderr, "error: merchant ammo purchase failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_HEALTH) ||
        game.gold != 0 ||
        game.player_health != 145) {
        fprintf(stderr, "error: merchant health purchase failed\n");
        return 0;
    }
    if (buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != 0 ||
        game.ammo != 98) {
        fprintf(stderr, "error: merchant allowed a purchase without enough gold\n");
        return 0;
    }

    game.gold = 999;
    game.ammo = pistol_ammo_cap(&game);
    if (buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != 999 ||
        game.ammo != pistol_ammo_cap(&game)) {
        fprintf(stderr, "error: merchant sold ammo past the ammo cap\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_MAX_HP) ||
        game.max_health_upgrades != 1 ||
        player_max_health(&game) != PLAYER_MAX_HEALTH + HEALTH_UPGRADE_AMOUNT ||
        game.player_health != 165) {
        fprintf(stderr, "error: merchant max health upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_DAMAGE) ||
        game.damage_upgrades != 1 ||
        weapon_damage_bonus(&game) != 1) {
        fprintf(stderr, "error: merchant damage upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_AMMO_CAP) ||
        game.ammo_cap_upgrades != 1 ||
        pistol_ammo_cap(&game) != MAX_PISTOL_AMMO + AMMO_CAP_UPGRADE_AMOUNT ||
        game.ammo != pistol_ammo_cap(&game)) {
        fprintf(stderr, "error: merchant ammo cap upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_SHOTGUN) ||
        !game.shotgun_unlocked ||
        game.selected_weapon != WEAPON_SHOTGUN) {
        fprintf(stderr, "error: merchant shotgun purchase failed\n");
        return 0;
    }
    if (buy_merchant_shop_item(&game, SHOP_ITEM_SHOTGUN)) {
        fprintf(stderr, "error: merchant sold shotgun twice\n");
        return 0;
    }
    return 1;
}

static int find_relic_item(const GameState *game, int relic_index)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game->items[i].active && game->items[i].type == ITEM_RELIC && game->items[i].relic_index == relic_index) {
            return i;
        }
    }
    return -1;
}

static int verify_generated_dungeon_relic_reachability(void)
{
    static const int modes[] = {GENERATOR_ROOMS, GENERATOR_TIGHT};
    for (int i = 0; i < (int)(sizeof(modes) / sizeof(modes[0])); ++i) {
        GameState game;
        init_game_seed(&game, LEVEL_TEST_SEED + 707u + (uint32_t)i * 101u, modes[i]);
        game.dungeon_relic_index = i;
        place_dungeon_exit_portal(&game);
        place_dungeon_relic(&game);

        int relic_item = find_relic_item(&game, i);
        if (relic_item < 0) {
            fprintf(stderr, "error: generated dungeon mode %d did not place a relic\n", modes[i]);
            return 0;
        }
        int rx = (int)game.items[relic_item].pos.x;
        int ry = (int)game.items[relic_item].pos.y;
        if (!dungeon_tile_reachable_from_entrance(&game, rx, ry)) {
            fprintf(stderr, "error: generated dungeon mode %d placed relic without entrance path\n", modes[i]);
            return 0;
        }
    }
    return 1;
}

static int verify_relic_story_progress(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 808u, GENERATOR_FOREST);

    int boss_gate = -1;
    int relic_portals = 0;
    int seen_relics = 0;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        Portal *portal = &game.portals[i];
        if (!portal->active || portal->exit_to_forest) {
            continue;
        }
        if (portal->boss_gate) {
            boss_gate = i;
            continue;
        }
        if (portal->relic_index < 0 || portal->relic_index >= RELIC_COUNT || (seen_relics & (1 << portal->relic_index))) {
            fprintf(stderr, "error: forest relic portals are not unique\n");
            return 0;
        }
        seen_relics |= 1 << portal->relic_index;
        relic_portals++;
    }
    if (relic_portals != RELIC_COUNT || seen_relics != RELIC_MASK_ALL || boss_gate < 0) {
        fprintf(stderr, "error: forest did not create 4 relic portals plus a boss gate\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[boss_gate].x, game.portals[boss_gate].y)) {
        fprintf(stderr, "error: boss gate is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST ||
        game.relic_flash <= 1.0 || game.relic_notice_count != 0) {
        fprintf(stderr, "error: locked boss gate allowed entry or gave no feedback\n");
        return 0;
    }

    int portal_index = -1;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        if (game.portals[i].active && !game.portals[i].exit_to_forest && !game.portals[i].boss_gate) {
            portal_index = i;
            break;
        }
    }
    if (portal_index < 0 || !setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: relic dungeon entrance is not interactable\n");
        return 0;
    }
    int relic_index = game.portals[portal_index].relic_index;
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.dungeon_relic_index != relic_index) {
        fprintf(stderr, "error: relic dungeon did not preserve relic index\n");
        return 0;
    }
    if (active_music_track != music_track_for_relic(relic_index)) {
        fprintf(stderr, "error: relic dungeon did not switch to its music track\n");
        return 0;
    }

    int relic_item = find_relic_item(&game, relic_index);
    if (relic_item < 0) {
        fprintf(stderr, "error: relic dungeon did not spawn its relic\n");
        return 0;
    }
    if (!dungeon_tile_reachable_from_entrance(&game, (int)game.items[relic_item].pos.x, (int)game.items[relic_item].pos.y)) {
        fprintf(stderr, "error: relic dungeon placed relic without entrance path\n");
        return 0;
    }
    int relic_guardian = 0;
    for (int i = 0; i < game.monster_count; ++i) {
        Monster *monster = &game.monsters[i];
        if (!monster->active || monster->type != MONSTER_GIANT_SKELETON) {
            continue;
        }
        Vec2 diff = {
            monster->pos.x - game.items[relic_item].pos.x,
            monster->pos.y - game.items[relic_item].pos.y,
        };
        if (vec_len(diff) <= 4.5) {
            relic_guardian = 1;
            break;
        }
    }
    if (!relic_guardian) {
        fprintf(stderr, "error: relic dungeon did not spawn a nearby giant skeleton guardian\n");
        return 0;
    }
    int screen_x = 0;
    int sprite_h = 0;
    double depth = 0.0;
    Vec2 exit_pos = {game.portals[0].x + 0.5, game.portals[0].y + 0.5};
    if (!generated_floor(game.portals[0].x, game.portals[0].y) ||
        !project_sprite(&cam, exit_pos, 0.70, &screen_x, &sprite_h, &depth) ||
        screen_x < SCREEN_W / 4 || screen_x > SCREEN_W * 3 / 4) {
        fprintf(stderr, "error: relic dungeon exit is not visible from the crypt entrance\n");
        return 0;
    }
    cam.pos = game.items[relic_item].pos;
    update_items(&game, &cam);
    if (!(game.relic_mask & (1 << relic_index)) ||
        !(saved_forest.game.relic_mask & (1 << relic_index)) ||
        game.relic_count != 1 ||
        game.boss_unlocked ||
        game.relic_flash <= 1.0 ||
        game.relic_notice_count != 1) {
        fprintf(stderr, "error: relic pickup did not persist progress correctly\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: relic dungeon exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || !(game.relic_mask & (1 << relic_index))) {
        fprintf(stderr, "error: returning to forest lost relic progress\n");
        return 0;
    }
    if (active_music_track != MUSIC_TRACK_FOREST) {
        fprintf(stderr, "error: returning to forest did not restore forest music\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: relic dungeon re-entry is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (find_relic_item(&game, relic_index) >= 0) {
        fprintf(stderr, "error: collected relic spawned again on re-entry\n");
        return 0;
    }
    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: relic dungeon exit after re-entry is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);

    game.relic_mask = RELIC_MASK_ALL;
    sync_relic_progress(&game);
    if (!setup_interaction_camera(&cam, game.portals[boss_gate].x, game.portals[boss_gate].y)) {
        fprintf(stderr, "error: unlocked boss gate is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.generator_mode != GENERATOR_BOSS || game.dungeon_relic_index != -1) {
        fprintf(stderr, "error: unlocked boss gate did not enter boss level\n");
        return 0;
    }
    if (active_music_track != MUSIC_TRACK_TOCCATA) {
        fprintf(stderr, "error: boss gate did not switch to boss music\n");
        return 0;
    }

    GameState normal;
    init_game(&normal);
    normal.monster_count = 1;
    memset(normal.monsters, 0, sizeof(normal.monsters));
    normal.monsters[0] = (Monster){.active = 1, .hp = 1, .pos = {5.5, 22.5}, .type = 1, .patrol_count = 1};
    damage_monster(&normal, &normal.monsters[0], 1, (Vec2){2.5, 22.5});
    if (normal.victory) {
        fprintf(stderr, "error: normal dungeon victory triggered without boss\n");
        return 0;
    }

    GameState boss_game;
    init_game_seed(&boss_game, LEVEL_TEST_SEED + 909u, GENERATOR_BOSS);
    Monster *boss = &boss_game.monsters[boss_game.monster_count - 1];
    boss->hp = 1;
    damage_monster(&boss_game, boss, 1, (Vec2){2.5, 22.5});
    if (!boss_game.victory) {
        fprintf(stderr, "error: boss death did not trigger victory\n");
        return 0;
    }
    int active_explosion = -1;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (boss_game.projectiles[i].active && boss_game.projectiles[i].type == PROJECTILE_EXPLOSION) {
            active_explosion = i;
            break;
        }
    }
    if (active_explosion < 0) {
        fprintf(stderr, "error: boss death did not spawn a victory explosion\n");
        return 0;
    }
    double explosion_life = boss_game.projectiles[active_explosion].life;
    Camera boss_cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    update_game(&boss_game, &boss_cam, 1.0 / 60.0);
    if (!boss_game.victory || boss_game.game_over || boss_game.projectiles[active_explosion].life >= explosion_life) {
        fprintf(stderr, "error: victory state did not keep post-boss effects responsive\n");
        return 0;
    }

    return 1;
}

static int verify_music_track_mapping(void)
{
    int seen = 0;
    for (int relic = 0; relic < RELIC_COUNT; ++relic) {
        int track = music_track_for_relic(relic);
        if (track < 0 || track >= MUSIC_TRACK_COUNT || (seen & (1 << track))) {
            fprintf(stderr, "error: relic music track mapping is not unique\n");
            return 0;
        }
        if (!midi_tracks[track].events || midi_tracks[track].event_count <= 0) {
            fprintf(stderr, "error: relic music track %d is not loaded\n", track);
            return 0;
        }
        seen |= 1 << track;
        set_active_music_track(track);
        if (active_music_track != track || midi_tracks[track].next_event != 0 || midi_tracks[track].playhead != 0.0) {
            fprintf(stderr, "error: relic music track %d did not reset cleanly\n", track);
            return 0;
        }
    }
    set_active_music_track(MUSIC_TRACK_FOREST);
    if (active_music_track != MUSIC_TRACK_FOREST) {
        fprintf(stderr, "error: forest music track did not activate\n");
        return 0;
    }
    return 1;
}

static int verify_fm_instrument_distribution(void)
{
    int mask = 0;
    static const struct {
        int track;
        int channel;
        int note;
    } checks[] = {
        {MUSIC_TRACK_DIES_IRAE, 0, 36},
        {MUSIC_TRACK_DIES_IRAE, 1, 60},
        {MUSIC_TRACK_MASONIC_FUNERAL, 2, 62},
        {MUSIC_TRACK_PATHETIQUE, 0, 52},
        {MUSIC_TRACK_TOCCATA, 0, 67},
        {MUSIC_TRACK_TOCCATA, 1, 79},
    };
    for (int i = 0; i < (int)(sizeof(checks) / sizeof(checks[0])); ++i) {
        int instrument = fm_instrument_for_note(checks[i].track, checks[i].channel, checks[i].note);
        if (instrument < 0 || instrument >= FM_INST_COUNT) {
            fprintf(stderr, "error: FM instrument selection returned invalid preset\n");
            return 0;
        }
        mask |= 1 << instrument;
    }
    int used = 0;
    for (int i = 0; i < FM_INST_COUNT; ++i) {
        if (mask & (1 << i)) {
            used++;
        }
    }
    if ((mask & (1 << FM_INST_EPIANO)) == 0 || used < 4) {
        fprintf(stderr, "error: FM instrument distribution is too narrow\n");
        return 0;
    }

    set_active_music_track(MUSIC_TRACK_PATHETIQUE);
    memset(midi_voices, 0, sizeof(midi_voices));
    fm_midi_note_on(40, 96, 0);
    fm_midi_note_on(60, 100, 1);
    fm_midi_note_on(79, 84, 2);
    double peak = 0.0;
    for (int i = 0; i < 512; ++i) {
        double sample = fm_midi_voices_sample();
        if (!isfinite(sample)) {
            fprintf(stderr, "error: FM instrument sample became non-finite\n");
            return 0;
        }
        if (fabs(sample) > peak) {
            peak = fabs(sample);
        }
    }
    if (peak <= 0.0001 || peak > 8.0) {
        fprintf(stderr, "error: FM instrument sample peak is invalid\n");
        return 0;
    }
    set_active_music_track(MUSIC_TRACK_FOREST);
    return 1;
}

static int verify_render_effect_presets(void)
{
    static const struct {
        const char *text;
        int expected;
    } checks[] = {
        {"off", RENDER_EFFECTS_OFF},
        {"full", RENDER_EFFECTS_PRESET1},
        {"1", RENDER_EFFECTS_PRESET1},
        {"preset2", RENDER_EFFECTS_PRESET2},
        {"lut3", RENDER_EFFECTS_PRESET3},
    };

    for (int i = 0; i < (int)(sizeof(checks) / sizeof(checks[0])); ++i) {
        int effects = -1;
        if (!parse_render_effects(checks[i].text, &effects) || effects != checks[i].expected) {
            fprintf(stderr, "error: render effects preset parser rejected %s\n", checks[i].text);
            return 0;
        }
    }
    if (strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET1), "preset1") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET2), "preset2") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET3), "preset3") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_OFF), "off") != 0) {
        fprintf(stderr, "error: render effects preset config text is invalid\n");
        return 0;
    }
    if (normalize_render_effects(-1) != RENDER_EFFECTS_OFF ||
        normalize_render_effects(RENDER_EFFECTS_COUNT) != RENDER_EFFECTS_OFF) {
        fprintf(stderr, "error: render effects normalization is invalid\n");
        return 0;
    }
    return 1;
}

static int verify_help_key_close(void)
{
    GameState game;
    init_game(&game);
    if (game.help_timer <= 0.0 || game.show_help) {
        fprintf(stderr, "error: startup help state is invalid\n");
        return 0;
    }
    if (!close_help_on_key(&game) || game.help_timer > 0.0 || game.show_help) {
        fprintf(stderr, "error: key press did not close startup help\n");
        return 0;
    }
    if (close_help_on_key(&game)) {
        fprintf(stderr, "error: hidden help reported as closed\n");
        return 0;
    }

    game.show_help = 1;
    if (!close_help_on_key(&game) || game.show_help || game.help_timer > 0.0) {
        fprintf(stderr, "error: key press did not close toggled help\n");
        return 0;
    }
    return 1;
}

static int verify_prompt_font_glyphs(void)
{
    const char *required = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./-:%!";
    for (const char *c = required; *c; ++c) {
        int has_pixels = 0;
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = prompt_font_glyph(*c, row);
            if (bits & ~31u) {
                fprintf(stderr, "error: prompt font glyph %c row %d exceeds 5 pixels\n", *c, row);
                return 0;
            }
            if (bits) {
                has_pixels = 1;
            }
        }
        if (!has_pixels) {
            fprintf(stderr, "error: prompt font glyph %c is missing\n", *c);
            return 0;
        }
    }
    return 1;
}

static int verify_difficulty_scaling(void)
{
    int easy_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_EASY);
    int normal_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_NORMAL);
    int hard_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_HARD);
    int nightmare_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_NIGHTMARE);
    if (!(easy_hp < normal_hp && normal_hp < hard_hp && hard_hp < nightmare_hp)) {
        fprintf(stderr, "error: difficulty HP scaling is not ordered\n");
        return 0;
    }

    GameState game;
    memset(&game, 0, sizeof(game));
    game.difficulty = DIFFICULTY_EASY;
    int easy_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_NORMAL;
    int normal_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_HARD;
    int hard_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_NIGHTMARE;
    int nightmare_damage = scale_enemy_damage_for_difficulty(&game, 20);
    if (!(easy_damage < normal_damage && normal_damage < hard_damage && hard_damage < nightmare_damage)) {
        fprintf(stderr, "error: difficulty damage scaling is not ordered\n");
        return 0;
    }
    return 1;
}

static int verify_boss_sprite_asset(void)
{
    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        for (int anim = 0; anim < BOSS_ANIM_FRAMES; ++anim) {
            int visible = 0;
            for (int i = 0; i < BOSS_SPRITE_SIZE * BOSS_SPRITE_SIZE; ++i) {
                if (!is_sprite_key(boss_sprites[frame][anim][i])) {
                    visible++;
                }
            }
            if (visible < 900) {
                fprintf(stderr, "error: boss sprite frame %d anim %d has too few visible pixels\n", frame, anim);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_furniture_sprite_asset(void)
{
    for (int sprite = 0; sprite < FURNITURE_SPRITE_COUNT; ++sprite) {
        int visible = 0;
        for (int i = 0; i < FURNITURE_SIZE * FURNITURE_SIZE; ++i) {
            if (!is_sprite_key(furniture_sprites[sprite][i])) {
                visible++;
            }
        }
        if (visible < 90) {
            fprintf(stderr, "error: furniture sprite %d has too few visible pixels\n", sprite);
            return 0;
        }
    }
    return 1;
}

static int verify_monster_sprite_assets(void)
{
    for (int type = 0; type < MONSTER_TYPES; ++type) {
        for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
            for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
                int visible = 0;
                for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; ++i) {
                    if (!is_sprite_key(monster_sprites[type][frame][anim][i])) {
                        visible++;
                    }
                }
                if (visible < 120) {
                    fprintf(stderr, "error: monster sprite type %d frame %d anim %d has too few visible pixels\n", type, frame, anim);
                    return 0;
                }
            }
        }
    }

    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
            int visible = 0;
            for (int i = 0; i < GIANT_SKELETON_SPRITE_SIZE * GIANT_SKELETON_SPRITE_SIZE; ++i) {
                if (!is_sprite_key(giant_skeleton_sprites[frame][anim][i])) {
                    visible++;
                }
            }
            if (visible < 900) {
                fprintf(stderr, "error: giant skeleton sprite frame %d anim %d has too few visible pixels\n", frame, anim);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_trainer_mode(void)
{
    int previous_trainer = runtime_trainer;
    runtime_trainer = 1;
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED + 1001u, GENERATOR_FOREST);
    runtime_trainer = previous_trainer;

    if (!game.trainer || game.relic_mask != RELIC_MASK_ALL || game.relic_count != RELIC_COUNT || !game.boss_unlocked) {
        fprintf(stderr, "error: trainer mode did not start with all relics collected\n");
        return 0;
    }

    game.player_health = 23;
    apply_player_damage(&game, 999);
    if (game.player_health != 23 || game.game_over) {
        fprintf(stderr, "error: trainer mode allowed player damage\n");
        return 0;
    }

    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    game.pistol_unlocked = 1;
    game.selected_weapon = WEAPON_PISTOL;
    game.ammo = 0;
    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != 0 || game.shot_cooldown <= 0.0) {
        fprintf(stderr, "error: trainer mode did not allow pistol fire with empty ammo\n");
        return 0;
    }

    game.fireball_unlocked = 1;
    game.selected_weapon = WEAPON_FIREBALL;
    game.fireball_ammo = 0;
    game.shot_cooldown = 0.0;
    memset(game.projectiles, 0, sizeof(game.projectiles));
    player_fire(&game, &cam);
    int fireball_spawned = 0;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active && game.projectiles[i].owner == PROJECTILE_OWNER_PLAYER) {
            fireball_spawned = 1;
            break;
        }
    }
    if (game.fireball_ammo != 0 || game.shot_cooldown <= 0.0 || !fireball_spawned) {
        fprintf(stderr, "error: trainer mode did not allow fireball fire with empty ammo\n");
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
        depth_buffer[i] = 12.0;
    }
    for (int x = 0; x < SCREEN_W; ++x) {
        z_buffer[x] = 12.0;
    }

    render_volumetric_fog(&cam, &game);

    int changed = 0;
    for (int i = 0; i < SCREEN_W * (SCREEN_H - 14); ++i) {
        if (framebuffer[i] != rgb(90, 72, 56)) {
            changed++;
        }
    }

    if (changed == 0) {
        fprintf(stderr, "error: volumetric fog pass did not modify the framebuffer\n");
        return 0;
    }
    return 1;
}

static int verify_tree_visible_at_collision_range(void)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.generator_mode = GENERATOR_FOREST;
    game.time = 1.0;
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    Tree tree = {
        .active = 1,
        .variant = 0,
        .pos = {2.5 + TREE_COLLISION_RADIUS + 0.04, 22.5},
    };
    uint32_t background = rgb(8, 7, 6);

    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        framebuffer[i] = background;
        depth_buffer[i] = 80.0;
    }
    for (int x = 0; x < SCREEN_W; ++x) {
        z_buffer[x] = 80.0;
    }

    render_tree(&cam, &game, &tree);

    int changed = 0;
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        if (framebuffer[i] != background) {
            changed++;
        }
    }
    if (changed == 0) {
        fprintf(stderr, "error: tree billboard disappeared before collision boundary\n");
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
        double min_dist = monster->is_boss ? 0.95 : 0.48;
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
    saved_forest.valid = 0;
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

static int dump_frame_mode(const char *path, int mode)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!init_assets()) {
        return 1;
    }
    if (!verify_sfx_assets()) {
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
    if (!verify_monster_safety_rules()) {
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
    if (!verify_monster_melee_attack()) {
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
    if (!verify_forest_dungeon_transition()) {
        return 1;
    }
    if (!verify_forest_house_transition()) {
        return 1;
    }
    if (!verify_merchant_shop()) {
        return 1;
    }
    if (!verify_generated_dungeon_relic_reachability()) {
        return 1;
    }
    if (!verify_relic_story_progress()) {
        return 1;
    }
    if (!verify_music_track_mapping()) {
        return 1;
    }
    if (!verify_fm_instrument_distribution()) {
        return 1;
    }
    if (!verify_render_effect_presets()) {
        return 1;
    }
    if (!verify_help_key_close()) {
        return 1;
    }
    if (!verify_prompt_font_glyphs()) {
        return 1;
    }
    if (!verify_difficulty_scaling()) {
        return 1;
    }
    if (!verify_boss_sprite_asset()) {
        return 1;
    }
    if (!verify_furniture_sprite_asset()) {
        return 1;
    }
    if (!verify_monster_sprite_assets()) {
        return 1;
    }
    if (!verify_trainer_mode()) {
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
    if (!verify_tree_visible_at_collision_range()) {
        return 1;
    }
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED, mode);
    if (mode == GENERATOR_HOUSE) {
        cam = (Camera){
            .pos = {8.35, 12.50},
            .dir = {-1.0, 0.0},
            .plane = {0.0, -0.66},
        };
    }
    reveal_fog(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (mode != GENERATOR_FOREST && mode != GENERATOR_HOUSE) {
        game.fireball_unlocked = 1;
        game.fireball_ammo = 6;
        select_weapon(&game, WEAPON_FIREBALL);
        player_fire(&game, &cam);
        for (int i = 0; i < 18; ++i) {
            update_projectiles(&game, &cam, 1.0 / 60.0);
        }
    }
    render_scene(&cam, &game);
    return write_ppm(path);
}

static int dump_frame(const char *path)
{
    return dump_frame_mode(path, GENERATOR_ROOMS);
}

static int dump_frame_quality(const char *path, int quality)
{
    int previous_quality = render_quality;
    render_quality = quality;
    int result = dump_frame(path);
    render_quality = previous_quality;
    return result;
}

static int parse_generator_mode_name(const char *text, int *out_mode)
{
    if (strcmp(text, "rooms") == 0) {
        *out_mode = GENERATOR_ROOMS;
        return 1;
    }
    if (strcmp(text, "forest") == 0) {
        *out_mode = GENERATOR_FOREST;
        return 1;
    }
    if (strcmp(text, "tight") == 0) {
        *out_mode = GENERATOR_TIGHT;
        return 1;
    }
    if (strcmp(text, "boss") == 0) {
        *out_mode = GENERATOR_BOSS;
        return 1;
    }
    if (strcmp(text, "house") == 0) {
        *out_mode = GENERATOR_HOUSE;
        return 1;
    }
    return 0;
}

static int profile_dump_frame(const char *path, int quality, int mode)
{
    RenderProfile profile;
    int previous_quality = render_quality;
    RenderProfile *previous_profile = active_profile;
    render_quality = quality;
    active_profile = &profile;
    int result = dump_frame_mode(path, mode);
    active_profile = previous_profile;
    render_quality = previous_quality;
    if (result == 0) {
        fprintf(stderr,
                "profile total=%.3f floor=%.3f wall=%.3f sprite=%.3f fog=%.3f bloom=%.3f post=%.3f\n",
                profile.total_ms,
                profile.floor_ms,
                profile.wall_ms,
                profile.sprite_ms,
                profile.fog_ms,
                profile.bloom_ms,
                profile.post_ms);
    }
    return result;
}

static int dump_forest_forward_frames(const char *prefix)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!init_assets()) {
        return 1;
    }

    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED, GENERATOR_FOREST);
    reveal_fog(&game, &cam);
    game.time = 0.75;
    game.monster_count = 0;
    memset(game.monsters, 0, sizeof(game.monsters));
    memset(game.items, 0, sizeof(game.items));
    memset(game.projectiles, 0, sizeof(game.projectiles));

    for (int frame = 0; frame < 3; ++frame) {
        if (frame > 0) {
            for (int step = 0; step < 18; ++step) {
                move_camera(&cam, &game, 1.0, 0.0, 1.0 / 60.0);
                reveal_fog(&game, &cam);
            }
        }
        render_scene(&cam, &game);

        char path[256];
        int n = snprintf(path, sizeof(path), "%s_%d.ppm", prefix, frame);
        if (n < 0 || n >= (int)sizeof(path)) {
            fprintf(stderr, "error: dump prefix is too long\n");
            return 1;
        }
        if (write_ppm(path) != 0) {
            return 1;
        }
    }
    return 0;
}

typedef struct {
    int window_w;
    int window_h;
    int integer_scale;
    int render_quality;
    int render_effects;
    int difficulty;
    int fullscreen;
    int sfx_volume;
    int music_volume;
    int trainer;
    const char *scale_quality;
} RuntimeConfig;

static int save_settings_file(int difficulty, int quality, int effects, int fullscreen, int sfx_step, int music_step, int trainer);

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;
    Camera cam;
    GameState game;
    int running;
    int paused;
    int menu_open;
    int menu_page;
    int menu_selected;
    int shop_open;
    int shop_selected;
    int game_started;
    int fullscreen;
    int relative_mouse;
    int show_fps;
    int show_timings;
    int render_quality;
    int render_effects;
    int difficulty;
    int sfx_volume;
    int music_volume;
    int trainer;
    int settings_ready;
    int fps_frames;
    uint64_t prev;
    double fps_accum;
    double fps_value;
    double frame_ms;
    RenderProfile profile;
} Runtime;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t saved_at;
    uint32_t payload_size;
    uint32_t game_size;
    uint32_t camera_size;
    uint32_t saved_level_size;
    uint32_t map_w;
    uint32_t map_h;
    uint32_t torch_count;
} SaveGameHeader;

typedef struct {
    uint32_t runtime_level_seed;
    int runtime_level_mode;
    int runtime_difficulty;
    int runtime_trainer;
    int active_music_track;
    int game_started;
    GameState game;
    Camera camera;
    int map[MAP_H][MAP_W];
    Torch torches[MAX_TORCHES];
    SavedLevel saved_forest;
} SaveGamePayload;

static void close_slot_menu(Runtime *rt);

static void set_runtime_relative_mouse(Runtime *rt, int enabled)
{
    enabled = enabled ? 1 : 0;
    if (rt->relative_mouse == enabled) {
        return;
    }
    if (SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) != 0) {
        fprintf(stderr, "warning: SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
        return;
    }
    rt->relative_mouse = enabled;
    SDL_ShowCursor(enabled ? SDL_DISABLE : SDL_ENABLE);
}

static SaveGameHeader savegame_header(uint64_t saved_at)
{
    return (SaveGameHeader){
        .magic = SAVEGAME_MAGIC,
        .version = SAVEGAME_VERSION,
        .saved_at = saved_at,
        .payload_size = (uint32_t)sizeof(SaveGamePayload),
        .game_size = (uint32_t)sizeof(GameState),
        .camera_size = (uint32_t)sizeof(Camera),
        .saved_level_size = (uint32_t)sizeof(SavedLevel),
        .map_w = MAP_W,
        .map_h = MAP_H,
        .torch_count = MAX_TORCHES,
    };
}

static int write_exact(FILE *f, const void *data, size_t size)
{
    return fwrite(data, 1, size, f) == size;
}

static int read_exact(FILE *f, void *data, size_t size)
{
    return fread(data, 1, size, f) == size;
}

static int savegame_header_matches(const SaveGameHeader *header)
{
    SaveGameHeader expected = savegame_header(0);
    return header->magic == expected.magic &&
           header->version == expected.version &&
           header->payload_size == expected.payload_size &&
           header->game_size == expected.game_size &&
           header->camera_size == expected.camera_size &&
           header->saved_level_size == expected.saved_level_size &&
           header->map_w == expected.map_w &&
           header->map_h == expected.map_h &&
           header->torch_count == expected.torch_count;
}

static int valid_generator_mode(int mode)
{
    return mode >= 0 && mode < GENERATOR_COUNT;
}

static int valid_music_track(int track)
{
    return track >= 0 && track <= MUSIC_TRACK_FOREST;
}

static int save_slot_path(int slot, int temp, char *path, size_t path_size)
{
    if (slot < 0 || slot >= SAVEGAME_SLOT_COUNT) {
        return 0;
    }
    int n = snprintf(path, path_size, temp ? SAVEGAME_TMP_PATH_FORMAT : SAVEGAME_PATH_FORMAT, slot + 1);
    return n > 0 && n < (int)path_size;
}

static int read_save_slot_header(int slot, SaveGameHeader *out_header)
{
    char path[64];
    if (!save_slot_path(slot, 0, path, sizeof(path))) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        return errno == ENOENT ? 0 : -1;
    }
    SaveGameHeader header;
    int ok = read_exact(f, &header, sizeof(header)) && savegame_header_matches(&header);
    fclose(f);
    if (!ok) {
        return -1;
    }
    if (out_header) {
        *out_header = header;
    }
    return 1;
}

static void save_slot_menu_label(int slot, char *out, size_t out_size)
{
    SaveGameHeader header;
    int status = read_save_slot_header(slot, &header);
    if (status <= 0) {
        snprintf(out, out_size, "S%d %s", slot + 1, status < 0 ? "BLAD" : "PUSTY");
        return;
    }

    time_t saved_time = (time_t)header.saved_at;
    struct tm *local = localtime(&saved_time);
    char stamp[20];
    if (local && strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M", local) > 0) {
        snprintf(out, out_size, "S%d %s", slot + 1, stamp);
    } else {
        snprintf(out, out_size, "S%d BLAD DATY", slot + 1);
    }
}

static int save_runtime_game(Runtime *rt, int slot)
{
    char path[64];
    char tmp_path[64];
    if (!save_slot_path(slot, 0, path, sizeof(path)) ||
        !save_slot_path(slot, 1, tmp_path, sizeof(tmp_path))) {
        fprintf(stderr, "error: invalid savegame slot %d\n", slot + 1);
        return 0;
    }

    time_t now = time(NULL);
    if (now == (time_t)-1) {
        fprintf(stderr, "error: cannot read current time for savegame timestamp\n");
        return 0;
    }
    SaveGameHeader header = savegame_header((uint64_t)now);
    SaveGamePayload payload;
    memset(&payload, 0, sizeof(payload));
    payload.runtime_level_seed = runtime_level_seed;
    payload.runtime_level_mode = runtime_level_mode;
    payload.runtime_difficulty = runtime_difficulty;
    payload.runtime_trainer = runtime_trainer ? 1 : 0;
    payload.active_music_track = active_music_track;
    payload.game_started = rt->game_started ? 1 : 0;
    payload.game = rt->game;
    payload.camera = rt->cam;
    memcpy(payload.map, level_map, sizeof(level_map));
    memcpy(payload.torches, torches, sizeof(torches));
    payload.saved_forest = saved_forest;

    FILE *f = fopen(tmp_path, "wb");
    if (!f) {
        fprintf(stderr, "error: cannot open savegame %s: %s\n", tmp_path, strerror(errno));
        return 0;
    }
    int ok = write_exact(f, &header, sizeof(header)) &&
             write_exact(f, &payload, sizeof(payload));
    if (fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "error: cannot write savegame %s\n", tmp_path);
        remove(tmp_path);
        return 0;
    }
    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "error: cannot replace savegame %s: %s\n", path, strerror(errno));
        remove(tmp_path);
        return 0;
    }
    close_slot_menu(rt);
    return 1;
}

static int load_runtime_game(Runtime *rt, int slot)
{
    char path[64];
    if (!save_slot_path(slot, 0, path, sizeof(path))) {
        fprintf(stderr, "error: invalid savegame slot %d\n", slot + 1);
        return 0;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open savegame %s: %s\n", path, strerror(errno));
        return 0;
    }

    SaveGameHeader header;
    SaveGamePayload payload;
    int ok = read_exact(f, &header, sizeof(header)) &&
             savegame_header_matches(&header) &&
             read_exact(f, &payload, sizeof(payload));
    if (fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "error: savegame %s has unsupported or corrupt format\n", path);
        return 0;
    }
    if (!valid_generator_mode(payload.runtime_level_mode) ||
        !valid_generator_mode(payload.game.generator_mode) ||
        normalize_difficulty(payload.runtime_difficulty) != payload.runtime_difficulty ||
        !valid_music_track(payload.active_music_track)) {
        fprintf(stderr, "error: savegame %s has invalid state values\n", path);
        return 0;
    }

    rt->game = payload.game;
    rt->cam = payload.camera;
    memcpy(level_map, payload.map, sizeof(level_map));
    memcpy(torches, payload.torches, sizeof(torches));
    saved_forest = payload.saved_forest;
    runtime_level_seed = payload.runtime_level_seed;
    runtime_level_mode = payload.runtime_level_mode;
    runtime_difficulty = payload.runtime_difficulty;
    runtime_trainer = payload.runtime_trainer ? 1 : 0;
    rt->difficulty = runtime_difficulty;
    rt->trainer = runtime_trainer;
    rt->game_started = 1;
    rt->paused = 0;
    rt->menu_open = 0;
    rt->menu_page = MENU_PAGE_MAIN;
    rt->menu_selected = MAIN_MENU_ITEM_PLAY;
    active_game = &rt->game;
    moon_visibility_cache_ready = 0;
    torch_flicker_cache_ready = 0;
    sync_relic_progress(&rt->game);
    if (saved_forest.valid) {
        sync_relic_progress(&saved_forest.game);
    }
    set_active_music_track(payload.active_music_track);
    return 1;
}

static int save_runtime_settings(const Runtime *rt)
{
    if (!rt->settings_ready) {
        return 1;
    }
    return save_settings_file(rt->difficulty,
                              rt->render_quality,
                              rt->render_effects,
                              rt->fullscreen,
                              rt->sfx_volume,
                              rt->music_volume,
                              rt->trainer);
}

static void set_runtime_difficulty(Runtime *rt, int difficulty)
{
    rt->difficulty = normalize_difficulty(difficulty);
    runtime_difficulty = rt->difficulty;
    rt->game.difficulty = rt->difficulty;
    if (saved_forest.valid) {
        saved_forest.game.difficulty = rt->difficulty;
    }
}

static void cycle_runtime_difficulty(Runtime *rt, int delta)
{
    set_runtime_difficulty(rt, adjust_difficulty(rt->difficulty, delta));
    save_runtime_settings(rt);
}

static void adjust_runtime_audio_volume(Runtime *rt, int target_music, int delta)
{
    if (target_music) {
        rt->music_volume = clamp_volume_step(rt->music_volume + delta);
    } else {
        rt->sfx_volume = clamp_volume_step(rt->sfx_volume + delta);
    }
    set_audio_volume_steps(rt->sfx_volume, rt->music_volume);
    save_runtime_settings(rt);
}

static void toggle_runtime_render_quality(Runtime *rt)
{
    rt->render_quality = rt->render_quality == RENDER_QUALITY_FAST ? RENDER_QUALITY_PBR : RENDER_QUALITY_FAST;
    save_runtime_settings(rt);
}

static void cycle_runtime_render_effects(Runtime *rt, int delta)
{
    rt->render_effects = normalize_render_effects(rt->render_effects);
    rt->render_effects = (rt->render_effects + delta + RENDER_EFFECTS_COUNT) % RENDER_EFFECTS_COUNT;
    save_runtime_settings(rt);
}

static int toggle_runtime_fullscreen(Runtime *rt)
{
    int next_fullscreen = !rt->fullscreen;
    if (SDL_SetWindowFullscreen(rt->window, next_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return 0;
    }
    rt->fullscreen = next_fullscreen;
    save_runtime_settings(rt);
    return 1;
}

static void adjust_runtime_settings_item(Runtime *rt, int item, int delta)
{
    if (item == SETTINGS_MENU_ITEM_DIFFICULTY) {
        cycle_runtime_difficulty(rt, delta);
    } else if (item == SETTINGS_MENU_ITEM_QUALITY) {
        toggle_runtime_render_quality(rt);
    } else if (item == SETTINGS_MENU_ITEM_POST) {
        cycle_runtime_render_effects(rt, delta);
    } else if (item == SETTINGS_MENU_ITEM_SFX_VOLUME) {
        adjust_runtime_audio_volume(rt, 0, delta);
    } else if (item == SETTINGS_MENU_ITEM_MUSIC_VOLUME) {
        adjust_runtime_audio_volume(rt, 1, delta);
    } else if (item == SETTINGS_MENU_ITEM_FULLSCREEN) {
        if (!toggle_runtime_fullscreen(rt)) {
            rt->running = 0;
        }
    }
}

static void open_main_menu(Runtime *rt)
{
    rt->menu_open = 1;
    rt->paused = 1;
    rt->menu_page = MENU_PAGE_MAIN;
    rt->menu_selected = MAIN_MENU_ITEM_PLAY;
}

static void open_settings_menu(Runtime *rt)
{
    rt->menu_page = MENU_PAGE_SETTINGS;
    rt->menu_selected = SETTINGS_MENU_ITEM_DIFFICULTY;
}

static void close_settings_menu(Runtime *rt)
{
    rt->menu_page = MENU_PAGE_MAIN;
    rt->menu_selected = MAIN_MENU_ITEM_SETTINGS;
}

static void open_save_menu(Runtime *rt)
{
    rt->menu_open = 1;
    rt->paused = 1;
    rt->menu_page = MENU_PAGE_SAVE;
    rt->menu_selected = 0;
}

static void open_load_menu(Runtime *rt)
{
    rt->menu_open = 1;
    rt->paused = 1;
    rt->menu_page = MENU_PAGE_LOAD;
    rt->menu_selected = 0;
}

static void close_slot_menu(Runtime *rt)
{
    int previous_page = rt->menu_page;
    rt->menu_page = MENU_PAGE_MAIN;
    rt->menu_selected = previous_page == MENU_PAGE_LOAD ? MAIN_MENU_ITEM_LOAD : MAIN_MENU_ITEM_SAVE;
}

static void open_merchant_shop(Runtime *rt)
{
    rt->shop_open = 1;
    rt->shop_selected = SHOP_ITEM_AMMO;
    rt->paused = 1;
}

static void close_merchant_shop(Runtime *rt)
{
    rt->shop_open = 0;
    rt->paused = 0;
}

static void activate_merchant_shop_item(Runtime *rt)
{
    if (rt->shop_selected == SHOP_ITEM_EXIT) {
        close_merchant_shop(rt);
        play_sfx(SFX_DOOR, 0.32);
        return;
    }
    play_sfx(buy_merchant_shop_item(&rt->game, rt->shop_selected) ? SFX_PICKUP : SFX_LOCKED, 0.46);
}

static int parse_window_size(const char *text, int *out_w, int *out_h)
{
    char *end = NULL;
    long w = strtol(text, &end, 10);
    if (end == text || (*end != 'x' && *end != 'X')) {
        return 0;
    }
    const char *h_text = end + 1;
    long h = strtol(h_text, &end, 10);
    if (end == h_text || *end != '\0' || w < 160 || h < 120 || w > 7680 || h > 4320) {
        return 0;
    }
    *out_w = (int)w;
    *out_h = (int)h;
    return 1;
}

static int parse_scale_quality(const char *text, const char **out_quality)
{
    if (strcmp(text, "nearest") == 0 || strcmp(text, "linear") == 0 || strcmp(text, "best") == 0) {
        *out_quality = text;
        return 1;
    }
    return 0;
}

static int parse_render_quality(const char *text, int *out_quality)
{
    if (strcmp(text, "pbr") == 0) {
        *out_quality = RENDER_QUALITY_PBR;
        return 1;
    }
    if (strcmp(text, "fast") == 0) {
        *out_quality = RENDER_QUALITY_FAST;
        return 1;
    }
    return 0;
}

static int parse_render_effects(const char *text, int *out_effects)
{
    if (strcmp(text, "full") == 0 || strcmp(text, "on") == 0 ||
        strcmp(text, "1") == 0 || strcmp(text, "preset1") == 0 || strcmp(text, "lut1") == 0) {
        *out_effects = RENDER_EFFECTS_PRESET1;
        return 1;
    }
    if (strcmp(text, "2") == 0 || strcmp(text, "preset2") == 0 || strcmp(text, "lut2") == 0) {
        *out_effects = RENDER_EFFECTS_PRESET2;
        return 1;
    }
    if (strcmp(text, "3") == 0 || strcmp(text, "preset3") == 0 || strcmp(text, "lut3") == 0) {
        *out_effects = RENDER_EFFECTS_PRESET3;
        return 1;
    }
    if (strcmp(text, "off") == 0) {
        *out_effects = RENDER_EFFECTS_OFF;
        return 1;
    }
    return 0;
}

static const char *render_quality_config_text(int quality)
{
    return quality == RENDER_QUALITY_FAST ? "fast" : "pbr";
}

static const char *render_effects_config_text(int effects)
{
    switch (normalize_render_effects(effects)) {
    case RENDER_EFFECTS_PRESET1:
        return "preset1";
    case RENDER_EFFECTS_PRESET2:
        return "preset2";
    case RENDER_EFFECTS_PRESET3:
        return "preset3";
    default:
        return "off";
    }
}

static void init_runtime_config_defaults(RuntimeConfig *config)
{
    config->window_w = SCREEN_W * WINDOW_SCALE;
    config->window_h = SCREEN_H * WINDOW_SCALE;
    config->integer_scale = 0;
    config->render_quality = DEFAULT_RENDER_QUALITY;
    config->render_effects = DEFAULT_RENDER_EFFECTS;
    config->difficulty = DEFAULT_DIFFICULTY;
    config->fullscreen = 0;
    config->sfx_volume = DEFAULT_SFX_VOLUME_STEP;
    config->music_volume = DEFAULT_MUSIC_VOLUME_STEP;
    config->trainer = 0;
    config->scale_quality = "nearest";
}

static char *trim_ini_text(char *text)
{
    while (isspace((unsigned char)*text)) {
        text++;
    }
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return text;
}

static int parse_ini_int(const char *text, int min_value, int max_value, int *out_value)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < min_value || value > max_value) {
        return 0;
    }
    *out_value = (int)value;
    return 1;
}

static int load_runtime_settings(RuntimeConfig *config)
{
    FILE *f = fopen(SETTINGS_PATH, "r");
    if (!f) {
        if (errno != ENOENT) {
            fprintf(stderr, "warning: cannot read %s: %s\n", SETTINGS_PATH, strerror(errno));
        }
        return 1;
    }

    char line[160];
    int line_no = 0;
    while (fgets(line, sizeof(line), f)) {
        line_no++;
        char *text = trim_ini_text(line);
        if (*text == '\0' || *text == '#' || *text == ';') {
            continue;
        }
        char *eq = strchr(text, '=');
        if (!eq) {
            fprintf(stderr, "warning: ignoring %s:%d without key=value\n", SETTINGS_PATH, line_no);
            continue;
        }
        *eq = '\0';
        char *key = trim_ini_text(text);
        char *value = trim_ini_text(eq + 1);
        int parsed = 0;
        if (strcmp(key, "quality") == 0) {
            parsed = parse_render_quality(value, &config->render_quality);
        } else if (strcmp(key, "post_process") == 0) {
            parsed = parse_render_effects(value, &config->render_effects);
        } else if (strcmp(key, "difficulty") == 0) {
            parsed = parse_difficulty_name(value, &config->difficulty);
        } else if (strcmp(key, "fullscreen") == 0) {
            parsed = parse_ini_int(value, 0, 1, &config->fullscreen);
        } else if (strcmp(key, "sfx_volume") == 0) {
            parsed = parse_ini_int(value, 0, AUDIO_VOLUME_STEPS, &config->sfx_volume);
        } else if (strcmp(key, "music_volume") == 0) {
            parsed = parse_ini_int(value, 0, AUDIO_VOLUME_STEPS, &config->music_volume);
        } else if (strcmp(key, "trainer") == 0) {
            parsed = parse_ini_int(value, 0, 1, &config->trainer);
        } else {
            fprintf(stderr, "warning: ignoring unknown setting %s:%d %s\n", SETTINGS_PATH, line_no, key);
            parsed = 1;
        }
        if (!parsed) {
            fprintf(stderr, "warning: ignoring invalid setting %s:%d %s=%s\n", SETTINGS_PATH, line_no, key, value);
        }
    }

    if (ferror(f)) {
        fprintf(stderr, "warning: cannot finish reading %s: %s\n", SETTINGS_PATH, strerror(errno));
    }
    fclose(f);
    config->difficulty = normalize_difficulty(config->difficulty);
    config->render_effects = normalize_render_effects(config->render_effects);
    config->sfx_volume = clamp_volume_step(config->sfx_volume);
    config->music_volume = clamp_volume_step(config->music_volume);
    config->trainer = config->trainer ? 1 : 0;
    return 1;
}

static int save_settings_file(int difficulty, int quality, int effects, int fullscreen, int sfx_step, int music_step, int trainer)
{
    FILE *f = fopen(SETTINGS_PATH, "w");
    if (!f) {
        fprintf(stderr, "error: cannot write %s: %s\n", SETTINGS_PATH, strerror(errno));
        return 0;
    }
    fprintf(f, "difficulty=%s\n", difficulty_config_text(difficulty));
    fprintf(f, "quality=%s\n", render_quality_config_text(quality));
    fprintf(f, "post_process=%s\n", render_effects_config_text(effects));
    fprintf(f, "fullscreen=%d\n", fullscreen ? 1 : 0);
    fprintf(f, "sfx_volume=%d\n", clamp_volume_step(sfx_step));
    fprintf(f, "music_volume=%d\n", clamp_volume_step(music_step));
    fprintf(f, "trainer=%d\n", trainer ? 1 : 0);
    if (fclose(f) != 0) {
        fprintf(stderr, "error: cannot close %s: %s\n", SETTINGS_PATH, strerror(errno));
        return 0;
    }
    return 1;
}

static int parse_runtime_config(int argc, char **argv, RuntimeConfig *config)
{
    init_runtime_config_defaults(config);
    load_runtime_settings(config);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--window") == 0) {
            if (i + 1 >= argc || !parse_window_size(argv[i + 1], &config->window_w, &config->window_h)) {
                fprintf(stderr, "error: --window expects WIDTHxHEIGHT, for example 640x480\n");
                return 0;
            }
            i += 1;
        } else if (strcmp(argv[i], "--scale") == 0) {
            if (i + 1 >= argc || !parse_scale_quality(argv[i + 1], &config->scale_quality)) {
                fprintf(stderr, "error: --scale expects nearest, linear, or best\n");
                return 0;
            }
            i += 1;
        } else if (strcmp(argv[i], "--integer-scale") == 0) {
            config->integer_scale = 1;
        } else if (strcmp(argv[i], "--quality") == 0) {
            if (i + 1 >= argc || !parse_render_quality(argv[i + 1], &config->render_quality)) {
                fprintf(stderr, "error: --quality expects pbr or fast\n");
                return 0;
            }
            i += 1;
        } else if (strcmp(argv[i], "--effects") == 0) {
            if (i + 1 >= argc || !parse_render_effects(argv[i + 1], &config->render_effects)) {
                fprintf(stderr, "error: --effects expects off, 1, 2, or 3\n");
                return 0;
            }
            i += 1;
        } else {
            fprintf(stderr, "error: unknown option %s\n", argv[i]);
            return 0;
        }
    }
    return 1;
}

static void shutdown_runtime(Runtime *rt)
{
    set_runtime_relative_mouse(rt, 0);
    if (rt->settings_ready) {
        save_runtime_settings(rt);
        rt->settings_ready = 0;
    }
    if (rt->screen) {
        SDL_DestroyTexture(rt->screen);
        rt->screen = NULL;
    }
    if (rt->renderer) {
        SDL_DestroyRenderer(rt->renderer);
        rt->renderer = NULL;
    }
    if (rt->window) {
        SDL_DestroyWindow(rt->window);
        rt->window = NULL;
    }
    shutdown_audio();
    free_midi_tracks();
    SDL_Quit();
}

static int init_runtime(Runtime *rt, const RuntimeConfig *config)
{
    memset(rt, 0, sizeof(*rt));

    if (!init_assets()) {
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    if (!init_audio()) {
        SDL_Quit();
        return 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, config->scale_quality);

    rt->window = SDL_CreateWindow(
        "Dioom",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_w,
        config->window_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!rt->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        shutdown_audio();
        SDL_Quit();
        return 0;
    }

    rt->renderer = SDL_CreateRenderer(
        rt->window,
        -1,
        SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!rt->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        shutdown_runtime(rt);
        return 0;
    }
    SDL_RenderSetIntegerScale(rt->renderer, config->integer_scale ? SDL_TRUE : SDL_FALSE);
    SDL_RenderSetLogicalSize(rt->renderer, SCREEN_W, SCREEN_H);

    rt->screen = SDL_CreateTexture(
        rt->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W,
        SCREEN_H);
    if (!rt->screen) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        shutdown_runtime(rt);
        return 0;
    }

    rt->running = 1;
    rt->paused = 1;
    rt->menu_open = 1;
    rt->menu_page = MENU_PAGE_MAIN;
    rt->menu_selected = MAIN_MENU_ITEM_PLAY;
    rt->difficulty = normalize_difficulty(config->difficulty);
    rt->trainer = config->trainer ? 1 : 0;
    runtime_difficulty = rt->difficulty;
    runtime_trainer = rt->trainer;
    runtime_level_seed = LEVEL_TEST_SEED ^ (uint32_t)SDL_GetTicks() ^ (uint32_t)SDL_GetPerformanceCounter();
    rt->render_quality = config->render_quality;
    rt->render_effects = normalize_render_effects(config->render_effects);
    render_quality = rt->render_quality;
    render_effects = rt->render_effects;
    rt->sfx_volume = clamp_volume_step(config->sfx_volume);
    rt->music_volume = clamp_volume_step(config->music_volume);
    set_audio_volume_steps(rt->sfx_volume, rt->music_volume);
    if (config->fullscreen) {
        if (SDL_SetWindowFullscreen(rt->window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
            fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
            shutdown_runtime(rt);
            return 0;
        }
        rt->fullscreen = 1;
    }
    reset_run(&rt->game, &rt->cam);
    rt->settings_ready = 1;
    rt->prev = SDL_GetPerformanceCounter();
    return 1;
}

static void runtime_frame(void *userdata)
{
    Runtime *rt = (Runtime *)userdata;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            rt->running = 0;
        } else if (event.type == SDL_KEYDOWN) {
            SDL_Keycode key = event.key.keysym.sym;
            if (rt->shop_open) {
                if ((key == SDLK_w || key == SDLK_UP) && event.key.repeat == 0) {
                    rt->shop_selected = (rt->shop_selected + SHOP_ITEM_COUNT - 1) % SHOP_ITEM_COUNT;
                } else if ((key == SDLK_s || key == SDLK_DOWN) && event.key.repeat == 0) {
                    rt->shop_selected = (rt->shop_selected + 1) % SHOP_ITEM_COUNT;
                } else if ((key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) && event.key.repeat == 0) {
                    activate_merchant_shop_item(rt);
                } else if ((key == SDLK_ESCAPE || key == SDLK_e) && event.key.repeat == 0) {
                    close_merchant_shop(rt);
                    play_sfx(SFX_DOOR, 0.30);
                }
                continue;
            }
            int help_closed = rt->menu_open ? 0 : close_help_on_key(&rt->game);
            if (rt->menu_open) {
                int menu_count = menu_item_count_for_page(rt->menu_page);
                if ((key == SDLK_w || key == SDLK_UP) && event.key.repeat == 0) {
                    rt->menu_selected = (rt->menu_selected + menu_count - 1) % menu_count;
                } else if ((key == SDLK_s || key == SDLK_DOWN) && event.key.repeat == 0) {
                    rt->menu_selected = (rt->menu_selected + 1) % menu_count;
                } else if (rt->menu_page == MENU_PAGE_SETTINGS &&
                           (key == SDLK_a || key == SDLK_LEFT || key == SDLK_d || key == SDLK_RIGHT) &&
                           event.key.repeat == 0) {
                    int delta = (key == SDLK_a || key == SDLK_LEFT) ? -1 : 1;
                    adjust_runtime_settings_item(rt, rt->menu_selected, delta);
                } else if ((key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) && event.key.repeat == 0) {
                    if (rt->menu_page == MENU_PAGE_SETTINGS) {
                        if (rt->menu_selected == SETTINGS_MENU_ITEM_BACK) {
                            close_settings_menu(rt);
                        } else {
                            adjust_runtime_settings_item(rt, rt->menu_selected, 1);
                        }
                    } else if (rt->menu_page == MENU_PAGE_SAVE || rt->menu_page == MENU_PAGE_LOAD) {
                        if (rt->menu_selected == SLOT_MENU_ITEM_BACK) {
                            close_slot_menu(rt);
                        } else if (rt->menu_page == MENU_PAGE_SAVE) {
                            play_sfx(save_runtime_game(rt, rt->menu_selected) ? SFX_PICKUP : SFX_LOCKED, 0.48);
                        } else {
                            play_sfx(load_runtime_game(rt, rt->menu_selected) ? SFX_PORTAL : SFX_LOCKED, 0.48);
                        }
                    } else {
                        if (rt->menu_selected == MAIN_MENU_ITEM_PLAY) {
                            if (!rt->game_started) {
                                reset_run(&rt->game, &rt->cam);
                            }
                            rt->menu_open = 0;
                            rt->paused = 0;
                            rt->game_started = 1;
                        } else if (rt->menu_selected == MAIN_MENU_ITEM_RESTART) {
                            reset_run(&rt->game, &rt->cam);
                            rt->menu_open = 0;
                            rt->paused = 0;
                            rt->game_started = 1;
                        } else if (rt->menu_selected == MAIN_MENU_ITEM_SAVE) {
                            open_save_menu(rt);
                        } else if (rt->menu_selected == MAIN_MENU_ITEM_LOAD) {
                            open_load_menu(rt);
                        } else if (rt->menu_selected == MAIN_MENU_ITEM_SETTINGS) {
                            open_settings_menu(rt);
                        } else if (rt->menu_selected == MAIN_MENU_ITEM_EXIT) {
                            rt->running = 0;
                        }
                    }
                } else if (key == SDLK_ESCAPE && event.key.repeat == 0) {
                    if (rt->menu_page == MENU_PAGE_SETTINGS) {
                        close_settings_menu(rt);
                    } else if (rt->menu_page == MENU_PAGE_SAVE || rt->menu_page == MENU_PAGE_LOAD) {
                        close_slot_menu(rt);
                    } else if (rt->game_started) {
                        rt->menu_open = 0;
                        rt->paused = 0;
                    }
                } else if (key == SDLK_F3 && event.key.repeat == 0) {
                    rt->show_fps = !rt->show_fps;
                    rt->fps_accum = 0.0;
                    rt->fps_frames = 0;
                } else if (key == SDLK_F4 && event.key.repeat == 0) {
                    rt->show_timings = !rt->show_timings;
                } else if (key == SDLK_F11 && event.key.repeat == 0) {
                    if (!toggle_runtime_fullscreen(rt)) {
                        rt->running = 0;
                    }
                } else if (key == SDLK_F8 && event.key.repeat == 0) {
                    open_save_menu(rt);
                } else if (key == SDLK_F9 && event.key.repeat == 0) {
                    open_load_menu(rt);
                }
                continue;
            }
            if (key == SDLK_ESCAPE) {
                open_main_menu(rt);
            } else if (key == SDLK_1) {
                select_weapon(&rt->game, WEAPON_KNIFE);
            } else if (key == SDLK_2) {
                select_weapon(&rt->game, WEAPON_PISTOL);
            } else if (key == SDLK_3 && event.key.repeat == 0) {
                select_weapon(&rt->game, WEAPON_FIREBALL);
            } else if (key == SDLK_SPACE) {
                if (!rt->paused) {
                    player_fire(&rt->game, &rt->cam);
                }
            } else if (key == SDLK_f) {
                if (!rt->paused) {
                    interact_world(&rt->game, &rt->cam);
                }
            } else if (key == SDLK_e && event.key.repeat == 0) {
                if (!rt->paused && active_merchant_house_prompt(&rt->game, &rt->cam)) {
                    open_merchant_shop(rt);
                    play_sfx(SFX_DOOR, 0.36);
                }
            } else if (key == SDLK_TAB && event.key.repeat == 0) {
                rt->game.show_automap = !rt->game.show_automap;
            } else if (key == SDLK_h && event.key.repeat == 0) {
                if (!help_closed) {
                    rt->game.show_help = 1;
                    rt->game.help_timer = 0.0;
                }
            } else if (key == SDLK_p && event.key.repeat == 0) {
                rt->paused = !rt->paused;
            } else if (key == SDLK_r && event.key.repeat == 0) {
                reset_run(&rt->game, &rt->cam);
                rt->paused = 0;
                rt->game_started = 1;
            } else if (key == SDLK_F3 && event.key.repeat == 0) {
                rt->show_fps = !rt->show_fps;
                rt->fps_accum = 0.0;
                rt->fps_frames = 0;
            } else if (key == SDLK_F4 && event.key.repeat == 0) {
                rt->show_timings = !rt->show_timings;
            } else if (key == SDLK_F11 && event.key.repeat == 0) {
                if (!toggle_runtime_fullscreen(rt)) {
                    rt->running = 0;
                }
            } else if (key == SDLK_F8 && event.key.repeat == 0) {
                open_save_menu(rt);
            } else if (key == SDLK_F9 && event.key.repeat == 0) {
                open_load_menu(rt);
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            if (!rt->paused && !rt->menu_open && !rt->shop_open && rt->game_started && !rt->game.game_over) {
                rotate_camera(&rt->cam, event.motion.xrel * 0.0024);
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (!rt->paused && !rt->menu_open && !rt->shop_open) {
                player_fire(&rt->game, &rt->cam);
            }
        } else if (event.type == SDL_MOUSEWHEEL) {
            if (!rt->paused && !rt->menu_open && !rt->shop_open && rt->game_started) {
                cycle_weapon(&rt->game, event.wheel.y > 0 ? -1 : 1);
            }
        }
    }

    if (!rt->running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        shutdown_runtime(rt);
        return;
    }

    set_runtime_relative_mouse(
        rt,
        rt->game_started && !rt->paused && !rt->menu_open && !rt->shop_open && !rt->game.game_over);

    uint64_t now = SDL_GetPerformanceCounter();
    double dt = (double)(now - rt->prev) / (double)SDL_GetPerformanceFrequency();
    rt->prev = now;
    if (dt > 0.0) {
        rt->fps_accum += dt;
        rt->fps_frames += 1;
        if (rt->fps_accum >= 0.25) {
            rt->fps_value = rt->fps_frames / rt->fps_accum;
            rt->frame_ms = 1000.0 / rt->fps_value;
            rt->fps_accum = 0.0;
            rt->fps_frames = 0;
        }
    }
    if (dt > 0.05) {
        dt = 0.05;
    }

    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    double forward = 0.0;
    double strafe = 0.0;
    double turn = 0.0;

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) forward += 1.0;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) forward -= 1.0;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_Q]) strafe -= 1.0;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_E]) strafe += 1.0;
    if (keys[SDL_SCANCODE_LEFT]) turn -= 1.0;
    if (keys[SDL_SCANCODE_RIGHT]) turn += 1.0;

    if (!rt->paused && !rt->menu_open && !rt->game.game_over && (forward != 0.0 || strafe != 0.0)) {
        move_camera(&rt->cam, &rt->game, forward, strafe, dt);
    }
    if (!rt->paused && !rt->menu_open && !rt->game.game_over && turn != 0.0) {
        rotate_camera(&rt->cam, turn * 2.2 * dt);
    }

    if (!rt->paused && !rt->menu_open && !rt->game.game_over) {
        if (!rt->game.victory) {
            update_items(&rt->game, &rt->cam);
        }
        update_game(&rt->game, &rt->cam, dt);
    }
    render_quality = rt->render_quality;
    render_effects = rt->render_effects;
    active_profile = rt->show_timings ? &rt->profile : NULL;
    render_scene(&rt->cam, &rt->game);
    active_profile = NULL;
    render_quality = RENDER_QUALITY_PBR;
    if (rt->menu_open) {
        render_game_menu(rt->menu_page,
                         rt->menu_selected,
                         rt->game_started,
                         rt->difficulty,
                         rt->render_quality,
                         rt->render_effects,
                         rt->sfx_volume,
                         rt->music_volume,
                         rt->fullscreen);
    } else if (rt->shop_open) {
        render_merchant_shop_screen(&rt->game, rt->shop_selected);
    } else if (rt->paused) {
        render_pause_overlay();
    }
    if (rt->show_fps) {
        render_fps_overlay(rt->fps_value, rt->frame_ms, rt->render_quality, rt->render_effects);
    }
    if (rt->show_timings) {
        render_timing_overlay(&rt->profile);
    }
    SDL_UpdateTexture(rt->screen, NULL, framebuffer, SCREEN_W * (int)sizeof(uint32_t));

    SDL_RenderClear(rt->renderer);
    SDL_RenderCopy(rt->renderer, rt->screen, NULL, NULL);
    SDL_RenderPresent(rt->renderer);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--dump") == 0) {
        return dump_frame(argv[2]);
    }
    if (argc == 4 && strcmp(argv[1], "--dump-quality") == 0) {
        int quality = RENDER_QUALITY_PBR;
        if (!parse_render_quality(argv[2], &quality)) {
            fprintf(stderr, "error: --dump-quality expects pbr or fast\n");
            return 1;
        }
        return dump_frame_quality(argv[3], quality);
    }
    if (argc == 5 && strcmp(argv[1], "--profile-dump") == 0) {
        int quality = RENDER_QUALITY_PBR;
        int mode = GENERATOR_ROOMS;
        if (!parse_render_quality(argv[2], &quality)) {
            fprintf(stderr, "error: --profile-dump expects pbr or fast quality\n");
            return 1;
        }
        if (!parse_generator_mode_name(argv[3], &mode)) {
            fprintf(stderr, "error: --profile-dump expects rooms, forest, tight, boss, or house mode\n");
            return 1;
        }
        return profile_dump_frame(argv[4], quality, mode);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-house") == 0) {
        return dump_frame_mode(argv[2], GENERATOR_HOUSE);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-forest") == 0) {
        return dump_frame_mode(argv[2], GENERATOR_FOREST);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-forest-forward") == 0) {
        return dump_forest_forward_frames(argv[2]);
    }

    RuntimeConfig config;
    if (!parse_runtime_config(argc, argv, &config)) {
        return 1;
    }

    static Runtime rt;
    if (!init_runtime(&rt, &config)) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(runtime_frame, &rt, 0, 1);
#else
    while (rt.running) {
        runtime_frame(&rt);
    }
#endif

    return 0;
}
