#include "dioom.h"

static double bloom_strength_for_effects(int effects);
static uint8_t lut_channel(double value);
static double color_grade_channel_value(int effects, int channel, double value, double luma);
static uint32_t average_color(uint32_t a, uint32_t b);

int vignette_ready = 0;

int color_grade_lut_ready = 0;

static uint8_t color_grade_lut[RENDER_EFFECTS_COUNT][3][256][256];

/* Vignette strength in 0..65535, rebuilt when the resolution changes. */
static uint16_t vignette_buffer[MAX_SCREEN_W * MAX_SCREEN_H];

static uint8_t post_luma_buffer[MAX_SCREEN_W * MAX_SCREEN_H];

static uint32_t post_row_buffer[2][MAX_SCREEN_W];

/* Bloom runs at half resolution: [0] holds downsampled glow, [1] the blurred result. */
static float bloom_half_buffer[2][(MAX_SCREEN_W / 2) * (MAX_SCREEN_H / 2)];

static float bloom_upsample_row[MAX_SCREEN_W / 2];

static float bloom_bright_lut[256];

static int bloom_bright_lut_ready = 0;

void render_volumetric_fog(const Camera *cam, const GameState *game)
{
    uint32_t fog = fog_color_for_game(game);
    int fog_stride = SCREEN_W / 160;
    int scatter_steps = fog_stride > 1 ? 2 : 4;

    for (int x = 0; x < SCREEN_W; x += fog_stride) {
        double wall_depth = z_buffer[x];
        double camera_x = 2.0 * x / (double)SCREEN_W - 1.0;
        double ray_dir_x = cam->dir.x + cam->plane.x * camera_x;
        double ray_dir_y = cam->dir.y + cam->plane.y * camera_x;

        for (int y = 0; y < SCREEN_H - HUD_HEIGHT; y += fog_stride) {
            int idx = y * SCREEN_W + x;
            int from_horizon = abs(y - SCREEN_H / 2);
            double pixel_depth = depth_buffer[idx];
            int forest_mode = game->generator_mode == GENERATOR_FOREST;
            int wall_pixel = forest_mode && pixel_depth > 0.01 && fabs(pixel_depth - wall_depth) < 0.0001;
            if (pixel_depth <= 0.01) {
                pixel_depth = wall_depth;
            }
            if (from_horizon > 0 && (game->generator_mode == GENERATOR_FOREST || game->generator_mode == GENERATOR_HOUSE)) {
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
                for (int by = 0; by < fog_stride && y + by < SCREEN_H - HUD_HEIGHT; ++by) {
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
                    torch_scatter += cached_world_light(p.x, p.y) * (1.0 - sample * 0.10);
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
                    for (int by = 0; by < fog_stride && y + by < SCREEN_H - HUD_HEIGHT; ++by) {
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

int normalize_render_effects(int effects)
{
    if (effects < 0 || effects >= RENDER_EFFECTS_COUNT) {
        return RENDER_EFFECTS_OFF;
    }
    return effects;
}

const char *render_effects_menu_text(int effects)
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

static void apply_light_row(int y)
{
    uint32_t warm = rgb(255, 134, 48);
    for (int x = 0; x < SCREEN_W; ++x) {
        int idx = y * SCREEN_W + x;
        double light = light_buffer[idx];
        if (light > 0.002) {
            framebuffer[idx] = lerp_color_q8(framebuffer[idx], warm, mix_amount_q8(light * 0.18));
        }
    }
}

void render_light_and_bloom(int effects)
{
    double strength = bloom_strength_for_effects(effects);
    int limit_y = SCREEN_H - HUD_HEIGHT;
    int half_w = SCREEN_W / 2;
    int half_h = limit_y / 2;
    if (!bloom_bright_lut_ready) {
        for (int luma = 0; luma < 256; ++luma) {
            bloom_bright_lut[luma] = (float)(smooth01((luma / 255.0 - 0.62) / 0.30) * 0.22);
        }
        bloom_bright_lut_ready = 1;
    }

    /* Tint lit pixels, then downsample glow plus bright pixels into 2x2 cells. */
    float *glow_half = bloom_half_buffer[0];
    for (int hy = 0; hy < half_h; ++hy) {
        int y = hy * 2;
        apply_light_row(y);
        apply_light_row(y + 1);
        if (strength <= 0.0) {
            continue;
        }
        for (int hx = 0; hx < half_w; ++hx) {
            int idx = y * SCREEN_W + hx * 2;
            float sum = (float)(glow_buffer[idx] + glow_buffer[idx + 1] +
                                glow_buffer[idx + SCREEN_W] + glow_buffer[idx + SCREEN_W + 1]);
            sum += bloom_bright_lut[luma_u8(framebuffer[idx])] + bloom_bright_lut[luma_u8(framebuffer[idx + 1])] +
                   bloom_bright_lut[luma_u8(framebuffer[idx + SCREEN_W])] +
                   bloom_bright_lut[luma_u8(framebuffer[idx + SCREEN_W + 1])];
            glow_half[hy * half_w + hx] = sum * 0.25f;
        }
    }
    for (int y = half_h * 2; y < limit_y; ++y) {
        apply_light_row(y);
    }
    if (strength <= 0.0 || half_w < 3 || half_h < 3) {
        return;
    }

    float *blur = bloom_half_buffer[1];
    memset(blur, 0, (size_t)half_w * (size_t)half_h * sizeof(blur[0]));
    for (int hy = 1; hy < half_h - 1; ++hy) {
        for (int hx = 1; hx < half_w - 1; ++hx) {
            int idx = hy * half_w + hx;
            blur[idx] =
                glow_half[idx] * 0.34f +
                (glow_half[idx - 1] + glow_half[idx + 1] + glow_half[idx - half_w] + glow_half[idx + half_w]) * 0.12f +
                (glow_half[idx - half_w - 1] + glow_half[idx - half_w + 1] +
                 glow_half[idx + half_w - 1] + glow_half[idx + half_w + 1]) * 0.045f;
        }
    }

    /* Bilinear upsample: rows first into a scratch row, then columns per pixel. */
    uint32_t bloom_color = rgb(255, 122, 40);
    for (int y = 0; y < limit_y; ++y) {
        float fy = (y - 0.5f) * 0.5f;
        int hy0 = (int)floorf(fy);
        float ty = fy - hy0;
        int hy1 = hy0 + 1;
        if (hy0 < 0) hy0 = 0;
        if (hy1 >= half_h) hy1 = half_h - 1;
        const float *row0 = blur + hy0 * half_w;
        const float *row1 = blur + hy1 * half_w;
        float row_max = 0.0f;
        for (int hx = 0; hx < half_w; ++hx) {
            bloom_upsample_row[hx] = row0[hx] + (row1[hx] - row0[hx]) * ty;
            if (bloom_upsample_row[hx] > row_max) row_max = bloom_upsample_row[hx];
        }
        if (row_max <= 0.004f) {
            continue;
        }
        for (int x = 0; x < SCREEN_W; ++x) {
            int hx0 = (x - 1) >> 1;
            int hx1 = hx0 + 1;
            float tx = (x & 1) ? 0.25f : 0.75f;
            if (hx0 < 0) hx0 = 0;
            if (hx1 >= half_w) hx1 = half_w - 1;
            float bloom = bloom_upsample_row[hx0] + (bloom_upsample_row[hx1] - bloom_upsample_row[hx0]) * tx;
            if (bloom > 0.004f) {
                int idx = y * SCREEN_W + x;
                uint32_t c = framebuffer[idx];
                uint32_t tint = lerp_color_q8(bloom_color, c, 97u);
                framebuffer[idx] = add_color(c, tint, bloom * strength);
            }
        }
    }
}

/* Per-channel floor((a + b) / 2); matches mix_color(a, b, 0.5) exactly. */
static uint32_t average_color(uint32_t a, uint32_t b)
{
    return 0xFF000000u | (((a & 0xFEFEFEu) >> 1) + ((b & 0xFEFEFEu) >> 1) + (a & b & 0x010101u));
}

void render_edge_antialias(void)
{
    int limit_y = SCREEN_H - HUD_HEIGHT;
    int pixels = SCREEN_W * limit_y;
    for (int i = 0; i < pixels; ++i) {
        post_luma_buffer[i] = luma_u8(framebuffer[i]);
    }

    /* Filter in place: keep the unfiltered previous and current rows, the next row is still untouched. */
    uint32_t *prev_row = post_row_buffer[0];
    uint32_t *row = post_row_buffer[1];
    memcpy(prev_row, framebuffer, (size_t)SCREEN_W * sizeof(framebuffer[0]));
    for (int y = 1; y < limit_y - 1; ++y) {
        memcpy(row, framebuffer + y * SCREEN_W, (size_t)SCREEN_W * sizeof(framebuffer[0]));
        for (int x = 1; x < SCREEN_W - 1; ++x) {
            int idx = y * SCREEN_W + x;
            int lc = post_luma_buffer[idx];
            int ll = post_luma_buffer[idx - 1];
            int lr = post_luma_buffer[idx + 1];
            int lu = post_luma_buffer[idx - SCREEN_W];
            int ld = post_luma_buffer[idx + SCREEN_W];
            int lmin = lc;
            int lmax = lc;
            if (ll < lmin) lmin = ll;
            if (lr < lmin) lmin = lr;
            if (lu < lmin) lmin = lu;
            if (ld < lmin) lmin = ld;
            if (ll > lmax) lmax = ll;
            if (lr > lmax) lmax = lr;
            if (lu > lmax) lmax = lu;
            if (ld > lmax) lmax = ld;

            int contrast = lmax - lmin;
            if (contrast <= 28) {
                continue;
            }
            if (lc > 214 && contrast > 115) {
                continue;
            }

            int horizontal_delta = abs(ll - lr);
            int vertical_delta = abs(lu - ld);
            uint32_t target = horizontal_delta < vertical_delta
                ? average_color(row[x - 1], row[x + 1])
                : average_color(prev_row[x], framebuffer[idx + SCREEN_W]);
            double amount = ((double)contrast / 255.0 - 0.11) * 1.45;
            if (amount > 0.34) {
                amount = 0.34;
            }
            framebuffer[idx] = mix_color(row[x], target, amount);
        }
        uint32_t *swap = prev_row;
        prev_row = row;
        row = swap;
    }
}

static uint8_t lut_channel(double value)
{
    value = clamp01(value);
    return clamp_u8((int)(value * 255.0 + 0.5));
}

static double color_grade_channel_value(int effects, int channel, double value, double luma)
{
    switch (normalize_render_effects(effects)) {
    case RENDER_EFFECTS_PRESET1:
        if (channel == 0) return pow(clamp01(value * 1.06 + luma * 0.025), 0.96);
        if (channel == 1) return pow(clamp01(value * 1.02 + luma * 0.018), 0.98);
        return pow(clamp01(value * 0.96 + luma * 0.010), 1.03);
    case RENDER_EFFECTS_PRESET2:
        if (channel == 0) return pow(clamp01(value * 0.88 + luma * 0.035), 1.08);
        if (channel == 1) return clamp01(value * 1.00 + luma * 0.025);
        return pow(clamp01(value * 1.14 + luma * 0.030), 0.94);
    case RENDER_EFFECTS_PRESET3:
        if (channel == 0) return pow(clamp01(value * 1.18 + luma * 0.040), 0.90);
        if (channel == 1) return pow(clamp01(value * 0.94 + luma * 0.020), 1.02);
        return pow(clamp01(value * 0.82 + luma * 0.010), 1.12);
    default:
        return value;
    }
}

void build_color_grade_lut(void)
{
    for (int effects = RENDER_EFFECTS_PRESET1; effects < RENDER_EFFECTS_COUNT; ++effects) {
        for (int channel = 0; channel < 3; ++channel) {
            for (int luma = 0; luma < 256; ++luma) {
                double l = luma / 255.0;
                for (int value = 0; value < 256; ++value) {
                    color_grade_lut[effects][channel][luma][value] =
                        lut_channel(color_grade_channel_value(effects, channel, value / 255.0, l));
                }
            }
        }
    }
    color_grade_lut_ready = 1;
}

void render_muzzle_light(const GameState *game)
{
    if (game->muzzle_light <= 0.0) {
        return;
    }

    double t = clamp01(game->muzzle_light / MUZZLE_LIGHT_TIME);
    int cx = SCREEN_W / 2;
    int cy = SCREEN_H / 2 + 8;
    int mx = SCREEN_W / 2 + 31;
    int my = SCREEN_H - HUD_HEIGHT - 108 * SCREEN_H / 480;
    int min_x = cx - 220;
    int max_x = cx + 220;
    int min_y = cy - 130;
    int max_y = my + 44;
    if (min_x < 0) min_x = 0;
    if (max_x >= SCREEN_W) max_x = SCREEN_W - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= SCREEN_H - HUD_HEIGHT) max_y = SCREEN_H - HUD_HEIGHT - 1;

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

void render_forest_weather_overlay(const GameState *game)
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
            if (y + 1 < SCREEN_H - HUD_HEIGHT) {
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

void render_color_grade(const GameState *game, int effects)
{
    effects = normalize_render_effects(effects);
    if (effects == RENDER_EFFECTS_OFF) {
        return;
    }

    if (!vignette_ready) {
        for (int y = 0; y < SCREEN_H - HUD_HEIGHT; ++y) {
            double ny = (y - SCREEN_H * 0.5) / (SCREEN_H * 0.5);
            for (int x = 0; x < SCREEN_W; ++x) {
                double nx = (x - SCREEN_W * 0.5) / (SCREEN_W * 0.5);
                vignette_buffer[y * SCREEN_W + x] = (uint16_t)(clamp01((nx * nx + ny * ny) * 0.34) * 65535.0 + 0.5);
            }
        }
        vignette_ready = 1;
    }
    if (!color_grade_lut_ready) {
        build_color_grade_lut();
    }

    double contrast = 1.12;
    double brightness = -5.0;
    uint32_t vignette_tint = rgb(20, 28, 30);
    double vignette_scale = 0.45;
    uint32_t wash = rgb(0, 0, 0);
    double wash_amount = 0.0;
    if (effects == RENDER_EFFECTS_PRESET2) {
        contrast = 1.06;
        brightness = -2.0;
        vignette_tint = rgb(14, 24, 38);
        vignette_scale = 0.38;
        wash = rgb(54, 80, 112);
        wash_amount = 0.045;
    } else if (effects == RENDER_EFFECTS_PRESET3) {
        contrast = 1.18;
        brightness = -7.0;
        vignette_tint = rgb(34, 20, 14);
        vignette_scale = 0.50;
        wash = rgb(118, 56, 28);
        wash_amount = 0.040;
    } else if (game->generator_mode == GENERATOR_FOREST) {
        contrast = 1.08;
        brightness = 6.0;
        vignette_tint = rgb(22, 34, 48);
        vignette_scale = 0.30;
        wash = rgb(82, 110, 134);
        wash_amount = 0.055;
    }

    /* Contrast and wash are fixed per-channel curves; only the vignette varies per pixel. */
    uint8_t contrast_lut[256];
    uint8_t wash_lut[3][256];
    for (int v = 0; v < 256; ++v) {
        contrast_lut[v] = clamp_u8((int)((v - 128) * contrast + 128 + brightness));
        for (int ch = 0; ch < 3; ++ch) {
            int w = (int)((wash >> (16 - ch * 8)) & 0xFFu);
            wash_lut[ch][v] = clamp_u8((int)(v + (w - v) * wash_amount));
        }
    }
    int tint_r = (int)((vignette_tint >> 16) & 0xFFu);
    int tint_g = (int)((vignette_tint >> 8) & 0xFFu);
    int tint_b = (int)(vignette_tint & 0xFFu);
    uint32_t vignette_q = (uint32_t)(vignette_scale * 256.0 + 0.5);
    const uint8_t (*grade)[256][256] = color_grade_lut[effects];

    int pixels = SCREEN_W * (SCREEN_H - HUD_HEIGHT);
    for (int idx = 0; idx < pixels; ++idx) {
        uint32_t c = framebuffer[idx];
        int r = contrast_lut[(c >> 16) & 0xFFu];
        int g = contrast_lut[(c >> 8) & 0xFFu];
        int b = contrast_lut[c & 0xFFu];
        int t = (int)((vignette_buffer[idx] * vignette_q) >> 16);
        r = wash_lut[0][r + (((tint_r - r) * t) >> 8)];
        g = wash_lut[1][g + (((tint_g - g) * t) >> 8)];
        b = wash_lut[2][b + (((tint_b - b) * t) >> 8)];
        int luma = (r * 19595 + g * 38470 + b * 7471 + 32768) >> 16;
        c = rgb(grade[0][luma][r], grade[1][luma][g], grade[2][luma][b]);
        if (light_buffer[idx] > 0.03 || glow_buffer[idx] > 0.03) {
            c = mix_color(c, rgb(255, 132, 48), clamp01((light_buffer[idx] + glow_buffer[idx]) * 0.05));
        }
        framebuffer[idx] = c;
    }
}
