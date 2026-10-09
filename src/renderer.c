#include "dioom.h"

double sector_ceiling[MAP_H][MAP_W];

static uint32_t fog_color(void);
static void build_fog_luts(void);
static double fog_amount_from_lut(double distance, const double *lut);
static uint32_t apply_fast_material(uint32_t albedo, double diffuse_light, double direct_light);
static int is_door_wall(int wall);
static int wall_texture_index(int wall);
static int sector_layout_wall(const GameState *game, int x, int y);
static double ray_wall_u(const Camera *cam, double ray_dir_x, double ray_dir_y, double perp_wall_dist, int side);
static int ray_hits_opening_door(const Door *door, double wall_u);
static double torch_light_at(double x, double y, double time);
static double player_torch_light_at(const Camera *cam, const GameState *game, double x, double y);
static double trace_forest_moon_visibility(double x, double y);
static void build_moon_visibility_cache(void);
static void render_forest_sky(const Camera *cam, const GameState *game);
static void render_floor_ceiling(const Camera *cam, const GameState *game);
static void render_forest_moon(const Camera *cam, const GameState *game);
static uint32_t apply_wall_decals(const GameState *game, int map_x, int map_y, int side, double wall_u, double wall_v, uint32_t lit);
static int intersect_house_ray(const House *house, const Camera *cam, double ray_dir_x, double ray_dir_y, HouseHit *hit);
static uint32_t house_texel_for_face(int face, double u, double v);
static int project_world_point(const Camera *cam, double world_x, double world_y, double z, ProjectedPoint *out);
static double prop_min_x(const Prop *prop);
static double prop_max_x(const Prop *prop);
static double prop_min_y(const Prop *prop);
static double prop_max_y(const Prop *prop);
static double prop_face_light(int face);
static void render_prop_triangle(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, const Prop *prop, double light);
static double prop_surface_light(const Camera *cam, const GameState *game, double world_x, double world_y, double face_light);
static void render_prop_quad(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, TexturedPoint d, const Prop *prop, double light);
static void render_prop_vertical_quad(const Camera *cam,
                                      const GameState *game,
                                      const Prop *prop,
                                      double ax,
                                      double ay,
                                      double bx,
                                      double by,
                                      double u0,
                                      double u1,
                                      double light);
static void render_prop_box_sides(const Camera *cam, const GameState *game, const Prop *prop);
static void render_prop_top(const Camera *cam, const GameState *game, const Prop *prop);
static void render_prop_cylinder_sides(const Camera *cam, const GameState *game, const Prop *prop);
static void render_prop_cylinder_top(const Camera *cam, const GameState *game, const Prop *prop);
static void render_props_3d(const Camera *cam, const GameState *game);
static void render_roof_triangle(const GameState *game, ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, uint32_t color);
static void render_gable_triangle(const GameState *game, TexturedPoint a, TexturedPoint b, TexturedPoint c, int tex, double light);
static void render_house_roof_quad(const GameState *game, ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, ProjectedPoint d, uint32_t color);
static void render_house_roofs_and_gables(const Camera *cam, const GameState *game);
static void render_houses(const Camera *cam, const GameState *game);
static double elapsed_ms(uint64_t start, uint64_t end);
static uint32_t sector_lit_texel(const Camera *cam, const GameState *game,
                               int tex, int u, int v, double wx, double wy,
                               double depth, double face_light);
static void render_sector_plane(const Camera *cam, const GameState *game, int x,
                                Vec2 ray, double height, double near_t, double far_t,
                                int texture, double light);
static void render_sector_face(const Camera *cam, const GameState *game, int x,
                               Vec2 ray, double t, double low, double high,
                               int side, int texture, int tile_x, int tile_y, int decals);

static double world_light_grid[MAP_H * 2 + 1][MAP_W * 2 + 1];

int moon_visibility_cache_ready = 0;

static double moon_visibility_cache[MAP_H][MAP_W];

int torch_flicker_cache_ready = 0;

static double torch_flicker_cache_time = 0.0;

static double torch_flicker_cache[MAX_TORCHES];

static double sector_light[MAP_H + 1][MAP_W + 1];

double sector_floor[MAP_H][MAP_W];

int render_effects = DEFAULT_RENDER_EFFECTS;

int render_quality = DEFAULT_RENDER_QUALITY;

RenderProfile *active_profile = NULL;

static int fog_lut_ready = 0;

static double forest_fog_lut[FOG_LUT_SIZE + 1];

static double fog_lut[FOG_LUT_SIZE + 1];

float light_buffer[MAX_SCREEN_W * MAX_SCREEN_H];

float glow_buffer[MAX_SCREEN_W * MAX_SCREEN_H];

float depth_buffer[MAX_SCREEN_W * MAX_SCREEN_H];

double z_buffer[MAX_SCREEN_W];

uint32_t framebuffer[MAX_SCREEN_W * MAX_SCREEN_H];

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

uint8_t clamp_u8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

uint32_t shade(uint32_t color, double amount)
{
    int r = (int)(((color >> 16) & 0xFFu) * amount);
    int g = (int)(((color >> 8) & 0xFFu) * amount);
    int b = (int)((color & 0xFFu) * amount);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

uint32_t mix_color(uint32_t a, uint32_t b, double t)
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

double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

double smooth01(double v)
{
    v = clamp01(v);
    return v * v * (3.0 - 2.0 * v);
}

uint32_t lerp_color_q8(uint32_t a, uint32_t b, uint32_t t)
{
    uint32_t keep = 256u - t;
    uint32_t rb = ((a & 0xFF00FFu) * keep + (b & 0xFF00FFu) * t) >> 8;
    uint32_t g = ((a & 0x00FF00u) * keep + (b & 0x00FF00u) * t) >> 8;
    return 0xFF000000u | (rb & 0xFF00FFu) | (g & 0x00FF00u);
}

uint32_t mix_amount_q8(double t)
{
    return (uint32_t)(clamp01(t) * 256.0 + 0.5);
}

void surface_shade_init(SurfaceShade *shade_out, double light)
{
    shade_out->light = light <= 0.0 ? 0u : (uint32_t)(light * 256.0 + 0.5);
    shade_out->keep = 1.0;
    shade_out->add[0] = shade_out->add[1] = shade_out->add[2] = 0.0;
}

void surface_shade_mix(SurfaceShade *shade_out, uint32_t color, double amount)
{
    amount = clamp01(amount);
    shade_out->keep *= 1.0 - amount;
    shade_out->add[0] = shade_out->add[0] * (1.0 - amount) + ((color >> 16) & 0xFFu) * amount;
    shade_out->add[1] = shade_out->add[1] * (1.0 - amount) + ((color >> 8) & 0xFFu) * amount;
    shade_out->add[2] = shade_out->add[2] * (1.0 - amount) + (color & 0xFFu) * amount;
}

void surface_shade_finish(SurfaceShade *shade_out)
{
    shade_out->keep_q = (uint32_t)(shade_out->keep * 256.0 + 0.5);
    for (int ch = 0; ch < 3; ++ch) {
        shade_out->add_q[ch] = (uint32_t)(shade_out->add[ch] * 256.0 + 0.5);
    }
}

uint32_t surface_shade_apply(const SurfaceShade *shade_in, uint32_t texel)
{
    uint32_t r = (((texel >> 16) & 0xFFu) * shade_in->light) >> 8;
    uint32_t g = (((texel >> 8) & 0xFFu) * shade_in->light) >> 8;
    uint32_t b = ((texel & 0xFFu) * shade_in->light) >> 8;
    if (r > 255u) r = 255u;
    if (g > 255u) g = 255u;
    if (b > 255u) b = 255u;
    r = (r * shade_in->keep_q + shade_in->add_q[0]) >> 8;
    g = (g * shade_in->keep_q + shade_in->add_q[1]) >> 8;
    b = (b * shade_in->keep_q + shade_in->add_q[2]) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static uint32_t fog_color(void)
{
    return rgb(38, 46, 44);
}

uint32_t fog_color_for_game(const GameState *game)
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

double fog_amount(double distance)
{
    return fog_amount_from_lut(distance, fog_lut);
}

double fog_amount_for_game(const GameState *game, double distance)
{
    if (!game || game->generator_mode != GENERATOR_FOREST) {
        return fog_amount(distance);
    }
    return clamp01(fog_amount_from_lut(distance, forest_fog_lut) + game->relic_count * 0.035);
}

uint32_t apply_fog(uint32_t color, double distance, double strength)
{
    return mix_color(color, fog_color(), fog_amount(distance) * clamp01(strength));
}

uint32_t apply_game_fog(const GameState *game, uint32_t color, double distance, double strength)
{
    return mix_color(color, fog_color_for_game(game), fog_amount_for_game(game, distance) * clamp01(strength));
}

double luminance(uint32_t color)
{
    double r = (double)((color >> 16) & 0xFFu);
    double g = (double)((color >> 8) & 0xFFu);
    double b = (double)(color & 0xFFu);
    return (r * 0.2126 + g * 0.7152 + b * 0.0722) / 255.0;
}

uint8_t luma_u8(uint32_t color)
{
    int r = (int)((color >> 16) & 0xFFu);
    int g = (int)((color >> 8) & 0xFFu);
    int b = (int)(color & 0xFFu);
    return (uint8_t)((r * 54 + g * 183 + b * 19 + 128) >> 8);
}

static uint32_t apply_fast_material(uint32_t albedo, double diffuse_light, double direct_light)
{
    return shade(albedo, diffuse_light + direct_light * 0.18);
}

uint32_t add_color(uint32_t color, uint32_t add, double amount)
{
    int r = (int)((color >> 16) & 0xFFu) + (int)(((add >> 16) & 0xFFu) * amount);
    int g = (int)((color >> 8) & 0xFFu) + (int)(((add >> 8) & 0xFFu) * amount);
    int b = (int)(color & 0xFFu) + (int)((add & 0xFFu) * amount);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

uint32_t contrast_color(uint32_t color, double contrast, double brightness)
{
    int cr = (int)((color >> 16) & 0xFFu);
    int cg = (int)((color >> 8) & 0xFFu);
    int cb = (int)(color & 0xFFu);
    int r = (int)((cr - 128) * contrast + 128 + brightness);
    int g = (int)((cg - 128) * contrast + 128 + brightness);
    int b = (int)((cb - 128) * contrast + 128 + brightness);
    return rgb(clamp_u8(r), clamp_u8(g), clamp_u8(b));
}

void put_pixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        framebuffer[y * SCREEN_W + x] = color;
    }
}

void add_glow(int x, int y, double amount)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        int idx = y * SCREEN_W + x;
        glow_buffer[idx] += amount;
        if (glow_buffer[idx] > 1.0) {
            glow_buffer[idx] = 1.0;
        }
    }
}

void add_light(int x, int y, double amount)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        int idx = y * SCREEN_W + x;
        light_buffer[idx] += amount;
        if (light_buffer[idx] > 1.0) {
            light_buffer[idx] = 1.0;
        }
    }
}

void reset_render_buffers(void)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        depth_buffer[i] = 1e30f;
        glow_buffer[i] = 0.0f;
        light_buffer[i] = 0.0f;
    }
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
    return tex_idx % 5;
}

double floor_height_at(Vec2 pos)
{
    int x = (int)floor(pos.x), y = (int)floor(pos.y);
    return x >= 0 && y >= 0 && x < MAP_W && y < MAP_H ? sector_floor[y][x] : 0.0;
}

double camera_eye_height(const Camera *cam)
{
    return floor_height_at(cam->pos) + 0.5;
}

static int sector_layout_wall(const GameState *game, int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H || level_map[y][x]) return 1;
    if (door_at_tile(game, x, y)) return 1;
    for (int i = 0; i < MAX_SECRETS; ++i) {
        if (game->secrets[i].x == x && game->secrets[i].y == y) return 1;
    }
    return 0;
}

void build_sector_heights(const GameState *game)
{
    int dungeon = game->generator_mode != GENERATOR_FOREST && game->generator_mode != GENERATOR_HOUSE;
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            int clearance = 4;
            if (dungeon && !sector_layout_wall(game, x, y)) {
                for (int dy = -4; dy <= 4; ++dy) {
                    for (int dx = -4; dx <= 4; ++dx) {
                        int d = abs(dx) + abs(dy);
                        if (d < clearance && sector_layout_wall(game, x + dx, y + dy)) clearance = d;
                    }
                }
            } else {
                clearance = 1;
            }
            int steps = clearance > 1 ? clearance - 1 : 0;
            sector_floor[y][x] = steps * 0.125;
            sector_ceiling[y][x] = dungeon ? 1.25 + steps * 0.25 : 1.0;
        }
    }
}

int sprite_height_offset(const Camera *cam, Vec2 pos, double depth)
{
    return (int)(SCREEN_H * (floor_height_at(cam->pos) - floor_height_at(pos)) / depth);
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

void prepare_torch_flicker_cache(double time)
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
    double dread = story_dread > 0.0 ? 0.72 + 0.12 * sin(game->time * 19.0) : 1.0;
    return clamp01(dread * flicker * (0.48 + forward * 0.20) * fade / (1.0 + dist2 * 0.34));
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

double forest_moon_visibility_at(double x, double y)
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

uint32_t star_hash(int x, int y)
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

void prepare_world_light_grid(double time)
{
    for (int y = 0; y <= MAP_H * 2; ++y)
        for (int x = 0; x <= MAP_W * 2; ++x)
            world_light_grid[y][x] = torch_light_at(x * 0.5, y * 0.5, time);
}

double cached_world_light(double x, double y)
{
    /* The map is enclosed; outside samples have no torch contribution. */
    if (x < 0.0 || y < 0.0 || x >= MAP_W || y >= MAP_H) return 0.0;
    double gx = x * 2.0, gy = y * 2.0;
    int ix = (int)gx, iy = (int)gy;
    double fx = gx - ix, fy = gy - iy;
    return (world_light_grid[iy][ix] * (1.0 - fx) + world_light_grid[iy][ix + 1] * fx) * (1.0 - fy) +
           (world_light_grid[iy + 1][ix] * (1.0 - fx) + world_light_grid[iy + 1][ix + 1] * fx) * fy;
}

static void render_floor_ceiling(const Camera *cam, const GameState *game)
{
    int forest_mode = game->generator_mode == GENERATOR_FOREST;
    const int floor_tex = game->generator_mode == GENERATOR_FOREST ? 5 : 6;
    const int ceil_tex = game->generator_mode == GENERATOR_FOREST ? 4 : 3;
    double ray_dir_x0 = cam->dir.x - cam->plane.x;
    double ray_dir_y0 = cam->dir.y - cam->plane.y;
    double ray_dir_x1 = cam->dir.x + cam->plane.x;
    double ray_dir_y1 = cam->dir.y + cam->plane.y;
    uint32_t fog = fog_color_for_game(game);
    uint32_t torch_tint = rgb(255, 132, 48);
    uint32_t moon_tint = rgb(118, 150, 136);

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
        int floor_visible = y <= SCREEN_H - HUD_HEIGHT;
        if (!floor_visible && forest_mode) {
            continue;
        }
        double floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / SCREEN_W;
        double floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / SCREEN_W;
        double floor_x = cam->pos.x + row_distance * ray_dir_x0;
        double floor_y = cam->pos.y + row_distance * ray_dir_y0;
        double light = forest_mode
            ? 0.125 + 0.42 / (1.0 + row_distance * 0.14)
            : 0.045 + 0.30 / (1.0 + row_distance * 0.17);
        double floor_base = light * (forest_mode ? 0.86 : 0.48);
        double ceil_base = light * (forest_mode ? 0.44 : 0.18);
        double floor_fog = fog_amount_for_game(game, row_distance) * clamp01(forest_mode ? 1.24 : 0.86);
        double ceil_fog = fog_amount_for_game(game, row_distance) * 0.76;
        int light_stride = (SCREEN_W >= 640 || SCREEN_H >= 400) ? 4 : 1;
        int floor_row = y * SCREEN_W;
        int ceil_row = (SCREEN_H - y - 1) * SCREEN_W;

        for (int x = 0; x < SCREEN_W; x += light_stride) {
            int block_w = light_stride;
            if (x + block_w > SCREEN_W) {
                block_w = SCREEN_W - x;
            }
            double sample_offset = (block_w - 1) * 0.5;
            double sample_floor_x = floor_x + floor_step_x * sample_offset;
            double sample_floor_y = floor_y + floor_step_y * sample_offset;
            double player_light = player_torch_light_at(cam, game, sample_floor_x, sample_floor_y);
            double torch_light = clamp01(cached_world_light(sample_floor_x, sample_floor_y) + player_light);
            double moon_light = forest_mode ? forest_moon_visibility_at(sample_floor_x, sample_floor_y) : 0.0;

            /* Lighting is constant across the block, so shade, tints and fog fold into one surface shade. */
            SurfaceShade floor_shade;
            surface_shade_init(&floor_shade, floor_base + torch_light * 0.70 + moon_light * 0.08 +
                                             (torch_light * 0.76 + moon_light * 0.16) * 0.18);
            surface_shade_mix(&floor_shade, torch_tint, torch_light * 0.10);
            if (forest_mode) {
                surface_shade_mix(&floor_shade, moon_tint, moon_light * 0.18);
            }
            surface_shade_mix(&floor_shade, fog, floor_fog);
            surface_shade_finish(&floor_shade);
            SurfaceShade ceil_shade;
            if (!forest_mode) {
                surface_shade_init(&ceil_shade, ceil_base + torch_light * 0.26 + moon_light * 0.05 +
                                                (torch_light * 0.26 + moon_light * 0.08) * 0.18);
                surface_shade_mix(&ceil_shade, torch_tint, torch_light * 0.04);
                surface_shade_mix(&ceil_shade, fog, ceil_fog);
                surface_shade_finish(&ceil_shade);
            }
            /* The floor is the first pass to add light here, so the clamped sums can be stored directly. */
            double floor_light = fmin(torch_light * 0.18 + player_light * 0.10, 1.0);
            if (forest_mode) {
                floor_light = fmin(floor_light + moon_light * 0.05, 1.0);
            }
            double ceil_light = fmin(torch_light * 0.08 + player_light * 0.04, 1.0);

            for (int bx = 0; bx < block_w; ++bx) {
                int sx = x + bx;
                int cell_x = (int)floor(floor_x);
                int cell_y = (int)floor(floor_y);
                int tx = (int)(TEX_SIZE * (floor_x - cell_x)) & (TEX_SIZE - 1);
                int ty = (int)(TEX_SIZE * (floor_y - cell_y)) & (TEX_SIZE - 1);
                int texel_idx = ty * TEX_SIZE + tx;

                if (floor_visible) {
                    framebuffer[floor_row + sx] = surface_shade_apply(&floor_shade, textures[floor_tex][texel_idx]);
                    depth_buffer[floor_row + sx] = row_distance;
                    light_buffer[floor_row + sx] = floor_light;
                }
                if (!forest_mode) {
                    framebuffer[ceil_row + sx] = surface_shade_apply(&ceil_shade, textures[ceil_tex][texel_idx]);
                    depth_buffer[ceil_row + sx] = row_distance;
                    light_buffer[ceil_row + sx] = ceil_light;
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
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H - HUD_HEIGHT) {
                double a = 0.055 * (1.0 - (y - cy) / (double)(SCREEN_H / 2 + 24));
                framebuffer[y * SCREEN_W + x] = mix_color(framebuffer[y * SCREEN_W + x], rgb(92, 126, 154), clamp01(a));
                add_glow(x, y, a);
            }
        }
    }
}

int is_sprite_key(uint32_t color)
{
    int r = (int)((color >> 16) & 0xFFu);
    int g = (int)((color >> 8) & 0xFFu);
    int b = (int)(color & 0xFFu);
    return r > 210 && g < 70 && b > 210;
}

void clear_wall_decal_index(GameState *game)
{
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            game->wall_decal_head[y][x][0] = -1;
            game->wall_decal_head[y][x][1] = -1;
        }
    }
}

void link_wall_decal(GameState *game, int index)
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

int prop_is_cylinder(const Prop *prop)
{
    return prop && prop->type == PROP_BARREL;
}

double prop_footprint_radius(const Prop *prop)
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
    if (max_y >= SCREEN_H - HUD_HEIGHT) max_y = SCREEN_H - HUD_HEIGHT - 1;
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
    if (max_y >= SCREEN_H - HUD_HEIGHT) max_y = SCREEN_H - HUD_HEIGHT - 1;
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
    if (max_y >= SCREEN_H - HUD_HEIGHT) max_y = SCREEN_H - HUD_HEIGHT - 1;
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
        if (raw_bottom <= 0.0 || raw_top >= SCREEN_H - HUD_HEIGHT) {
            continue;
        }

        int draw_start = (int)floor(raw_top);
        int draw_end = (int)ceil(raw_bottom);
        if (draw_start < 0) draw_start = 0;
        if (draw_end >= SCREEN_H - HUD_HEIGHT) draw_end = SCREEN_H - HUD_HEIGHT - 1;
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

static uint32_t sector_lit_texel(const Camera *cam, const GameState *game,
                               int tex, int u, int v, double wx, double wy,
                               double depth, double face_light)
{
    (void)cam;
    double gx = fmax(0.0, fmin(wx, MAP_W - 0.0001));
    double gy = fmax(0.0, fmin(wy, MAP_H - 0.0001));
    int cx = (int)gx, cy = (int)gy;
    double fx = gx - cx, fy = gy - cy;
    double torch = (sector_light[cy][cx] * (1.0 - fx) + sector_light[cy][cx + 1] * fx) * (1.0 - fy) +
                   (sector_light[cy + 1][cx] * (1.0 - fx) + sector_light[cy + 1][cx + 1] * fx) * fy;
    double light = face_light / (1.0 + depth * 0.10) + fmin(torch, 1.0) * 0.45;
    /* Stepped light levels keep the generated pixel art crisp: shade by level / 16. */
    uint32_t level = (uint32_t)(fmin(light, 1.2) * 16.0);
    uint32_t texel = textures[tex][(v & 63) * TEX_SIZE + (u & 63)];
    uint32_t r = (((texel >> 16) & 0xFFu) * level) >> 4;
    uint32_t g = (((texel >> 8) & 0xFFu) * level) >> 4;
    uint32_t b = ((texel & 0xFFu) * level) >> 4;
    uint32_t color = rgb(r > 255u ? 255u : r, g > 255u ? 255u : g, b > 255u ? 255u : b);
    return lerp_color_q8(color, fog_color_for_game(game), mix_amount_q8(fog_amount_for_game(game, depth) * 0.45));
}

static void render_sector_plane(const Camera *cam, const GameState *game, int x,
                                Vec2 ray, double height, double near_t, double far_t,
                                int texture, double light)
{
    double dz = camera_eye_height(cam) - height;
    if (fabs(dz) < 0.0001) return;
    double a = SCREEN_H * 0.5 + SCREEN_H * dz / fmax(near_t, 0.001);
    double b = SCREEN_H * 0.5 + SCREEN_H * dz / far_t;
    int top = (int)ceil(fmax(0.0, fmin(a, b)));
    int bottom = (int)floor(fmin(SCREEN_H - HUD_HEIGHT - 1.0, fmax(a, b)));
    for (int y = top; y <= bottom; ++y) {
        double t = SCREEN_H * dz / (y + 0.5 - SCREEN_H * 0.5);
        if (t < near_t || t > far_t || t <= 0.0) continue;
        int idx = y * SCREEN_W + x;
        if (t >= depth_buffer[idx]) continue;
        double wx = cam->pos.x + ray.x * t, wy = cam->pos.y + ray.y * t;
        framebuffer[idx] = sector_lit_texel(cam, game, texture, (int)floor(wx * TEX_SIZE),
                                          (int)floor(wy * TEX_SIZE), wx, wy, t, light);
        depth_buffer[idx] = t;
    }
}

static void render_sector_face(const Camera *cam, const GameState *game, int x,
                               Vec2 ray, double t, double low, double high,
                               int side, int texture, int tile_x, int tile_y, int decals)
{
    if (high - low < 0.0001) return;
    double eye = camera_eye_height(cam);
    double top_y = SCREEN_H * 0.5 - SCREEN_H * (high - eye) / t;
    double bottom_y = SCREEN_H * 0.5 - SCREEN_H * (low - eye) / t;
    int top = (int)ceil(fmax(0.0, top_y));
    int bottom = (int)floor(fmin(SCREEN_H - HUD_HEIGHT - 1.0, bottom_y));
    double wx = cam->pos.x + ray.x * t, wy = cam->pos.y + ray.y * t;
    double u = ray_wall_u(cam, ray.x, ray.y, t, side);
    int tex_x = (int)(u * TEX_SIZE) & 63;
    if (decals && (tile_x < 0 || tile_x >= MAP_W || tile_y < 0 || tile_y >= MAP_H ||
                   game->wall_decal_head[tile_y][tile_x][side] < 0)) {
        decals = 0;
    }
    int cached_v = -1;
    uint32_t cached_color = 0;
    for (int y = top; y <= bottom; ++y) {
        int idx = y * SCREEN_W + x;
        if (t >= depth_buffer[idx]) continue;
        double z = eye + (SCREEN_H * 0.5 - y - 0.5) * t / SCREEN_H;
        int v = (int)floor((high - z) * TEX_SIZE);
        uint32_t color;
        if (!decals && v == cached_v) {
            color = cached_color;
        } else {
            color = sector_lit_texel(cam, game, texture, tex_x, v, wx, wy, t, side ? 0.58 : 0.76);
            cached_v = v;
            cached_color = color;
        }
        if (decals) color = apply_wall_decals(game, tile_x, tile_y, side, u, (high - z) / (high - low), color);
        framebuffer[idx] = color;
        depth_buffer[idx] = t;
    }
}

void render_sector_world(const Camera *cam, const GameState *game)
{
    for (int y = 0; y <= MAP_H; ++y) {
        for (int x = 0; x <= MAP_W; ++x) {
            sector_light[y][x] = torch_light_at(x, y, game->time) + player_torch_light_at(cam, game, x, y);
        }
    }
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) framebuffer[i] = rgb(10, 8, 8);
    for (int x = 0; x < SCREEN_W; ++x) {
        double camera_x = 2.0 * x / SCREEN_W - 1.0;
        Vec2 ray = {cam->dir.x + cam->plane.x * camera_x, cam->dir.y + cam->plane.y * camera_x};
        int mx = (int)cam->pos.x, my = (int)cam->pos.y;
        double dx = ray.x == 0.0 ? 1e30 : fabs(1.0 / ray.x);
        double dy = ray.y == 0.0 ? 1e30 : fabs(1.0 / ray.y);
        int sx = ray.x < 0.0 ? -1 : 1, sy = ray.y < 0.0 ? -1 : 1;
        double tx = (ray.x < 0.0 ? cam->pos.x - mx : mx + 1.0 - cam->pos.x) * dx;
        double ty = (ray.y < 0.0 ? cam->pos.y - my : my + 1.0 - cam->pos.y) * dy;
        double entry = 0.001;
        z_buffer[x] = 1e30;
        for (int n = 0; n < MAP_W + MAP_H; ++n) {
            if (mx < 0 || my < 0 || mx >= MAP_W || my >= MAP_H) break;
            int side = tx < ty ? 0 : 1;
            double exit = side == 0 ? tx : ty;
            double fl = sector_floor[my][mx], cl = sector_ceiling[my][mx];
            render_sector_plane(cam, game, x, ray, fl, entry, exit, 6, 0.62);
            render_sector_plane(cam, game, x, ray, cl, entry, exit, 7, 0.44);
            int nx = mx + (side == 0 ? sx : 0), ny = my + (side == 1 ? sy : 0);
            int wall = map_at(nx, ny);
            const Door *door = is_door_wall(wall) ? door_at_tile(game, nx, ny) : NULL;
            if (wall && door && door->opening &&
                !ray_hits_opening_door(door, ray_wall_u(cam, ray.x, ray.y, exit, side))) wall = 0;
            if (wall) {
                render_sector_face(cam, game, x, ray, exit, fl, cl, side, wall_texture_index(wall), nx, ny,
                                   !is_door_wall(wall));
                z_buffer[x] = exit;
                break;
            }
            if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H) break;
            double next_fl = sector_floor[ny][nx], next_cl = sector_ceiling[ny][nx];
            render_sector_face(cam, game, x, ray, exit, fmin(fl, next_fl), fmax(fl, next_fl), side, 0, nx, ny, 0);
            render_sector_face(cam, game, x, ray, exit, fmin(cl, next_cl), fmax(cl, next_cl), side, 1, nx, ny, 0);
            entry = exit;
            mx = nx; my = ny;
            if (side == 0) tx += dx; else ty += dy;
        }
    }
}

void render_scene(const Camera *cam, const GameState *game)
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

    RenderProfile *profile = active_profile;
    uint64_t total_start = 0;
    uint64_t pass_start = 0;
    if (render_effects != RENDER_EFFECTS_OFF && !color_grade_lut_ready) {
        build_color_grade_lut();
    }
    if (profile) {
        memset(profile, 0, sizeof(*profile));
        total_start = SDL_GetPerformanceCounter();
        pass_start = total_start;
    }

    prepare_torch_flicker_cache(game->time);
    if (game->generator_mode == GENERATOR_FOREST || game->generator_mode == GENERATOR_HOUSE) prepare_world_light_grid(game->time);
    if (game->generator_mode == GENERATOR_FOREST && !moon_visibility_cache_ready) {
        build_moon_visibility_cache();
    }
    reset_render_buffers();
    int sector_mode = game->generator_mode != GENERATOR_FOREST && game->generator_mode != GENERATOR_HOUSE;
    if (sector_mode) render_sector_world(cam, game);
    else render_floor_ceiling(cam, game);
    render_forest_moon(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->floor_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    if (!sector_mode) for (int x = 0; x < SCREEN_W; ++x) {
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
        if (draw_end > SCREEN_H - HUD_HEIGHT) draw_end = SCREEN_H - HUD_HEIGHT;

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
        uint32_t torch_tint = rgb(255, 132, 48);
        z_buffer[x] = perp_wall_dist;
        int cached_tex_y = -1;
        uint32_t cached_lit = 0;

        for (int y = draw_start; y <= draw_end; ++y) {
            int tex_y = ((int)tex_pos) & (TEX_SIZE - 1);
            tex_pos += step;
            if (tex_y == cached_tex_y) {
                framebuffer[y * SCREEN_W + x] = cached_lit;
                depth_buffer[y * SCREEN_W + x] = perp_wall_dist;
                add_light(x, y, torch_light * 0.10 + player_light * 0.05);
                if (game->generator_mode == GENERATOR_FOREST) {
                    add_light(x, y, moon_light * 0.035);
                }
                continue;
            }
            int texel_idx = tex_y * TEX_SIZE + tex_x;
            uint32_t color = textures[tex_idx][texel_idx];
            uint32_t lit;
            if (game->generator_mode == GENERATOR_FOREST) {
                double wall_v = tex_y / (double)(TEX_SIZE - 1);
                double height_shadow = 0.92 - smooth01(wall_v) * 0.10;
                lit = shade(
                    color,
                    height_shadow * (light * 1.04 + torch_light * 0.34 + moon_light * 0.08));
                lit = mix_color(lit, rgb(56, 74, 64), moon_light * 0.08);
            } else {
                double bump = 0.5;
                lit = apply_fast_material(
                    color,
                    light * (0.84 + bump * 0.22) + torch_light * (0.62 + bump * 0.30) + moon_light * 0.08,
                    torch_light * (0.72 + bump * 0.50) + moon_light * 0.13);
            }
            lit = mix_color(lit, torch_tint, clamp01(torch_light * 0.10));
            if (game->generator_mode == GENERATOR_FOREST) {
                lit = mix_color(lit, rgb(106, 136, 120), moon_light * 0.13);
            }
            if (wall != WALL_DOOR && wall != WALL_LOCKED_DOOR) {
                lit = apply_wall_decals(game, map_x, map_y, side, wall_u, (tex_y + 0.5) / TEX_SIZE, lit);
            }
            cached_lit = apply_game_fog(game, lit, perp_wall_dist, game->generator_mode == GENERATOR_FOREST ? 1.22 : 1.0);
            cached_tex_y = tex_y;
            framebuffer[y * SCREEN_W + x] = cached_lit;
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

    if (!sector_mode) render_volumetric_fog(cam, game);
    if (profile) {
        uint64_t now = SDL_GetPerformanceCounter();
        profile->fog_ms = elapsed_ms(pass_start, now);
        pass_start = now;
    }

    render_forest_weather_overlay(game);
    render_muzzle_light(game);
    if (render_effects != RENDER_EFFECTS_OFF) {
        render_light_and_bloom(render_effects);
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
    render_boss_bar(game);
    render_hud(game);
    render_minimap(cam, game);
    if (story_notice_time > 0.0 && story_notice) {
        int scale = SCREEN_H / 480;
        int w = (int)strlen(story_notice) * 12 * scale;
        int x = SCREEN_W / 2 - w / 2, y = SCREEN_H - HUD_HEIGHT - 58 * scale;
        blend_rect(x - 5 * scale, y - 4 * scale, w + 10 * scale, 22 * scale, rgb(12, 9, 8), 0.82);
        draw_scaled_text(x, y, story_notice, rgb(226, 184, 112), 2 * scale);
    }
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
