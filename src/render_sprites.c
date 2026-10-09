#include "dioom.h"

static double normalize_angle(double angle);
static void grounded_sprite_bounds(const Camera *cam, Vec2 pos, double depth, int sprite_h, double scale, int *raw_start_y, int *raw_end_y);
static uint32_t projectile_texel(const Projectile *projectile, int tex_x, int tex_y);
static void render_projectile(const Camera *cam, const Projectile *projectile);
static int item_sprite_for_type(const Item *item);
static uint32_t item_texel(const Item *item, int tex_x, int tex_y);
static void render_item(const Camera *cam, const Item *item);
static uint32_t portal_texel(const Portal *portal, int tex_x, int tex_y);
static void render_portal(const Camera *cam, const Portal *portal);
static uint32_t torch_texel(int tex_x, int tex_y, double time, int index);
static uint32_t campfire_texel(int tex_x, int tex_y, double time, int index);
static void render_torch(const Camera *cam, const Torch *torch, int index, double time);
static void render_screen_ellipse(int cx, int cy, int rx, int ry, double depth, uint32_t color, double strength);

static double normalize_angle(double angle)
{
    while (angle < -M_PI) angle += M_PI * 2.0;
    while (angle >= M_PI) angle -= M_PI * 2.0;
    return angle;
}

int monster_frame_for_camera(const Camera *cam, const Monster *monster)
{
    double to_camera_x = cam->pos.x - monster->pos.x;
    double to_camera_y = cam->pos.y - monster->pos.y;
    double monster_front_angle = atan2(monster->facing.y, monster->facing.x);
    double relative = normalize_angle(atan2(to_camera_y, to_camera_x) - monster_front_angle);
    int frame = (int)floor((relative + M_PI / 4.0) / (M_PI / 2.0));
    static const int frame_map[SPRITE_FRAMES] = {0, 3, 2, 1};
    return frame_map[frame & 3];
}

int monster_anim_frame_for_monster(const Monster *monster)
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

int project_sprite(const Camera *cam, Vec2 pos, double scale, int *screen_x, int *screen_h, double *depth)
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

static void grounded_sprite_bounds(const Camera *cam, Vec2 pos, double depth, int sprite_h, double scale, int *raw_start_y, int *raw_end_y)
{
    int base_h = (int)(sprite_h / scale);
    if (base_h < 1) {
        base_h = 1;
    }
    *raw_end_y = SCREEN_H / 2 + base_h / 2 + sprite_height_offset(cam, pos, depth);
    *raw_start_y = *raw_end_y - sprite_h;
}

void render_monster(const Camera *cam, const Monster *monster)
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
    int raw_start_y = -sprite_h / 2 + SCREEN_H / 2 - hover + sprite_height_offset(cam, monster->pos, depth);
    int raw_end_y = sprite_h / 2 + SCREEN_H / 2 - hover + sprite_height_offset(cam, monster->pos, depth);
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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
    grounded_sprite_bounds(cam, projectile->pos, depth, sprite_h, scale, &raw_start_y, &raw_end_y);
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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
    if (item->type == ITEM_SEAL) {
        if (item->relic_index < 0 || item->relic_index >= 3) return 0;
        uint32_t pixel = seal_sprites[item->relic_index][tex_y * PROJECTILE_SIZE + tex_x];
        return is_sprite_key(pixel) ? 0 : pixel;
    }

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
        ((item->type == ITEM_SHRINE || item->type == ITEM_SEAL) ? 0.62 :
         (item->type == ITEM_BONEPILE ? 0.48 :
          (item->type == ITEM_GOLD ? 0.34 : 0.42)));
    if (!project_sprite(cam, item->pos, scale, &sprite_screen_x, &sprite_h, &depth)) {
        return;
    }

    int sprite_w = sprite_h;
    int raw_start_y;
    int raw_end_y;
    grounded_sprite_bounds(cam, item->pos, depth, sprite_h, scale, &raw_start_y, &raw_end_y);
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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

uint32_t prop_texel(const Prop *prop, double u, double v)
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
    grounded_sprite_bounds(cam, pos, depth, sprite_h, scale, &raw_start_y, &raw_end_y);
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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

void render_tree(const Camera *cam, const GameState *game, const Tree *tree)
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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
    grounded_sprite_bounds(cam, torch->pos, depth, sprite_h, scale, &raw_start_y, &raw_end_y);
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
            if (depth >= depth_buffer[y * SCREEN_W + stripe]) continue;
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

void render_world_sprites(const Camera *cam, const GameState *game)
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
            if (end_y >= SCREEN_H - HUD_HEIGHT) end_y = SCREEN_H - HUD_HEIGHT - 1;

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
    if (end_y >= SCREEN_H - HUD_HEIGHT) end_y = SCREEN_H - HUD_HEIGHT - 1;

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

void render_dynamic_shadows(const Camera *cam, const GameState *game)
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
            render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2 - 3 + sprite_height_offset(cam, monster->pos, depth), sh * scale / 3, sh * scale / 14 + 1, depth, rgb(0, 0, 0), monster->is_boss ? 0.52 : 0.34);
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
            render_screen_ellipse(sx, SCREEN_H / 2 + sh / 2 - 1 + sprite_height_offset(cam, item->pos, depth), sh / 4, sh / 14 + 1, depth, rgb(0, 0, 0), 0.22);
        }
    }
}

void render_decals(const Camera *cam, const GameState *game)
{
    const double horizon = SCREEN_H * 0.5;
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

        double pos_z = SCREEN_H * (camera_eye_height(cam) - floor_height_at(decal->pos));
        int center_y = (int)(horizon + pos_z / depth);
        int start_x = sx - screen_radius;
        int end_x = sx + screen_radius;
        int start_y = center_y - screen_radius;
        int end_y = center_y + screen_radius;
        if (start_x < 0) start_x = 0;
        if (end_x >= SCREEN_W) end_x = SCREEN_W - 1;
        if (start_y <= (int)horizon) start_y = (int)horizon + 1;
        if (end_y >= SCREEN_H - HUD_HEIGHT) end_y = SCREEN_H - HUD_HEIGHT - 1;
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
                if (row_distance >= depth_buffer[y * SCREEN_W + x] + 0.08) {
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
