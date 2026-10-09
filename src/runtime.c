#include "dioom.h"

static const char *difficulty_config_text(int difficulty);
static int parse_difficulty_name(const char *text, int *out_difficulty);
static int adjust_difficulty(int difficulty, int delta);
static void set_runtime_relative_mouse(Runtime *rt, int enabled);
static int save_runtime_settings(const Runtime *rt);
static void set_runtime_difficulty(Runtime *rt, int difficulty);
static void cycle_runtime_difficulty(Runtime *rt, int delta);
static void adjust_runtime_audio_volume(Runtime *rt, int target_music, int delta);
static void cycle_runtime_render_effects(Runtime *rt, int delta);
static int toggle_runtime_fullscreen(Runtime *rt);
static int set_runtime_resolution(Runtime *rt, int resolution);
static void adjust_runtime_settings_item(Runtime *rt, int item, int delta);
static void open_main_menu(Runtime *rt);
static void open_settings_menu(Runtime *rt);
static void close_settings_menu(Runtime *rt);
static void open_save_menu(Runtime *rt);
static void open_load_menu(Runtime *rt);
static void open_merchant_shop(Runtime *rt);
static void close_merchant_shop(Runtime *rt);
static void activate_merchant_shop_item(Runtime *rt);
static int parse_window_size(const char *text, int *out_w, int *out_h);
static int parse_scale_quality(const char *text, const char **out_quality);
static const char *render_quality_config_text(int quality);
static void init_runtime_config_defaults(RuntimeConfig *config);
static char *trim_ini_text(char *text);
static int parse_ini_int(const char *text, int min_value, int max_value, int *out_value);
static int load_runtime_settings(RuntimeConfig *config);
static int save_settings_file(int difficulty, int quality, int effects, int fullscreen, int sfx_step, int music_step, int trainer, int resolution);
static void shutdown_runtime(Runtime *rt);
static void handle_runtime_keydown(Runtime *rt, SDL_Keycode key, int repeat);

#ifdef __EMSCRIPTEN__
EM_JS(void, present_framebuffer_js, (int width, int height, int ptr), {
    var canvas = Module.canvas || document.getElementById("canvas");
    if (!canvas) return;
    if (canvas.width !== width) canvas.width = width;
    if (canvas.height !== height) canvas.height = height;
    var ctx = Module.dioomCtx;
    if (!ctx) {
        ctx = canvas.getContext("2d");
        Module.dioomCtx = ctx;
    }
    var image = Module.dioomImageData;
    if (!image || image.width !== width || image.height !== height) {
        image = ctx.createImageData(width, height);
        Module.dioomImageData = image;
    }
    var src = HEAPU32.subarray(ptr >>> 2, (ptr >>> 2) + width * height);
    var dst = new Uint32Array(image.data.buffer);
    for (var i = 0; i < src.length; ++i) {
        var c = src[i];
        dst[i] = (c & 0xff00ff00) | ((c & 0x00ff0000) >>> 16) | ((c & 0x000000ff) << 16);
    }
    ctx.putImageData(image, 0, 0);
});

static void present_framebuffer_web(void)
{
    present_framebuffer_js(SCREEN_W, SCREEN_H, (int)(intptr_t)framebuffer);
}

enum {
    WEB_INPUT_NONE = 0,
    WEB_INPUT_W,
    WEB_INPUT_S,
    WEB_INPUT_A,
    WEB_INPUT_D,
    WEB_INPUT_Q,
    WEB_INPUT_E,
    WEB_INPUT_UP,
    WEB_INPUT_DOWN,
    WEB_INPUT_LEFT,
    WEB_INPUT_RIGHT,
    WEB_INPUT_ENTER,
    WEB_INPUT_SPACE,
    WEB_INPUT_ESCAPE,
    WEB_INPUT_1,
    WEB_INPUT_2,
    WEB_INPUT_3,
    WEB_INPUT_TAB,
    WEB_INPUT_H,
    WEB_INPUT_P,
    WEB_INPUT_R,
    WEB_INPUT_F,
    WEB_INPUT_F3,
    WEB_INPUT_F4,
    WEB_INPUT_F8,
    WEB_INPUT_F9,
    WEB_INPUT_F11
};

EM_JS(void, web_input_init, (void), {
    if (Module.dioomInputReady) {
        return;
    }
    Module.dioomInputReady = true;
    Module.dioomKeys = Object.create(null);
    Module.dioomKeyQueue = [];
    const mapCode = function(code) {
        switch (code) {
        case 'KeyW': return 1;
        case 'KeyS': return 2;
        case 'KeyA': return 3;
        case 'KeyD': return 4;
        case 'KeyQ': return 5;
        case 'KeyE': return 6;
        case 'ArrowUp': return 7;
        case 'ArrowDown': return 8;
        case 'ArrowLeft': return 9;
        case 'ArrowRight': return 10;
        case 'Enter':
        case 'NumpadEnter': return 11;
        case 'Space': return 12;
        case 'Escape': return 13;
        case 'Digit1': return 14;
        case 'Digit2': return 15;
        case 'Digit3': return 16;
        case 'Tab': return 17;
        case 'KeyH': return 18;
        case 'KeyP': return 19;
        case 'KeyR': return 20;
        case 'KeyF': return 21;
        case 'F3': return 22;
        case 'F4': return 23;
        case 'F8': return 24;
        case 'F9': return 25;
        case 'F11': return 26;
        default: return 0;
        }
    };
    const shouldCapture = function(id) {
        return id !== 0;
    };
    window.addEventListener('keydown', function(event) {
        const id = mapCode(event.code);
        if (!shouldCapture(id)) {
            return;
        }
        if (!event.repeat) {
            Module.dioomKeyQueue.push(id);
        }
        Module.dioomKeys[id] = 1;
        event.preventDefault();
    }, { passive: false });
    window.addEventListener('keyup', function(event) {
        const id = mapCode(event.code);
        if (!shouldCapture(id)) {
            return;
        }
        Module.dioomKeys[id] = 0;
        event.preventDefault();
    }, { passive: false });
});

EM_JS(int, web_poll_keydown_id, (void), {
    if (!Module.dioomInputReady) {
        return 0;
    }
    return Module.dioomKeyQueue.length ? Module.dioomKeyQueue.shift() : 0;
});

EM_JS(int, web_key_down_id, (int id), {
    return Module.dioomInputReady && Module.dioomKeys[id] ? 1 : 0;
});

EM_JS(void, web_restore_persistent_files, (int slot_count), {
    try {
        var settings = localStorage.getItem('dioom.ini');
        if (settings !== null) {
            FS.writeFile('dioom.ini', settings);
        }
        var base64ToBytes = function(text) {
            var binary = atob(text);
            var bytes = new Uint8Array(binary.length);
            for (var i = 0; i < binary.length; ++i) {
                bytes[i] = binary.charCodeAt(i);
            }
            return bytes;
        };
        for (var slot = 1; slot <= slot_count; ++slot) {
            var path = 'dioom_slot' + slot + '.sav';
            var data = localStorage.getItem(path);
            if (data !== null) {
                FS.writeFile(path, base64ToBytes(data));
            }
        }
    } catch (error) {
        console.error('warning: cannot restore persistent browser files: ' + error);
    }
});

EM_JS(int, web_persist_text_file, (const char *path_ptr), {
    try {
        var path = UTF8ToString(path_ptr);
        var text = FS.readFile(path, { encoding: 'utf8' });
        localStorage.setItem(path, text);
        return 1;
    } catch (error) {
        console.error('error: cannot persist text file to browser storage: ' + error);
        return 0;
    }
});

EM_JS(int, web_persist_binary_file, (const char *path_ptr), {
    try {
        var path = UTF8ToString(path_ptr);
        var bytes = FS.readFile(path);
        var text = "";
        var chunk_size = 0x8000;
        for (var i = 0; i < bytes.length; i += chunk_size) {
            text += String.fromCharCode.apply(null, bytes.subarray(i, i + chunk_size));
        }
        localStorage.setItem(path, btoa(text));
        return 1;
    } catch (error) {
        console.error('error: cannot persist binary file to browser storage: ' + error);
        return 0;
    }
});

static SDL_Keycode web_input_keycode(int id)
{
    switch (id) {
    case WEB_INPUT_W: return SDLK_w;
    case WEB_INPUT_S: return SDLK_s;
    case WEB_INPUT_A: return SDLK_a;
    case WEB_INPUT_D: return SDLK_d;
    case WEB_INPUT_Q: return SDLK_q;
    case WEB_INPUT_E: return SDLK_e;
    case WEB_INPUT_UP: return SDLK_UP;
    case WEB_INPUT_DOWN: return SDLK_DOWN;
    case WEB_INPUT_LEFT: return SDLK_LEFT;
    case WEB_INPUT_RIGHT: return SDLK_RIGHT;
    case WEB_INPUT_ENTER: return SDLK_RETURN;
    case WEB_INPUT_SPACE: return SDLK_SPACE;
    case WEB_INPUT_ESCAPE: return SDLK_ESCAPE;
    case WEB_INPUT_1: return SDLK_1;
    case WEB_INPUT_2: return SDLK_2;
    case WEB_INPUT_3: return SDLK_3;
    case WEB_INPUT_TAB: return SDLK_TAB;
    case WEB_INPUT_H: return SDLK_h;
    case WEB_INPUT_P: return SDLK_p;
    case WEB_INPUT_R: return SDLK_r;
    case WEB_INPUT_F: return SDLK_f;
    case WEB_INPUT_F3: return SDLK_F3;
    case WEB_INPUT_F4: return SDLK_F4;
    case WEB_INPUT_F8: return SDLK_F8;
    case WEB_INPUT_F9: return SDLK_F9;
    case WEB_INPUT_F11: return SDLK_F11;
    default: return SDLK_UNKNOWN;
    }
}
#endif

int screen_height = INITIAL_SCREEN_H;

int screen_width = INITIAL_SCREEN_W;

int normalize_difficulty(int difficulty)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT) {
        return DIFFICULTY_NORMAL;
    }
    return difficulty;
}

const char *difficulty_menu_text(int difficulty)
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

int parse_resolution(const char *text, int *out_resolution)
{
    if (strcmp(text, "640x480") == 0) *out_resolution = 0;
    else if (strcmp(text, "1280x960") == 0) *out_resolution = 1;
    else return 0;
    return 1;
}

static void set_runtime_relative_mouse(Runtime *rt, int enabled)
{
    enabled = enabled ? 1 : 0;
    if (rt->relative_mouse == enabled) {
        return;
    }
#ifdef __EMSCRIPTEN__
    rt->relative_mouse = enabled;
#else
    if (SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) != 0) {
        fprintf(stderr, "warning: SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
        return;
    }
    rt->relative_mouse = enabled;
    SDL_ShowCursor(enabled ? SDL_DISABLE : SDL_ENABLE);
#endif
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
                              rt->trainer,
                              SCREEN_W == 1280);
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

static void cycle_runtime_render_effects(Runtime *rt, int delta)
{
    rt->render_effects = normalize_render_effects(rt->render_effects);
    rt->render_effects = (rt->render_effects + delta + RENDER_EFFECTS_COUNT) % RENDER_EFFECTS_COUNT;
    save_runtime_settings(rt);
}

static int toggle_runtime_fullscreen(Runtime *rt)
{
    int next_fullscreen = !rt->fullscreen;
#ifdef __EMSCRIPTEN__
    rt->fullscreen = next_fullscreen;
    save_runtime_settings(rt);
    return 1;
#else
    if (SDL_SetWindowFullscreen(rt->window, next_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return 0;
    }
    rt->fullscreen = next_fullscreen;
    save_runtime_settings(rt);
    return 1;
#endif
}

static int set_runtime_resolution(Runtime *rt, int resolution)
{
    int w = resolution ? 1280 : 640, h = resolution ? 960 : 480;
#ifndef __EMSCRIPTEN__
    SDL_Texture *next = SDL_CreateTexture(rt->renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!next) {
        fprintf(stderr, "error: cannot create %dx%d render texture: %s\n", w, h, SDL_GetError());
        return 0;
    }
    if (SDL_RenderSetLogicalSize(rt->renderer, w, h) != 0) {
        fprintf(stderr, "error: cannot set %dx%d logical size: %s\n", w, h, SDL_GetError());
        SDL_DestroyTexture(next);
        return 0;
    }
    SDL_DestroyTexture(rt->screen);
    rt->screen = next;
#endif
    SCREEN_W = w;
    SCREEN_H = h;
    vignette_ready = 0;
    save_runtime_settings(rt);
    return 1;
}

static void adjust_runtime_settings_item(Runtime *rt, int item, int delta)
{
    if (item == SETTINGS_MENU_ITEM_DIFFICULTY) {
        cycle_runtime_difficulty(rt, delta);

    } else if (item == SETTINGS_MENU_ITEM_POST) {
        cycle_runtime_render_effects(rt, delta);
    } else if (item == SETTINGS_MENU_ITEM_SFX_VOLUME) {
        adjust_runtime_audio_volume(rt, 0, delta);
    } else if (item == SETTINGS_MENU_ITEM_MUSIC_VOLUME) {
        adjust_runtime_audio_volume(rt, 1, delta);
    } else if (item == SETTINGS_MENU_ITEM_RESOLUTION) {
        if (!set_runtime_resolution(rt, SCREEN_W != 1280)) rt->running = 0;
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

void close_slot_menu(Runtime *rt)
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

int parse_render_quality(const char *text, int *out_quality)
{
    if (strcmp(text, "pbr") == 0) {
        fprintf(stderr, "note: PBR was removed; migrating quality=pbr to fast lighting.\n");
        *out_quality = RENDER_QUALITY_FAST;
        return 1;
    }
    if (strcmp(text, "fast") == 0) {
        *out_quality = RENDER_QUALITY_FAST;
        return 1;
    }
    return 0;
}

int parse_render_effects(const char *text, int *out_effects)
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
    (void)quality;
    return "fast";
}

const char *render_effects_config_text(int effects)
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
#ifdef __EMSCRIPTEN__
    config->window_w = SCREEN_W;
    config->window_h = SCREEN_H;
#else
    config->window_w = SCREEN_W * WINDOW_SCALE;
    config->window_h = SCREEN_H * WINDOW_SCALE;
#endif
    config->resolution = SCREEN_W == 1280;
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
        if (strcmp(key, "resolution") == 0) {
            parsed = parse_resolution(value, &config->resolution);
            if (!parsed) {
                fprintf(stderr, "error: %s:%d resolution must be 640x480 or 1280x960\n", SETTINGS_PATH, line_no);
                fclose(f);
                return 0;
            }
        } else if (strcmp(key, "quality") == 0) {
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

static int save_settings_file(int difficulty, int quality, int effects, int fullscreen, int sfx_step, int music_step, int trainer, int resolution)
{
    FILE *f = fopen(SETTINGS_PATH, "w");
    if (!f) {
        fprintf(stderr, "error: cannot write %s: %s\n", SETTINGS_PATH, strerror(errno));
        return 0;
    }
    fprintf(f, "resolution=%s\n", resolution ? "1280x960" : "640x480");
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
#ifdef __EMSCRIPTEN__
    if (!web_persist_text_file(SETTINGS_PATH)) {
        fprintf(stderr, "error: cannot persist %s to browser storage\n", SETTINGS_PATH);
        return 0;
    }
#endif
    return 1;
}

int parse_runtime_config(int argc, char **argv, RuntimeConfig *config)
{
    init_runtime_config_defaults(config);
    if (!load_runtime_settings(config)) return 0;
#ifdef __EMSCRIPTEN__
    config->fullscreen = 0;
#endif

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--resolution") == 0) {
            if (i + 1 >= argc || !parse_resolution(argv[i + 1], &config->resolution)) {
                fprintf(stderr, "error: --resolution expects 640x480 or 1280x960\n");
                return 0;
            }
            i += 1;
        } else if (strcmp(argv[i], "--window") == 0) {
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
                fprintf(stderr, "error: --quality expects fast (legacy pbr is migrated)\n");
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

int init_runtime(Runtime *rt, const RuntimeConfig *config)
{
    memset(rt, 0, sizeof(*rt));
    SCREEN_W = config->resolution ? 1280 : 640;
    SCREEN_H = config->resolution ? 960 : 480;
    vignette_ready = 0;

    if (!init_assets()) {
        return 0;
    }

#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_AUTO_UPDATE_JOYSTICKS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG, "");
    SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, "");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
#ifdef __EMSCRIPTEN__
    SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
    SDL_EventState(SDL_JOYDEVICEADDED, SDL_IGNORE);
    SDL_EventState(SDL_JOYDEVICEREMOVED, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_IGNORE);
#endif
    if (!init_audio()) {
        SDL_Quit();
        return 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, config->scale_quality);

    Uint32 window_flags = SDL_WINDOW_SHOWN;
#ifndef __EMSCRIPTEN__
    window_flags |= SDL_WINDOW_RESIZABLE;
#endif
    rt->window = SDL_CreateWindow(
        "Dioom",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_w,
        config->window_h,
        window_flags);
    if (!rt->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        shutdown_audio();
        SDL_Quit();
        return 0;
    }

#ifndef __EMSCRIPTEN__
    Uint32 renderer_flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC;
    rt->renderer = SDL_CreateRenderer(
        rt->window,
        -1,
        renderer_flags);
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
#endif

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
#ifdef __EMSCRIPTEN__
        rt->fullscreen = 1;
#else
        if (SDL_SetWindowFullscreen(rt->window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
            fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
            shutdown_runtime(rt);
            return 0;
        }
        rt->fullscreen = 1;
#endif
    }
    reset_run(&rt->game, &rt->cam);
    rt->settings_ready = 1;
    rt->prev = SDL_GetPerformanceCounter();
#ifdef __EMSCRIPTEN__
    web_input_init();
#endif
    return 1;
}

static void handle_runtime_keydown(Runtime *rt, SDL_Keycode key, int repeat)
{
    if (rt->story_panel) {
        if (repeat) return;
        if (key == SDLK_ESCAPE || (key == SDLK_j && rt->story_panel == 3)) {
            rt->story_panel = 0;
            rt->paused = 0;
        } else if (key == SDLK_j) {
            rt->story_panel = 3;
            rt->story_entry = rt->game.dungeon_relic_index >= 0 ? 1 + rt->game.dungeon_relic_index : 0;
            if (!(story.notes_mask & (1u << rt->story_entry))) rt->story_entry = 0;
        } else if (rt->story_panel == 3 && (key == SDLK_a || key == SDLK_LEFT || key == SDLK_d || key == SDLK_RIGHT)) {
            int delta = key == SDLK_a || key == SDLK_LEFT ? -1 : 1;
            for (int i = 0; i < STORY_ENTRY_COUNT; ++i) {
                rt->story_entry = (rt->story_entry + delta + STORY_ENTRY_COUNT) % STORY_ENTRY_COUNT;
                if (story.notes_mask & (1u << rt->story_entry)) break;
            }
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (rt->story_panel == 2) invoke_story_seal(&rt->game, &rt->cam, rt->story_seal);
            rt->story_panel = 0;
            rt->paused = 0;
        }
        return;
    }
    if (rt->shop_open) {
        if ((key == SDLK_w || key == SDLK_UP) && repeat == 0) {
            rt->shop_selected = (rt->shop_selected + SHOP_ITEM_COUNT - 1) % SHOP_ITEM_COUNT;
        } else if ((key == SDLK_s || key == SDLK_DOWN) && repeat == 0) {
            rt->shop_selected = (rt->shop_selected + 1) % SHOP_ITEM_COUNT;
        } else if ((key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) && repeat == 0) {
            activate_merchant_shop_item(rt);
        } else if ((key == SDLK_ESCAPE || key == SDLK_e) && repeat == 0) {
            close_merchant_shop(rt);
            play_sfx(SFX_DOOR, 0.30);
        }
        return;
    }
    int help_closed = rt->menu_open ? 0 : close_help_on_key(&rt->game);
    if (rt->menu_open) {
        int menu_count = menu_item_count_for_page(rt->menu_page);
        if ((key == SDLK_w || key == SDLK_UP) && repeat == 0) {
            rt->menu_selected = (rt->menu_selected + menu_count - 1) % menu_count;
        } else if ((key == SDLK_s || key == SDLK_DOWN) && repeat == 0) {
            rt->menu_selected = (rt->menu_selected + 1) % menu_count;
        } else if (rt->menu_page == MENU_PAGE_SETTINGS &&
                   (key == SDLK_a || key == SDLK_LEFT || key == SDLK_d || key == SDLK_RIGHT) &&
                   repeat == 0) {
            int delta = (key == SDLK_a || key == SDLK_LEFT) ? -1 : 1;
            adjust_runtime_settings_item(rt, rt->menu_selected, delta);
        } else if ((key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) && repeat == 0) {
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
                        story_discover(0);
                    }
                    rt->menu_open = 0;
                    rt->paused = 0;
                    rt->game_started = 1;
                } else if (rt->menu_selected == MAIN_MENU_ITEM_RESTART) {
                    reset_run(&rt->game, &rt->cam);
                    story_discover(0);
                    rt->menu_open = 0;
                    rt->paused = 0;
                    rt->game_started = 1;
                } else if (rt->menu_selected == MAIN_MENU_ITEM_SAVE) {
                    open_save_menu(rt);
                } else if (rt->menu_selected == MAIN_MENU_ITEM_LOAD) {
                    open_load_menu(rt);
                } else if (rt->menu_selected == MAIN_MENU_ITEM_SETTINGS) {
                    open_settings_menu(rt);
#ifndef __EMSCRIPTEN__
                } else if (rt->menu_selected == MAIN_MENU_ITEM_EXIT) {
                    rt->running = 0;
#endif
                }
            }
        } else if (key == SDLK_ESCAPE && repeat == 0) {
            if (rt->menu_page == MENU_PAGE_SETTINGS) {
                close_settings_menu(rt);
            } else if (rt->menu_page == MENU_PAGE_SAVE || rt->menu_page == MENU_PAGE_LOAD) {
                close_slot_menu(rt);
            } else if (rt->game_started) {
                rt->menu_open = 0;
                rt->paused = 0;
            }
        } else if (key == SDLK_F3 && repeat == 0) {
            rt->show_fps = !rt->show_fps;
            rt->fps_accum = 0.0;
            rt->fps_frames = 0;
        } else if (key == SDLK_F4 && repeat == 0) {
            rt->show_timings = !rt->show_timings;
        } else if (key == SDLK_F11 && repeat == 0) {
            if (!toggle_runtime_fullscreen(rt)) {
                rt->running = 0;
            }
        } else if (key == SDLK_F8 && repeat == 0) {
            open_save_menu(rt);
        } else if (key == SDLK_F9 && repeat == 0) {
            open_load_menu(rt);
        }
        return;
    }
    if (key == SDLK_ESCAPE) {
        open_main_menu(rt);
    } else if (key == SDLK_1) {
        select_weapon(&rt->game, WEAPON_KNIFE);
    } else if (key == SDLK_2) {
        select_weapon(&rt->game, WEAPON_PISTOL);
    } else if (key == SDLK_3 && repeat == 0) {
        select_weapon(&rt->game, WEAPON_FIREBALL);
    } else if (key == SDLK_SPACE) {
        if (!rt->paused) {
            player_fire(&rt->game, &rt->cam);
        }
    } else if (key == SDLK_j && repeat == 0 && !rt->paused && !rt->game.game_over) {
        story_discover(0);
        int entry = rt->game.dungeon_relic_index >= 0 ? rt->game.dungeon_relic_index + 1 : 0;
        if (!(story.notes_mask & (1u << entry))) entry = 0;
        story_popup = -1;
        open_story_panel(rt, 3, entry, -1);
    } else if (key == SDLK_f && repeat == 0) {
        if (!rt->paused) {
            int seal = active_seal_index(&rt->game, &rt->cam);
            if (seal >= 0) open_story_panel(rt, 2, -1, seal);
            else interact_world(&rt->game, &rt->cam);
        }
    } else if (key == SDLK_e && repeat == 0) {
        if (!rt->paused && active_merchant_house_prompt(&rt->game, &rt->cam)) {
            open_merchant_shop(rt);
            play_sfx(SFX_DOOR, 0.36);
        }
    } else if (key == SDLK_TAB && repeat == 0) {
        rt->game.show_automap = !rt->game.show_automap;
    } else if (key == SDLK_h && repeat == 0) {
        if (!help_closed) {
            rt->game.show_help = 1;
            rt->game.help_timer = 0.0;
        }
    } else if (key == SDLK_p && repeat == 0) {
        rt->paused = !rt->paused;
    } else if (key == SDLK_r && repeat == 0) {
        reset_run(&rt->game, &rt->cam);
        story_discover(0);
        rt->paused = 0;
        rt->game_started = 1;
    } else if (key == SDLK_F3 && repeat == 0) {
        rt->show_fps = !rt->show_fps;
        rt->fps_accum = 0.0;
        rt->fps_frames = 0;
    } else if (key == SDLK_F4 && repeat == 0) {
        rt->show_timings = !rt->show_timings;
    } else if (key == SDLK_F11 && repeat == 0) {
        if (!toggle_runtime_fullscreen(rt)) {
            rt->running = 0;
        }
    } else if (key == SDLK_F8 && repeat == 0) {
        open_save_menu(rt);
    } else if (key == SDLK_F9 && repeat == 0) {
        open_load_menu(rt);
    }
}

void runtime_frame(void *userdata)
{
    Runtime *rt = (Runtime *)userdata;
#ifdef __EMSCRIPTEN__
    int web_key_id = WEB_INPUT_NONE;
    while ((web_key_id = web_poll_keydown_id()) != WEB_INPUT_NONE) {
        SDL_Keycode key = web_input_keycode(web_key_id);
        if (key != SDLK_UNKNOWN) {
            handle_runtime_keydown(rt, key, 0);
        }
    }
#else
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            rt->running = 0;
        } else if (event.type == SDL_KEYDOWN) {
            handle_runtime_keydown(rt, event.key.keysym.sym, event.key.repeat);
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
#endif

    if (!rt->running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        shutdown_runtime(rt);
        return;
    }

    consume_story_popup(rt);
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

    double forward = 0.0;
    double strafe = 0.0;
    double turn = 0.0;

#ifdef __EMSCRIPTEN__
    if (web_key_down_id(WEB_INPUT_W) || web_key_down_id(WEB_INPUT_UP)) forward += 1.0;
    if (web_key_down_id(WEB_INPUT_S) || web_key_down_id(WEB_INPUT_DOWN)) forward -= 1.0;
    if (web_key_down_id(WEB_INPUT_A) || web_key_down_id(WEB_INPUT_Q)) strafe -= 1.0;
    if (web_key_down_id(WEB_INPUT_D) || web_key_down_id(WEB_INPUT_E)) strafe += 1.0;
    if (web_key_down_id(WEB_INPUT_LEFT)) turn -= 1.0;
    if (web_key_down_id(WEB_INPUT_RIGHT)) turn += 1.0;
#else
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) forward += 1.0;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) forward -= 1.0;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_Q]) strafe -= 1.0;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_E]) strafe += 1.0;
    if (keys[SDL_SCANCODE_LEFT]) turn -= 1.0;
    if (keys[SDL_SCANCODE_RIGHT]) turn += 1.0;
#endif

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
    consume_story_popup(rt);
    render_quality = rt->render_quality;
    render_effects = rt->render_effects;
    active_profile = rt->show_timings ? &rt->profile : NULL;
    render_scene(&rt->cam, &rt->game);
    active_profile = NULL;
    render_quality = RENDER_QUALITY_FAST;
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
    } else if (rt->story_panel) {
        render_story_panel(rt);
    } else if (rt->paused) {
        render_pause_overlay();
    }
    if (rt->show_fps) {
        render_fps_overlay(rt->fps_value, rt->frame_ms, rt->render_quality, rt->render_effects);
    }
    if (rt->show_timings) {
        render_timing_overlay(&rt->profile);
    }
#ifdef __EMSCRIPTEN__
    present_framebuffer_web();
#else
    SDL_UpdateTexture(rt->screen, NULL, framebuffer, SCREEN_W * (int)sizeof(uint32_t));

    SDL_RenderClear(rt->renderer);
    SDL_RenderCopy(rt->renderer, rt->screen, NULL, NULL);
    SDL_RenderPresent(rt->renderer);
#endif
}
