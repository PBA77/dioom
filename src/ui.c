#include "dioom.h"
#include "prompt_font.h"

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
static void draw_weapon_sprite(int sprite, int x, int y, int size);
static void hud_stat(int column, const char *label, const char *value, uint32_t color);
static uint8_t prompt_glyph(char c, int row);
static void draw_prompt_text(int x, int y, const char *text, uint32_t color);
static int prompt_text_width(const char *text);
static void draw_timing_line(int y, const char *label, double ms, uint32_t color);
static const Portal *active_portal_prompt(const GameState *game, const Camera *cam);
static const Door *active_door_prompt(const GameState *game, const Camera *cam);
static void render_interaction_label(const char *label);
static void render_centered_prompt_line_styled(int y, const char *text, uint32_t color, int underline, int strike);
static void render_centered_prompt_line(int y, const char *text, uint32_t color);
static const char *merchant_shop_item_name(int item);
static int merchant_shop_item_price(int item);
static int merchant_shop_item_full(const GameState *game, int item);
static void render_merchant_shop_row(const GameState *game, int item, int selected, int y);
static void render_centered_menu_text(int y, const char *text, uint32_t color);
static void render_menu_item(int y, const char *text, int selected, uint32_t color);
static void render_menu_slider(int y, const char *label, int value, int selected, uint32_t color);
static const char *menu_item_description(int page, int item);

void fill_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            put_pixel(xx, yy, color);
        }
    }
}

void render_hit_flash(const GameState *game)
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

void render_player_damage_feedback(const Camera *cam, const GameState *game)
{
    if (game->player_damage_flash <= 0.0) {
        return;
    }

    double t = clamp01(game->player_damage_flash / PLAYER_DAMAGE_FLASH_TIME);
    blend_rect(0, 0, SCREEN_W, SCREEN_H - HUD_HEIGHT, rgb(160, 8, 8), t * 0.18);

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

void render_crosshair(const GameState *game)
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

void render_shot_trace(const GameState *game)
{
    if (game->shot_trace <= 0.0 ||
        (game->selected_weapon != WEAPON_PISTOL && game->selected_weapon != WEAPON_SHOTGUN)) {
        return;
    }

    double progress = 1.0 - clamp01(game->shot_trace / WEAPON_FLASH_TIME);
    int muzzle_x = SCREEN_W / 2 + 31;
    int muzzle_y = SCREEN_H - HUD_HEIGHT - 108 * SCREEN_H / 480;
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

void render_weapon(const GameState *game)
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

    int view_scale = SCREEN_H / 480;
    size = size * 2 * view_scale;
    int x = SCREEN_W / 2 - size / 2 + (recoil_x + offset_x) * view_scale;
    int y = SCREEN_H - HUD_HEIGHT - size + (bob + recoil_y + offset_y + 12) * view_scale;
    draw_weapon_sprite(sprite, x, y, size);
}

void draw_scaled_text(int x, int y, const char *text, uint32_t color, int scale)
{
    for (const char *p = text; *p; ++p) {
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = prompt_font_glyph(*p, row);
            for (int col = 0; col < 5; ++col) {
                if (bits & (1u << (4 - col))) fill_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
        x += 6 * scale;
    }
}

static void hud_stat(int column, const char *label, const char *value, uint32_t color)
{
    int scale = SCREEN_H / 480;
    int center = (column * 128 + 64) * scale;
    int y = SCREEN_H - HUD_HEIGHT;
    draw_scaled_text(center - (int)strlen(label) * 3 * scale, y + 10 * scale, label, rgb(194, 186, 163), scale);
    draw_scaled_text(center - (int)strlen(value) * 9 * scale, y + 27 * scale, value, rgb(32, 12, 10), 3 * scale);
    draw_scaled_text(center - (int)strlen(value) * 9 * scale - scale, y + 26 * scale, value, color, 3 * scale);
}

void render_hud(const GameState *game)
{
    int scale = SCREEN_H / 480;
    int top = SCREEN_H - HUD_HEIGHT;
    for (int y = top; y < SCREEN_H; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            framebuffer[y * SCREEN_W + x] = hud_background[((y - top) / scale) * 640 + x / scale];
        }
    }
    char value[24];
    snprintf(value, sizeof(value), "%d", game->player_health);
    hud_stat(0, "HEALTH", value, game->player_health > 30 ? rgb(224, 50, 36) : rgb(255, 96, 54));
    int ammo = game->selected_weapon == WEAPON_FIREBALL ? game->fireball_ammo : game->ammo;
    if (game->selected_weapon == WEAPON_KNIFE) snprintf(value, sizeof(value), "--");
    else snprintf(value, sizeof(value), "%d", ammo);
    hud_stat(1, "AMMO", value, rgb(224, 50, 36));
    const char *weapon = game->selected_weapon == WEAPON_KNIFE ? "KNIFE" :
                         game->selected_weapon == WEAPON_FIREBALL ? "FIRE" :
                         game->selected_weapon == WEAPON_SHOTGUN ? "SGUN" : "GUN";
    hud_stat(2, "WEAPON", weapon, rgb(216, 190, 116));
    snprintf(value, sizeof(value), "%d/%d", game->relic_count, RELIC_COUNT);
    hud_stat(3, "RELICS", value, rgb(216, 190, 116));
    snprintf(value, sizeof(value), "%d", game->gold);
    hud_stat(4, "GOLD", value, rgb(216, 190, 116));
    snprintf(value, sizeof(value), "KEYS %d", game->keys);
    draw_scaled_text(400 * scale, top + 52 * scale, value, rgb(206, 184, 120), scale);
    snprintf(value, sizeof(value), "KILLS %d", game->kills);
    draw_scaled_text(534 * scale, top + 52 * scale, value, rgb(206, 184, 120), scale);
    if (game->rapid_timer > 0.0) fill_rect(6 * scale, top - 5 * scale, (int)(game->rapid_timer * 4.0) * scale, 3 * scale, rgb(48, 180, 230));
    if (game->damage_timer > 0.0) fill_rect(6 * scale, top - 10 * scale, (int)(game->damage_timer * 4.0) * scale, 3 * scale, rgb(190, 72, 230));
    if (game->pickup_flash > 0.0) fill_rect(SCREEN_W / 2 - 18 * scale, top - 8 * scale, 36 * scale, 4 * scale, rgb(220, 210, 96));
    if (game->game_over) draw_scaled_text(SCREEN_W / 2 - 54 * scale, 32 * scale, "YOU DIED", rgb(224, 50, 36), 3 * scale);
}

void render_minimap(const Camera *cam, const GameState *game)
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
                         item->type == ITEM_SEAL ? rgb(190, 130, 214) :
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

void render_full_automap(const Camera *cam, const GameState *game)
{
    const int cell = 7;
    const int ox = (SCREEN_W - MAP_W * cell) / 2;
    const int oy = (SCREEN_H - HUD_HEIGHT - MAP_H * cell) / 2;

    fill_rect(0, 0, SCREEN_W, SCREEN_H - HUD_HEIGHT, rgb(4, 5, 6));
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
                         item->type == ITEM_SEAL ? rgb(190, 130, 214) :
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

void render_pause_overlay(void)
{
    fill_rect(SCREEN_W / 2 - 42, 52, 84, 34, rgb(10, 10, 12));
    fill_rect(SCREEN_W / 2 - 24, 60, 12, 18, rgb(190, 166, 100));
    fill_rect(SCREEN_W / 2 + 12, 60, 12, 18, rgb(190, 166, 100));
    fill_rect(SCREEN_W / 2 - 32, 90, 64, 4, rgb(84, 72, 50));
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

void blend_rect(int x, int y, int w, int h, uint32_t color, double amount)
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

void render_fps_overlay(double fps, double frame_ms, int quality, int effects)
{
    char fps_text[16];
    char ms_text[16];
    (void)quality;
    const char *quality_text = "FAST";
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

void render_timing_overlay(const RenderProfile *profile)
{
    blend_rect(4, 78, 142, 184, rgb(0, 0, 0), 0.54);
    draw_timing_line(83, "FLOOR", profile->floor_ms, rgb(230, 226, 190));
    draw_timing_line(99, "WALL", profile->wall_ms, rgb(230, 226, 190));
    draw_timing_line(115, "SPRITE", profile->sprite_ms, rgb(210, 218, 184));
    draw_timing_line(131, "FOG", profile->fog_ms, rgb(176, 196, 174));
    draw_timing_line(147, "BLOOM", profile->bloom_ms, rgb(218, 176, 130));
    draw_timing_line(163, "POST", profile->post_ms, rgb(196, 188, 176));
    draw_timing_line(179, "TOTAL", profile->total_ms, rgb(238, 210, 146));
    char text[32];
    snprintf(text, sizeof(text), "SKILL %+.2f", adaptive.skill);
    draw_prompt_text(9, 199, text, rgb(190, 214, 190));
    snprintf(text, sizeof(text), "HP X%.2f", adaptive.level_hp_scale);
    draw_prompt_text(9, 215, text, rgb(190, 214, 190));
    snprintf(text, sizeof(text), "DMG X%.2f", adaptive_damage_scale());
    draw_prompt_text(9, 231, text, rgb(190, 214, 190));
    snprintf(text, sizeof(text), "CD X%.2f D%d", adaptive_cooldown_scale(), adaptive.deaths);
    draw_prompt_text(9, 247, text, rgb(190, 214, 190));
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

const House *active_house_prompt(const GameState *game, const Camera *cam, int *out_index)
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

int is_merchant_house_index(int house_index)
{
    return house_index == 0;
}

const House *active_merchant_house_prompt(const GameState *game, const Camera *cam)
{
    int house_index = -1;
    const House *house = active_house_prompt(game, cam, &house_index);
    if (!house || !is_merchant_house_index(house_index)) {
        return NULL;
    }
    return house;
}

int active_prop_index(const GameState *game, const Camera *cam)
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
        int note = story_prop_entry(game, prop);
        if (!prop->active || prop->loot_slot < 0 || (prop->looted && (note < 0 || (story.notes_mask & (1u << note))))) {
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
    int y = SCREEN_H - HUD_HEIGHT - 28;
    blend_rect(x - 7, y - 5, w + 14, 24, rgb(4, 4, 5), 0.34);
    blend_rect(x - 5, y - 3, w + 10, 20, rgb(62, 44, 24), 0.20);
    draw_prompt_text(x + 1, y + 1, label, rgb(74, 56, 36));
    draw_prompt_text(x, y, label, rgb(220, 202, 150));
}

void render_interaction_prompt(const Camera *cam, const GameState *game)
{
    int seal = active_seal_index(game, cam);
    if (seal >= 0) {
        char label[48];
        snprintf(label, sizeof(label), "F ZBADAJ %s", seal_names[game->items[seal].relic_index]);
        render_interaction_label(label);
        return;
    }
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
            render_interaction_label("E HANDEL / F LIST DZWONNIKA");
            return;
        }
        render_interaction_label("F WEJDZ");
        return;
    }

    int prop_index = active_prop_index(game, cam);
    if (prop_index >= 0) {
        render_interaction_label(game->props[prop_index].looted ? "F ODCZYTAJ ZAPIS" : "F PRZESZUKAJ");
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

void render_merchant_shop_screen(const GameState *game, int selected)
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

void render_victory_screen(void)
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
    render_centered_prompt_line(y + 14, "CISZA PO DZWONIE", rgb(246, 224, 158));
    render_centered_prompt_line(y + 48, "GLOSY SA WOLNE", rgb(238, 178, 54));
    render_centered_prompt_line(y + 72, "J EPILOG W DZIENNIKU", rgb(218, 202, 160));
    render_centered_prompt_line(y + 102, "R RESTART ESC MENU", rgb(150, 132, 100));
}

void render_boss_bar(const GameState *game)
{
    const Monster *boss = engaged_boss(game);
    if (!boss) return;
    int scale = SCREEN_H / 480;
    int max_hp = scale_monster_hp_for_difficulty(BOSS_HP, game->difficulty);
    int w = 240 * scale, h = 7 * scale;
    int x = SCREEN_W / 2 - w / 2, y = 30 * scale;
    int fill = max_hp > 0 ? (int)((long)w * boss->hp / max_hp) : 0;
    if (fill < 0) fill = 0;
    if (fill > w) fill = w;
    int phase = boss_phase(game, boss);
    uint32_t color = phase == 2 ? rgb(236, 92, 40) : (phase == 1 ? rgb(214, 128, 44) : rgb(176, 36, 30));
    blend_rect(x - 6 * scale, y - 20 * scale, w + 12 * scale, h + 26 * scale, rgb(4, 4, 6), 0.6);
    render_centered_prompt_line(y - 17 * scale, phase == 2 ? "STRAZNIK BRAMY - GLOSY" : (phase == 1 ? "STRAZNIK BRAMY - KORZENIE" : "STRAZNIK BRAMY"), rgb(230, 190, 116));
    fill_rect(x - scale, y - scale, w + 2 * scale, h + 2 * scale, rgb(58, 42, 30));
    fill_rect(x, y, w, h, rgb(24, 14, 12));
    if (fill > 0) fill_rect(x, y, fill, h, color);
    for (int i = 1; i < BOSS_PHASE_COUNT; ++i) fill_rect(x + w * i / BOSS_PHASE_COUNT, y, scale, h, rgb(10, 8, 8));
}

void render_help_overlay(const GameState *game)
{
    if (game->help_timer <= 0.0 && !game->show_help) return;
    if (engaged_boss(game)) return;
    int relic = game->dungeon_relic_index;
    const char *goal = game->boss_unlocked ? "OTWORZ BRAME W LESIE" : "BADAJ DOMY I KRYPTY";
    if (game->generator_mode == GENERATOR_BOSS) goal = "ODBIERZ GLOSY STRAZNIKOWI";
    char progress[40];
    if (relic >= 0 && relic < RELIC_COUNT) {
        goal = (story.solved_mask & (1u << relic)) ? "ODSZUKAJ RELIKWIE" : "ZBADAJ TRZY KAMIENIE";
        snprintf(progress, sizeof(progress), "PIECZEC %d/3", story.ritual_steps[relic]);
    } else snprintf(progress, sizeof(progress), "DZWON %d/%d", game->relic_count, RELIC_COUNT);
    blend_rect(SCREEN_W / 2 - 164, 16, 328, 90, rgb(4, 5, 6), 0.64);
    render_centered_prompt_line(24, "CEL", rgb(230, 190, 116));
    render_centered_prompt_line(44, goal, rgb(218, 202, 160));
    render_centered_prompt_line(64, progress, rgb(188, 164, 122));
    render_centered_prompt_line(84, "J DZIENNIK  H CEL", rgb(158, 142, 108));
}

static void render_centered_menu_text(int y, const char *text, uint32_t color)
{
    int scale = 2 * SCREEN_H / 480;
    int x = SCREEN_W / 2 - (int)strlen(text) * 3 * scale;
    draw_scaled_text(x + scale / 2, y + scale / 2, text, rgb(12, 8, 8), scale);
    draw_scaled_text(x, y, text, color, scale);
}

static void render_menu_item(int y, const char *text, int selected, uint32_t color)
{
    int scale = SCREEN_H / 480;
    int item_w = 276 * scale, item_h = 26 * scale;
    int item_x = SCREEN_W / 2 - item_w / 2;
    if (selected) {
        blend_rect(item_x, y - 5 * scale, item_w, item_h, rgb(106, 20, 16), 0.65);
        fill_rect(item_x - 8 * scale, y + 2 * scale, 4 * scale, 12 * scale, rgb(232, 56, 38));
    }
    render_centered_menu_text(y, text, color);
}

static void render_menu_slider(int y, const char *label, int value, int selected, uint32_t color)
{
    int scale = SCREEN_H / 480;
    int item_w = 276 * scale, item_x = SCREEN_W / 2 - item_w / 2;
    int bar_x = item_x + 120 * scale;
    if (selected) blend_rect(item_x, y - 5 * scale, item_w, 26 * scale, rgb(106, 20, 16), 0.65);
    draw_scaled_text(item_x + 12 * scale, y, label, color, 2 * scale);
    value = clamp_volume_step(value);
    for (int i = 0; i < AUDIO_VOLUME_STEPS; ++i) {
        fill_rect(bar_x + i * 11 * scale, y + 6 * scale, 8 * scale, 7 * scale,
                  i < value ? color : rgb(54, 48, 46));
    }
    char value_text[4];
    snprintf(value_text, sizeof(value_text), "%02d", value);
    draw_scaled_text(item_x + item_w - 32 * scale, y, value_text, color, 2 * scale);
}

int menu_item_count_for_page(int page)
{
    if (page == MENU_PAGE_SETTINGS) {
        return SETTINGS_MENU_ITEM_COUNT;
    }
    if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        return SLOT_MENU_ITEM_COUNT;
    }
#ifdef __EMSCRIPTEN__
    return MAIN_MENU_ITEM_EXIT;
#else
    return MAIN_MENU_ITEM_COUNT;
#endif
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
        case SETTINGS_MENU_ITEM_POST:
            return "BLOOM I LUT KOLORU";
        case SETTINGS_MENU_ITEM_SFX_VOLUME:
            return "GLOSNOSC EFEKTOW";
        case SETTINGS_MENU_ITEM_MUSIC_VOLUME:
            return "GLOSNOSC MUZYKI";
        case SETTINGS_MENU_ITEM_FULLSCREEN:
            return "PELNY EKRAN";
        case SETTINGS_MENU_ITEM_RESOLUTION:
            return "RENDER 640X480 / 1280X960";
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

void render_game_menu(int page,
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
    (void)quality;
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
        render_effects_menu_text(effects),
        "EFEKTY",
        "MUZYKA",
        fullscreen ? "FULLSCREEN ON" : "FULLSCREEN OFF",
        SCREEN_W == 1280 ? "RESOLUTION 1280X960" : "RESOLUTION 640X480",
        "WROC",
    };
    const char **items = page == MENU_PAGE_SETTINGS ? settings_items : main_items;
    int item_count = menu_item_count_for_page(page);
    int scale = SCREEN_H / 480;
    int w = 328 * scale;
    int h = page == MENU_PAGE_SETTINGS ? 376 : (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD ? 408 : 304);
#ifndef __EMSCRIPTEN__
    if (page == MENU_PAGE_MAIN) {
        h = 336;
    }
#endif
    h *= scale;
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
    blend_rect(x + 4 * scale, y + 4 * scale, w - 8 * scale, h - 8 * scale, rgb(50, 34, 22), 0.26);
    fill_rect(x + 18 * scale, y + 34 * scale, w - 36 * scale, scale, rgb(112, 82, 42));
    fill_rect(x + 18 * scale, y + h - 45 * scale, w - 36 * scale, scale, rgb(78, 58, 36));

    const char *title = "DIOOM";
    if (page == MENU_PAGE_SETTINGS) {
        title = "USTAWIENIA";
    } else if (page == MENU_PAGE_SAVE) {
        title = "ZAPIS GRY";
    } else if (page == MENU_PAGE_LOAD) {
        title = "ODCZYT GRY";
    }
    render_centered_menu_text(y + 14 * scale, title, rgb(236, 58, 40));
    for (int i = 0; i < item_count; ++i) {
        uint32_t color = selected == i ? rgb(242, 228, 192) : rgb(190, 174, 150);
        if (page == MENU_PAGE_SETTINGS && i != SETTINGS_MENU_ITEM_BACK) {
            color = selected == i ? rgb(242, 228, 192) : rgb(190, 174, 150);
        } else if ((page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) && i != SLOT_MENU_ITEM_BACK) {
            color = selected == i ? rgb(242, 228, 192) : rgb(190, 174, 150);
        } else if (page == MENU_PAGE_MAIN && (i == MAIN_MENU_ITEM_SAVE || i == MAIN_MENU_ITEM_LOAD)) {
            color = selected == i ? rgb(242, 228, 192) : rgb(190, 174, 150);
        } else if ((page == MENU_PAGE_MAIN && i == MAIN_MENU_ITEM_EXIT) ||
                   (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_BACK) ||
                   ((page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) && i == SLOT_MENU_ITEM_BACK)) {
            color = selected == i ? rgb(238, 142, 112) : rgb(184, 116, 94);
        }
        int item_y = y + 58 * scale + i * 32 * scale;
        if (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_SFX_VOLUME) {
            render_menu_slider(item_y, items[i], sfx_volume_value, selected == i, color);
        } else if (page == MENU_PAGE_SETTINGS && i == SETTINGS_MENU_ITEM_MUSIC_VOLUME) {
            render_menu_slider(item_y, items[i], music_volume_value, selected == i, color);
        } else {
            render_menu_item(item_y, items[i], selected == i, color);
        }
    }

    render_centered_menu_text(y + h - 62 * scale, menu_item_description(page, selected), rgb(174, 156, 112));
    render_centered_menu_text(y + h - 34 * scale, "W/S WYBOR ENTER AKCJA", rgb(148, 132, 100));
    const char *footer = game_started ? "ESC WROC" : "ENTER START";
#ifndef __EMSCRIPTEN__
    if (!game_started) {
        footer = "ESC WYJSCIE";
    }
#endif
    if (page == MENU_PAGE_SETTINGS) {
        footer = "A/D ZMIANA ESC WROC";
    } else if (page == MENU_PAGE_SAVE || page == MENU_PAGE_LOAD) {
        footer = "ENTER SLOT ESC WROC";
    }
    render_centered_menu_text(y + h - 18 * scale, footer, rgb(116, 106, 86));
}

void render_relic_notice(const GameState *game)
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
