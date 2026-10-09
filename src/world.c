#include "dioom.h"

static double raycast_wall_distance(Vec2 origin, Vec2 dir);
static double raycast_house_distance(Vec2 origin, Vec2 dir);
static double monster_front_notice_distance(int type);
static double monster_close_notice_distance(int type);
static double monster_fov_dot(int type);
static uint32_t rng_next(LevelRng *rng);
static int rng_range(LevelRng *rng, int min, int max);
static int level_wall_tex(int x, int y, uint32_t seed);
static void level_fill_walls(uint32_t seed);
static void level_fill_forest(uint32_t seed);
static void carve_tile(int x, int y);
static void carve_room(LevelRoom room);
static void carve_h_corridor(int x0, int x1, int y);
static void carve_v_corridor(int x, int y0, int y1);
static Vec2 room_center(LevelRoom room);
static int house_blocks_area(const House *house, double x, double y, double radius);
static int house_overlaps_tile(const House *house, int x, int y, double margin);
static int reserved_dynamic_tile(const GameState *game, int x, int y);
static int dungeon_route_tile(const GameState *game, int x, int y);
static Vec2 pick_floor_spot(LevelRng *rng, const GameState *game, double min_dist2);
static Vec2 pick_floor_spot_in_rect(LevelRng *rng, const GameState *game, int rx, int ry, int rw, int rh);
static void place_generated_doors(GameState *game, LevelRng *rng);
static void place_generated_secrets(GameState *game, LevelRng *rng);
static void place_generated_torches(const GameState *game);
static void place_forest_dungeon_portals(GameState *game, LevelRng *rng);
static int house_site_clear(const GameState *game, const House *candidate);
static void place_forest_houses(GameState *game, LevelRng *rng);
static void place_forest_trees(GameState *game, LevelRng *rng);
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
                             uint32_t loot_mask);
static void place_house_exit_portal(GameState *game);
static void place_house_torches(int variant);
static void generate_house_level(GameState *game, uint32_t seed);
static Vec2 pick_relic_guard_spot(const GameState *game, Vec2 relic_pos);
static void place_relic_guardian(GameState *game, Vec2 relic_pos);
static void place_generated_items(GameState *game, LevelRng *rng);
static void place_generated_decals(GameState *game, LevelRng *rng);
static void place_generated_wall_decals(GameState *game, LevelRng *rng);
static void place_generated_monsters(GameState *game, LevelRng *rng, int boss_room, LevelRoom room);
static void carve_maze(LevelRng *rng);
static void carve_tight_level(GameState *game, LevelRng *rng);
static void carve_forest_level(LevelRng *rng, uint32_t seed);
static void generate_rooms_level(LevelRng *rng, uint32_t seed);
static void generate_level(GameState *game, uint32_t seed, int mode);

SavedLevel saved_forest;

int runtime_trainer = 0;

int runtime_difficulty = DEFAULT_DIFFICULTY;

int runtime_level_mode = GENERATOR_FOREST;

uint32_t runtime_level_seed = LEVEL_TEST_SEED;

Torch torches[MAX_TORCHES];

int level_map[MAP_H][MAP_W];

GameState *active_game = NULL;

const Door *door_at_tile(const GameState *game, int x, int y)
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

int map_at(int x, int y)
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

int can_step_between(Vec2 from, Vec2 to, double radius)
{
    double base = floor_height_at(from);
    for (int iy = -1; iy <= 1; iy += 2) {
        for (int ix = -1; ix <= 1; ix += 2) {
            Vec2 edge = {to.x + ix * radius, to.y + iy * radius};
            int x = (int)floor(edge.x), y = (int)floor(edge.y);
            if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return 0;
            if (sector_floor[y][x] - base > 0.126 ||
                sector_ceiling[y][x] - fmax(base, sector_floor[y][x]) < 0.75) return 0;
        }
    }
    return 1;
}

double vec_len(Vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

Vec2 vec_norm(Vec2 v)
{
    double len = vec_len(v);
    if (len <= 0.0001) {
        return (Vec2){0.0, 0.0};
    }
    return (Vec2){v.x / len, v.y / len};
}

double vec_dot(Vec2 a, Vec2 b)
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

int has_line_of_sight(Vec2 from, Vec2 to)
{
    Vec2 diff = {to.x - from.x, to.y - from.y};
    double dist = vec_len(diff);
    Vec2 dir = vec_norm(diff);
    double wall_dist = raycast_wall_distance(from, dir);
    double house_dist = raycast_house_distance(from, dir);
    double block_dist = wall_dist < house_dist ? wall_dist : house_dist;
    return block_dist + 0.12 >= dist;
}

int monster_max_hp(int type)
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

double monster_patrol_speed(int type)
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

double monster_chase_speed(int type)
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

double monster_attack_speed(int type)
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

double monster_retreat_speed(int type)
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

double monster_preferred_distance(int type)
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

int monster_uses_projectile(int type)
{
    return type == 1 || type == MONSTER_FLYING_HEAD;
}

double monster_melee_range(int type)
{
    switch (type) {
    case 1: return 0.78;
    case 2: return 0.90;
    case 3: return 0.82;
    case MONSTER_GIANT_SKELETON: return 1.08;
    default: return 0.80;
    }
}

int monster_melee_damage(int type)
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

int monster_is_behind_player(const Monster *monster, const Camera *cam)
{
    Vec2 player_to_monster = {
        monster->pos.x - cam->pos.x,
        monster->pos.y - cam->pos.y,
    };
    Vec2 dir = vec_norm(player_to_monster);
    return vec_dot(cam->dir, dir) < -0.25;
}

int monster_can_directly_see_player(const Monster *monster, const Camera *cam, double *out_dist)
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

int monster_has_nearby_witness(const GameState *game, int monster_index, const Camera *cam)
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

double monster_shot_cooldown(int type, int index)
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

int monster_projectile_damage(int type)
{
    switch (type) {
    case 1: return 9;
    case MONSTER_FLYING_HEAD: return 12;
    default: return 0;
    }
}

int count_relics(int mask)
{
    int count = 0;
    for (int i = 0; i < RELIC_COUNT; ++i) {
        if (mask & (1 << i)) {
            count++;
        }
    }
    return count;
}

void sync_relic_progress(GameState *game)
{
    game->relic_mask &= RELIC_MASK_ALL;
    game->relic_count = count_relics(game->relic_mask);
    game->boss_unlocked = game->relic_mask == RELIC_MASK_ALL;
}

void reveal_fog(GameState *game, const Camera *cam)
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

double start_dist2(int x, int y)
{
    double dx = x + 0.5 - 2.5;
    double dy = y + 0.5 - 22.5;
    return dx * dx + dy * dy;
}

int generated_floor(int x, int y)
{
    return x > 0 && x < MAP_W - 1 && y > 0 && y < MAP_H - 1 && level_map[y][x] == 0;
}

double house_min_x(const House *house)
{
    return house->pos.x - house->half_w;
}

double house_max_x(const House *house)
{
    return house->pos.x + house->half_w;
}

double house_min_y(const House *house)
{
    return house->pos.y - house->half_d;
}

double house_max_y(const House *house)
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

int houses_block_area(const GameState *game, double x, double y, double radius)
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

int props_block_area(const GameState *game, double x, double y, double radius)
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

int occupied_spawn_tile(const GameState *game, int x, int y)
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

void mark_dungeon_reachable_tiles(const GameState *game, unsigned char reachable[MAP_H][MAP_W])
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

int dungeon_tile_reachable_from_entrance(const GameState *game, int x, int y)
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

void place_dungeon_exit_portal(GameState *game)
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
        .hp = adaptive_monster_hp(game, monster_max_hp(MONSTER_GIANT_SKELETON)),
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

void place_dungeon_relic(GameState *game)
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

void apply_forest_relic_escalation(GameState *game)
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
        int min_hp = adaptive_monster_hp(game, monster_max_hp(monster->type)) + escalation * 2;
        if (monster->hp < min_hp) {
            monster->hp = min_hp;
        }
        monster->shoot_timer *= 0.92;
    }

    LevelRng rng = {LEVEL_TEST_SEED ^ (uint32_t)(game->relic_count * 977u + game->kills * 31u)};
    int to_spawn = escalation + (escalation >= 3 ? 1 : 0) + adaptive_extra_spawns();
    if (to_spawn < 1) to_spawn = 1;
    for (int i = 0; i < game->monster_count && to_spawn > 0; ++i) {
        Monster *monster = &game->monsters[i];
        if (monster->active) {
            continue;
        }
        memset(monster, 0, sizeof(*monster));
        monster->active = 1;
        monster->pos = pick_floor_spot(&rng, game, 42.0);
        monster->type = escalation >= 3 && (to_spawn & 1) ? MONSTER_FLYING_HEAD : 3;
        monster->hp = adaptive_monster_hp(game, monster_max_hp(monster->type)) + escalation * 2;
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
        monster->hp = adaptive_monster_hp(game, monster_max_hp(monster->type));
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
            monster->hp = adaptive_monster_hp(game, BOSS_HP);
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
        build_sector_heights(game);
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
    build_sector_heights(game);
}

void init_game_seed(GameState *game, uint32_t seed, int mode)
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
    adaptive_begin_level();
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

void init_game(GameState *game)
{
    init_game_seed(game, LEVEL_TEST_SEED, GENERATOR_ROOMS);
}

int can_occupy(double x, double y, double radius)
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

int can_move(double x, double y)
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
