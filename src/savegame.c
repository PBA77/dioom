#include "dioom.h"

static SaveGameHeader savegame_header(uint64_t saved_at);
static int write_exact(FILE *f, const void *data, size_t size);
static int read_exact(FILE *f, void *data, size_t size);
static int savegame_header_matches(const SaveGameHeader *header);
static int valid_generator_mode(int mode);
static int valid_music_track(int track);
static int save_slot_path(int slot, int temp, char *path, size_t path_size);
static int read_save_slot_header(int slot, SaveGameHeader *out_header);

static SaveGameHeader savegame_header(uint64_t saved_at)
{
    return (SaveGameHeader){
        .magic = SAVEGAME_MAGIC,
        .version = SAVEGAME_VERSION,
        .saved_at = saved_at,
        .payload_size = (uint32_t)(sizeof(SaveGamePayload) + sizeof(StoryProgress) + sizeof(AdaptiveState)),
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
    uint32_t payload_size = header->version == 7u ? (uint32_t)sizeof(SaveGamePayload) :
                            header->version == 8u ? (uint32_t)(sizeof(SaveGamePayload) + sizeof(StoryProgress)) :
                            expected.payload_size;
    return header->magic == expected.magic &&
           (header->version == expected.version || header->version == 8u || header->version == 7u) &&
           header->payload_size == payload_size &&
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

void save_slot_menu_label(int slot, char *out, size_t out_size)
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

int save_runtime_game(Runtime *rt, int slot)
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
             write_exact(f, &payload, sizeof(payload)) &&
             write_exact(f, &story, sizeof(story)) &&
             write_exact(f, &adaptive, sizeof(adaptive));
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
#ifdef __EMSCRIPTEN__
    if (!web_persist_binary_file(path)) {
        fprintf(stderr, "error: cannot persist savegame %s to browser storage\n", path);
        return 0;
    }
#endif
    close_slot_menu(rt);
    return 1;
}

int load_runtime_game(Runtime *rt, int slot)
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
    StoryProgress loaded_story = {0};
    AdaptiveState loaded_adaptive = {.level_hp_scale = 1.0};
    int ok = read_exact(f, &header, sizeof(header)) &&
             savegame_header_matches(&header) &&
             read_exact(f, &payload, sizeof(payload));
    if (ok && header.version >= 8u) ok = read_exact(f, &loaded_story, sizeof(loaded_story));
    if (ok && header.version == SAVEGAME_VERSION) ok = read_exact(f, &loaded_adaptive, sizeof(loaded_adaptive));
    if (ok) ok = fgetc(f) == EOF && !ferror(f);
    if (ok && header.version == 7u) {
        /* Version 7 had no story block: retain collected relics and grant the
         * corresponding completed rituals, without changing the old file. */
        loaded_story.notes_mask = 1u;
        loaded_story.solved_mask = (uint32_t)payload.game.relic_mask & RELIC_MASK_ALL;
        for (int i = 0; i < RELIC_COUNT; ++i) {
            if (loaded_story.solved_mask & (1u << i)) {
                loaded_story.ritual_steps[i] = 3;
                loaded_story.notes_mask |= 1u << (1 + i);
            }
        }
        for (int i = 0; i < count_relics(payload.game.relic_mask); ++i) loaded_story.notes_mask |= 1u << (9 + i);
        int crypt = payload.game.dungeon_relic_index;
        if (crypt >= 0 && crypt < RELIC_COUNT) loaded_story.notes_mask |= 1u << (1 + crypt);
        if (payload.game.victory) loaded_story.notes_mask |= 1u << 13;
        if (payload.game.victory || payload.game.generator_mode == GENERATOR_BOSS) loaded_story.notes_mask |= 1u << STORY_ENTRY_GATE;
    }
    if (ok) {
        ok = !(loaded_story.notes_mask & ~STORY_NOTES_MASK_ALL) && !(loaded_story.solved_mask & ~RELIC_MASK_ALL) &&
             adaptive_state_valid(&loaded_adaptive);
        for (int i = 0; i < RELIC_COUNT && ok; ++i) {
            int solved = (loaded_story.solved_mask & (1u << i)) != 0;
            ok = loaded_story.ritual_steps[i] >= 0 && loaded_story.ritual_steps[i] <= (solved ? 3 : 2) &&
                 (!solved || loaded_story.ritual_steps[i] == 3) && loaded_story.failures[i] >= 0 && loaded_story.failures[i] <= 999;
        }
    }
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
    build_sector_heights(&rt->game);
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
    story = loaded_story;
    adaptive = loaded_adaptive;
    story_popup = -1;
    story_notice_time = 0.0;
    story_dread = 0.0;
    rt->story_panel = 0;
    if (header.version == 7u && !place_story_seals(&rt->game)) return 0;
    moon_visibility_cache_ready = 0;
    torch_flicker_cache_ready = 0;
    sync_relic_progress(&rt->game);
    if (saved_forest.valid) {
        sync_relic_progress(&saved_forest.game);
    }
    set_active_music_track(payload.active_music_track);
    return 1;
}
