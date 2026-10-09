#ifndef DIOOM_H
#define DIOOM_H

/* Shared game data and module interfaces. Private functions and storage stay in their .c file. */
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

#ifndef SCREEN_W
#define SCREEN_W 640
#endif
#ifndef SCREEN_H
#define SCREEN_H 480
#endif
#if !((SCREEN_W == 640 && SCREEN_H == 480) || (SCREEN_W == 1280 && SCREEN_H == 960))
#error "Dioom render resolution must be 640x480 or 1280x960"
#endif

#if SCREEN_W == 1280
#define INITIAL_SCREEN_W 1280
#define INITIAL_SCREEN_H 960
#else
#define INITIAL_SCREEN_W 640
#define INITIAL_SCREEN_H 480
#endif
#undef SCREEN_W
#undef SCREEN_H
#define SCREEN_W screen_width
#define SCREEN_H screen_height
#define MAX_SCREEN_W 1280
#define MAX_SCREEN_H 960
#define HUD_HEIGHT (64 * SCREEN_H / 480)
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
#define STORY_ENTRY_COUNT 15
#define STORY_ENTRY_GATE 14
#define STORY_NOTES_MASK_ALL ((1u << STORY_ENTRY_COUNT) - 1u)
#define BOSS_PHASE_COUNT 3
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
#define TEXTURE_ATLAS_PATH "assets/doom-textures.ppm"
#define HUD_BACKGROUND_PATH "assets/doom-hud.ppm"
#define SEAL_ATLAS_PATH "assets/seals.ppm"
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
#define SAVEGAME_VERSION 9u
#define ADAPTIVE_HP_RANGE 0.35
#define ADAPTIVE_DAMAGE_RANGE 0.35
#define ADAPTIVE_COOLDOWN_RANGE 0.25
#define ADAPTIVE_WINDOW_SECONDS 20.0
#define ADAPTIVE_EMA_RATE 0.35
#define ADAPTIVE_DEATH_PENALTY 0.35
#define MAX_SAMPLE_VOICES 16
#define MAX_MIDI_VOICES 24
#ifdef __EMSCRIPTEN__
#define ACTIVE_MIDI_VOICE_LIMIT 12
#define AUDIO_BUFFER_SAMPLES 4096
#else
#define ACTIVE_MIDI_VOICE_LIMIT MAX_MIDI_VOICES
#define AUDIO_BUFFER_SAMPLES 512
#endif
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
    ITEM_SEAL = 11,
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

/* Campaign-wide knowledge survives transitions between the forest and crypts.
 * Keep it outside GameState so version-7 save payloads retain their layout. */
typedef struct {
    uint32_t notes_mask;
    uint32_t solved_mask;
    int ritual_steps[RELIC_COUNT];
    int failures[RELIC_COUNT];
} StoryProgress;

typedef struct {
    int valid;
    GameState game;
    Camera camera;
    int map[MAP_H][MAP_W];
    Torch torches[MAX_TORCHES];
} SavedLevel;

/* Hidden skill estimate for dynamic difficulty. Stored after StoryProgress in
 * version-9 saves; older saves start from the neutral state. */
typedef struct {
    double skill;
    double level_hp_scale;
    double window_time;
    double low_health_time;
    int window_damage;
    int window_shots;
    int window_hits;
    int window_kills;
    int deaths;
    int evaluations;
} AdaptiveState;

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

/* Per-surface lighting folded into fixed point: shade a texel, then apply the accumulated mixes. */
typedef struct {
    uint32_t light;
    double keep;
    double add[3];
    uint32_t keep_q;
    uint32_t add_q[3];
} SurfaceShade;

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
    double freq;
    double mod_ratio;
    double phase;
    double mod_phase;
    double age;
    double release_time;
    double release_level;
} FmMidiVoice;

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

/* Door and secret opening animations must not reshape stairs after loading.
 * Use their original footprints, independent of their current open state. */

/* One tile is a sector. Distance from its room edge creates 1/8-unit stairs
 * and a raised central platform; the room ceiling opens above the platform. */

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

/* Sector surfaces share a per-pixel depth buffer with sprites. Rays continue
 * through height changes instead of treating every riser as a full wall. */

typedef struct {
    uint32_t state;
} LevelRng;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} LevelRoom;

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
    SETTINGS_MENU_ITEM_POST,
    SETTINGS_MENU_ITEM_SFX_VOLUME,
    SETTINGS_MENU_ITEM_MUSIC_VOLUME,
    SETTINGS_MENU_ITEM_FULLSCREEN,
    SETTINGS_MENU_ITEM_RESOLUTION,
    SETTINGS_MENU_ITEM_BACK,
    SETTINGS_MENU_ITEM_COUNT
};

#define SLOT_MENU_ITEM_BACK SAVEGAME_SLOT_COUNT
#define SLOT_MENU_ITEM_COUNT (SAVEGAME_SLOT_COUNT + 1)

typedef struct {
    int window_w;
    int window_h;
    int integer_scale;
    int resolution;
    int render_quality;
    int render_effects;
    int difficulty;
    int fullscreen;
    int sfx_volume;
    int music_volume;
    int trainer;
    const char *scale_quality;
} RuntimeConfig;

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
    int story_panel;
    int story_entry;
    int story_seal;
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

/* story */
extern const int seal_orders[RELIC_COUNT][3];
extern const char *seal_names[3];
extern double story_notice_time;
extern const char *story_notice;
extern double story_dread;
extern int story_popup;
extern StoryProgress story;
void story_discover(int entry);
void story_message(const char *text);
int story_prop_entry(const GameState *game, const Prop *prop);
int story_relic_unsealed(const GameState *game, const Item *item);
int place_story_seals(GameState *game);
int active_seal_index(const GameState *game, const Camera *cam);
int invoke_story_seal(GameState *game, const Camera *cam, int item_index);
void open_story_panel(Runtime *rt, int panel, int entry, int seal);
void consume_story_popup(Runtime *rt);
void render_story_panel(const Runtime *rt);

/* adaptive difficulty */
extern AdaptiveState adaptive;
void adaptive_reset(void);
double adaptive_hp_scale(void);
double adaptive_damage_scale(void);
double adaptive_cooldown_scale(void);
int adaptive_extra_spawns(void);
void adaptive_begin_level(void);
int adaptive_monster_hp(const GameState *game, int base_hp);
int adaptive_enemy_damage(int damage);
void adaptive_note_damage_taken(int damage);
void adaptive_note_shot(void);
void adaptive_note_hit(void);
void adaptive_note_kill(void);
void adaptive_note_death(void);
void adaptive_update(const GameState *game, double dt);
int adaptive_state_valid(const AdaptiveState *state);

/* gameplay */
int boss_phase(const GameState *game, const Monster *monster);
const Monster *engaged_boss(const GameState *game);
int player_max_health(const GameState *game);
int pistol_ammo_cap(const GameState *game);
int weapon_damage_bonus(const GameState *game);
int scale_monster_hp_for_difficulty(int hp, int difficulty);
int scale_enemy_damage_for_difficulty(const GameState *game, int damage);
void apply_player_damage(GameState *game, int damage);
int move_monster_by(Monster *monster, Vec2 delta);
void update_monster(GameState *game, int monster_index, const Camera *cam, double dt);
void spawn_decal(GameState *game, Vec2 pos, int variant, double radius, double life, double angle);
void damage_monster(GameState *game, Monster *monster, int damage, Vec2 source);
void spawn_monster_shot(GameState *game, const Monster *monster, const Camera *cam);
void update_projectiles(GameState *game, const Camera *cam, double dt);
void update_items(GameState *game, const Camera *cam);
void select_weapon(GameState *game, int weapon);
void cycle_weapon(GameState *game, int dir);
int portal_matches(const Portal *portal, int tx, int ty, int px, int py);
int close_help_on_key(GameState *game);
void loot_prop(GameState *game, Prop *prop);
int can_buy_merchant_shop_item(const GameState *game, int item);
int buy_merchant_shop_item(GameState *game, int item);
void interact_world(GameState *game, Camera *cam);
void player_fire(GameState *game, const Camera *cam);
void update_game(GameState *game, const Camera *cam, double dt);
void move_camera(Camera *cam, const GameState *game, double forward, double strafe, double dt);
void rotate_camera(Camera *cam, double amount);
void reset_run(GameState *game, Camera *cam);

/* runtime */
extern int screen_height;
extern int screen_width;
int normalize_difficulty(int difficulty);
const char *difficulty_menu_text(int difficulty);
int parse_resolution(const char *text, int *out_resolution);
void close_slot_menu(Runtime *rt);
int parse_render_quality(const char *text, int *out_quality);
int parse_render_effects(const char *text, int *out_effects);
const char *render_effects_config_text(int effects);
int parse_runtime_config(int argc, char **argv, RuntimeConfig *config);
int init_runtime(Runtime *rt, const RuntimeConfig *config);
void runtime_frame(void *userdata);

/* renderer */
extern double sector_ceiling[MAP_H][MAP_W];

extern int moon_visibility_cache_ready;
extern int torch_flicker_cache_ready;
extern double sector_floor[MAP_H][MAP_W];
extern int render_effects;
extern int render_quality;
extern RenderProfile *active_profile;
extern float light_buffer[MAX_SCREEN_W * MAX_SCREEN_H];
extern float glow_buffer[MAX_SCREEN_W * MAX_SCREEN_H];
extern float depth_buffer[MAX_SCREEN_W * MAX_SCREEN_H];
extern double z_buffer[MAX_SCREEN_W];
extern uint32_t framebuffer[MAX_SCREEN_W * MAX_SCREEN_H];
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint8_t clamp_u8(int v);
uint32_t shade(uint32_t color, double amount);
uint32_t mix_color(uint32_t a, uint32_t b, double t);
uint32_t lerp_color_q8(uint32_t a, uint32_t b, uint32_t t);
uint32_t mix_amount_q8(double t);
void surface_shade_init(SurfaceShade *shade_out, double light);
void surface_shade_mix(SurfaceShade *shade_out, uint32_t color, double amount);
void surface_shade_finish(SurfaceShade *shade_out);
uint32_t surface_shade_apply(const SurfaceShade *shade_in, uint32_t texel);
double clamp01(double v);
double smooth01(double v);
uint32_t fog_color_for_game(const GameState *game);
double fog_amount(double distance);
double fog_amount_for_game(const GameState *game, double distance);
uint32_t apply_fog(uint32_t color, double distance, double strength);
uint32_t apply_game_fog(const GameState *game, uint32_t color, double distance, double strength);
double luminance(uint32_t color);
uint8_t luma_u8(uint32_t color);
uint32_t add_color(uint32_t color, uint32_t add, double amount);
uint32_t contrast_color(uint32_t color, double contrast, double brightness);
void put_pixel(int x, int y, uint32_t color);
void add_glow(int x, int y, double amount);
void add_light(int x, int y, double amount);
void reset_render_buffers(void);
double floor_height_at(Vec2 pos);
double camera_eye_height(const Camera *cam);
void build_sector_heights(const GameState *game);
int sprite_height_offset(const Camera *cam, Vec2 pos, double depth);
void prepare_torch_flicker_cache(double time);
double forest_moon_visibility_at(double x, double y);
uint32_t star_hash(int x, int y);
void prepare_world_light_grid(double time);
double cached_world_light(double x, double y);
int is_sprite_key(uint32_t color);
void clear_wall_decal_index(GameState *game);
void link_wall_decal(GameState *game, int index);
int prop_is_cylinder(const Prop *prop);
double prop_footprint_radius(const Prop *prop);
void render_sector_world(const Camera *cam, const GameState *game);
void render_scene(const Camera *cam, const GameState *game);

/* audio */
extern double music_volume;
extern double sfx_volume;
extern int active_music_track;
extern FmMidiVoice midi_voices[MAX_MIDI_VOICES];
extern MidiMusic midi_tracks[MUSIC_TRACK_COUNT];
int fm_instrument_for_note(int track, int channel, int note);
void fm_midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel);
double fm_midi_voices_sample(void);
int clamp_volume_step(int step);
void set_audio_volume_steps(int sfx_step, int music_step);
void set_active_music_track(int track);
int music_track_for_relic(int relic_index);
int verify_sfx_assets(void);
int init_audio(void);
void shutdown_audio(void);
void play_sfx(int sfx, double volume);
void free_midi_tracks(void);
int load_midi_music(const char *path, MidiMusic *music);

/* ui */
void fill_rect(int x, int y, int w, int h, uint32_t color);
void render_hit_flash(const GameState *game);
void render_player_damage_feedback(const Camera *cam, const GameState *game);
void render_crosshair(const GameState *game);
void render_shot_trace(const GameState *game);
void render_weapon(const GameState *game);
void draw_scaled_text(int x, int y, const char *text, uint32_t color, int scale);
void render_hud(const GameState *game);
void render_boss_bar(const GameState *game);
void render_minimap(const Camera *cam, const GameState *game);
void render_full_automap(const Camera *cam, const GameState *game);
void render_pause_overlay(void);
void blend_rect(int x, int y, int w, int h, uint32_t color, double amount);
void render_fps_overlay(double fps, double frame_ms, int quality, int effects);
void render_timing_overlay(const RenderProfile *profile);
const House *active_house_prompt(const GameState *game, const Camera *cam, int *out_index);
int is_merchant_house_index(int house_index);
const House *active_merchant_house_prompt(const GameState *game, const Camera *cam);
int active_prop_index(const GameState *game, const Camera *cam);
void render_interaction_prompt(const Camera *cam, const GameState *game);
void render_merchant_shop_screen(const GameState *game, int selected);
void render_victory_screen(void);
void render_help_overlay(const GameState *game);
int menu_item_count_for_page(int page);
void render_game_menu(int page,
                             int selected,
                             int game_started,
                             int difficulty,
                             int quality,
                             int effects,
                             int sfx_volume_value,
                             int music_volume_value,
                             int fullscreen);
void render_relic_notice(const GameState *game);

/* assets */
extern uint32_t wall_decal_sprites[WALL_DECAL_COUNT][WALL_DECAL_SIZE * WALL_DECAL_SIZE];
extern uint32_t decal_sprites[DECAL_COUNT][DECAL_SIZE * DECAL_SIZE];
extern uint32_t weapon_sprites[WEAPON_SPRITE_COUNT][WEAPON_SPRITE_SIZE * WEAPON_SPRITE_SIZE];
extern uint32_t item_sprites[ITEM_SPRITE_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];
extern uint32_t seal_sprites[3][PROJECTILE_SIZE * PROJECTILE_SIZE];
extern uint32_t relic_sprites[RELIC_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];
extern uint32_t furniture_sprites[FURNITURE_SPRITE_COUNT][FURNITURE_SIZE * FURNITURE_SIZE];
extern uint32_t house_textures[HOUSE_TEX_COUNT][TEX_SIZE * TEX_SIZE];
extern uint32_t tree_sprites[TREE_TYPES][SPRITE_SIZE * SPRITE_SIZE];
extern uint32_t boss_sprites[SPRITE_FRAMES][BOSS_ANIM_FRAMES][BOSS_SPRITE_SIZE * BOSS_SPRITE_SIZE];
extern uint32_t giant_skeleton_sprites[SPRITE_FRAMES][MONSTER_ANIM_FRAMES][GIANT_SKELETON_SPRITE_SIZE * GIANT_SKELETON_SPRITE_SIZE];
extern uint32_t monster_sprites[MONSTER_TYPES][SPRITE_FRAMES][MONSTER_ANIM_FRAMES][SPRITE_SIZE * SPRITE_SIZE];
extern uint32_t textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];
extern uint32_t hud_background[640 * 64];
int append_midi_tempo_event(MidiTempoEvent **events, int *count, int *capacity, MidiTempoEvent event);
int compare_midi_raw_events(const void *a, const void *b);
int compare_midi_tempo_events(const void *a, const void *b);
int parse_midi_track(const uint8_t *data,
                            size_t start,
                            size_t end,
                            MidiRawEvent **raw_events,
                            int *raw_count,
                            int *raw_capacity,
                            MidiTempoEvent **tempo_events,
                            int *tempo_count,
                            int *tempo_capacity);
double midi_tick_to_seconds(uint32_t tick, const MidiTempoEvent *tempos, int tempo_count, int ticks_per_quarter);
int init_assets(void);

/* world */
extern SavedLevel saved_forest;
extern int runtime_trainer;
extern int runtime_difficulty;
extern int runtime_level_mode;
extern uint32_t runtime_level_seed;
extern Torch torches[MAX_TORCHES];
extern int level_map[MAP_H][MAP_W];
extern GameState *active_game;
const Door *door_at_tile(const GameState *game, int x, int y);
int map_at(int x, int y);
int can_step_between(Vec2 from, Vec2 to, double radius);
double vec_len(Vec2 v);
Vec2 vec_norm(Vec2 v);
double vec_dot(Vec2 a, Vec2 b);
int has_line_of_sight(Vec2 from, Vec2 to);
int monster_max_hp(int type);
double monster_patrol_speed(int type);
double monster_chase_speed(int type);
double monster_attack_speed(int type);
double monster_retreat_speed(int type);
double monster_preferred_distance(int type);
int monster_uses_projectile(int type);
double monster_melee_range(int type);
int monster_melee_damage(int type);
int monster_is_behind_player(const Monster *monster, const Camera *cam);
int monster_can_directly_see_player(const Monster *monster, const Camera *cam, double *out_dist);
int monster_has_nearby_witness(const GameState *game, int monster_index, const Camera *cam);
double monster_shot_cooldown(int type, int index);
int monster_projectile_damage(int type);
int count_relics(int mask);
void sync_relic_progress(GameState *game);
void reveal_fog(GameState *game, const Camera *cam);
double start_dist2(int x, int y);
int generated_floor(int x, int y);
double house_min_x(const House *house);
double house_max_x(const House *house);
double house_min_y(const House *house);
double house_max_y(const House *house);
int houses_block_area(const GameState *game, double x, double y, double radius);
int props_block_area(const GameState *game, double x, double y, double radius);
int occupied_spawn_tile(const GameState *game, int x, int y);
void mark_dungeon_reachable_tiles(const GameState *game, unsigned char reachable[MAP_H][MAP_W]);
int dungeon_tile_reachable_from_entrance(const GameState *game, int x, int y);
void place_dungeon_exit_portal(GameState *game);
void place_dungeon_relic(GameState *game);
void apply_forest_relic_escalation(GameState *game);
void init_game_seed(GameState *game, uint32_t seed, int mode);
void init_game(GameState *game);
int can_occupy(double x, double y, double radius);
int can_move(double x, double y);

/* render_sprites */
int monster_frame_for_camera(const Camera *cam, const Monster *monster);
int monster_anim_frame_for_monster(const Monster *monster);
int project_sprite(const Camera *cam, Vec2 pos, double scale, int *screen_x, int *screen_h, double *depth);
void render_monster(const Camera *cam, const Monster *monster);
uint32_t prop_texel(const Prop *prop, double u, double v);
void render_tree(const Camera *cam, const GameState *game, const Tree *tree);
void render_world_sprites(const Camera *cam, const GameState *game);
void render_dynamic_shadows(const Camera *cam, const GameState *game);
void render_decals(const Camera *cam, const GameState *game);

/* render_effects */
extern int vignette_ready;
extern int color_grade_lut_ready;
void render_volumetric_fog(const Camera *cam, const GameState *game);
int normalize_render_effects(int effects);
const char *render_effects_menu_text(int effects);
void render_light_and_bloom(int effects);
void render_edge_antialias(void);
void build_color_grade_lut(void);
void render_muzzle_light(const GameState *game);
void render_forest_weather_overlay(const GameState *game);
void render_color_grade(const GameState *game, int effects);

/* validation */
int dump_frame_mode(const char *path, int mode);
int dump_frame(const char *path);
int dump_frame_quality(const char *path, int quality);
int parse_generator_mode_name(const char *text, int *out_mode);
int profile_dump_frame(const char *path, int quality, int mode);
int dump_forest_forward_frames(const char *prefix);

/* savegame */
void save_slot_menu_label(int slot, char *out, size_t out_size);
int save_runtime_game(Runtime *rt, int slot);
int load_runtime_game(Runtime *rt, int slot);

#ifdef __EMSCRIPTEN__
void web_restore_persistent_files(int slot_count);
int web_persist_text_file(const char *path_ptr);
int web_persist_binary_file(const char *path_ptr);
#endif

#endif
