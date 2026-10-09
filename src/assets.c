#include "dioom.h"

static int ppm_next_int(FILE *f, int *value);
static int load_texture_atlas(const char *path);
static int load_hud_background(const char *path);
static int load_monster_atlas(const char *path);
static int load_giant_skeleton_atlas(const char *path);
static int load_boss_atlas(const char *path);
static int load_tree_atlas(const char *path);
static int load_house_atlas(const char *path);
static int load_furniture_atlas(const char *path);
static int load_relic_atlas(const char *path);
static int load_item_atlas(const char *path);
static int load_seal_atlas(const char *path);
static int load_weapon_atlas(const char *path);
static int load_decal_atlas(const char *path);
static int load_wall_decal_atlas(const char *path);
static int read_midi_vlq(const uint8_t *data, size_t end, size_t *pos, uint32_t *value);
static int append_midi_raw_event(MidiRawEvent **events, int *count, int *capacity, MidiRawEvent event);
static int load_music_assets(void);

uint32_t wall_decal_sprites[WALL_DECAL_COUNT][WALL_DECAL_SIZE * WALL_DECAL_SIZE];

uint32_t decal_sprites[DECAL_COUNT][DECAL_SIZE * DECAL_SIZE];

uint32_t weapon_sprites[WEAPON_SPRITE_COUNT][WEAPON_SPRITE_SIZE * WEAPON_SPRITE_SIZE];

uint32_t item_sprites[ITEM_SPRITE_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];

uint32_t seal_sprites[3][PROJECTILE_SIZE * PROJECTILE_SIZE];

uint32_t relic_sprites[RELIC_COUNT][PROJECTILE_SIZE * PROJECTILE_SIZE];

uint32_t furniture_sprites[FURNITURE_SPRITE_COUNT][FURNITURE_SIZE * FURNITURE_SIZE];

uint32_t house_textures[HOUSE_TEX_COUNT][TEX_SIZE * TEX_SIZE];

uint32_t tree_sprites[TREE_TYPES][SPRITE_SIZE * SPRITE_SIZE];

uint32_t boss_sprites[SPRITE_FRAMES][BOSS_ANIM_FRAMES][BOSS_SPRITE_SIZE * BOSS_SPRITE_SIZE];

uint32_t giant_skeleton_sprites[SPRITE_FRAMES][MONSTER_ANIM_FRAMES][GIANT_SKELETON_SPRITE_SIZE * GIANT_SKELETON_SPRITE_SIZE];

uint32_t monster_sprites[MONSTER_TYPES][SPRITE_FRAMES][MONSTER_ANIM_FRAMES][SPRITE_SIZE * SPRITE_SIZE];

uint32_t textures[TEX_COUNT][TEX_SIZE * TEX_SIZE];

uint32_t hud_background[640 * 64];

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

static int load_hud_background(const char *path)
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
             width == 640 &&
             height == 64 &&
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

    for (int i = 0; i < 640 * 64; ++i) {
        hud_background[i] = rgb(pixels[i * 3], pixels[i * 3 + 1], pixels[i * 3 + 2]);
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

static int load_seal_atlas(const char *path)
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
             width == PROJECTILE_SIZE * 3 &&
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

    for (int sprite = 0; sprite < 3; ++sprite) {
        int sprite_x = sprite * PROJECTILE_SIZE;
        for (int y = 0; y < PROJECTILE_SIZE; ++y) {
            for (int x = 0; x < PROJECTILE_SIZE; ++x) {
                size_t src = ((size_t)y * (size_t)width + (size_t)(sprite_x + x)) * 3;
                seal_sprites[sprite][y * PROJECTILE_SIZE + x] = rgb(pixels[src], pixels[src + 1], pixels[src + 2]);
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

int append_midi_tempo_event(MidiTempoEvent **events, int *count, int *capacity, MidiTempoEvent event)
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

int compare_midi_raw_events(const void *a, const void *b)
{
    const MidiRawEvent *ea = (const MidiRawEvent *)a;
    const MidiRawEvent *eb = (const MidiRawEvent *)b;
    if (ea->tick < eb->tick) return -1;
    if (ea->tick > eb->tick) return 1;
    if (ea->on != eb->on) return ea->on ? 1 : -1;
    if (ea->channel != eb->channel) return (int)ea->channel - (int)eb->channel;
    return (int)ea->note - (int)eb->note;
}

int compare_midi_tempo_events(const void *a, const void *b)
{
    const MidiTempoEvent *ea = (const MidiTempoEvent *)a;
    const MidiTempoEvent *eb = (const MidiTempoEvent *)b;
    if (ea->tick < eb->tick) return -1;
    if (ea->tick > eb->tick) return 1;
    return 0;
}

int parse_midi_track(const uint8_t *data,
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

double midi_tick_to_seconds(uint32_t tick, const MidiTempoEvent *tempos, int tempo_count, int ticks_per_quarter)
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

int init_assets(void)
{
    if (!load_hud_background(HUD_BACKGROUND_PATH)) {
        fprintf(stderr, "error: cannot load required HUD bitmap %s (expected 640x64 P6)\n", HUD_BACKGROUND_PATH);
        return 0;
    }
    if (!load_texture_atlas(TEXTURE_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required texture atlas %s\n", TEXTURE_ATLAS_PATH);
        return 0;
    }
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
    if (!load_seal_atlas(SEAL_ATLAS_PATH)) {
        fprintf(stderr, "error: cannot load required seal atlas %s (expected 96x32 P6)\n", SEAL_ATLAS_PATH);
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
