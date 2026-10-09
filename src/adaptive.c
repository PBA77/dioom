#include "dioom.h"

/* Hidden dynamic difficulty. A skill estimate in [-1, 1] is refined from short
 * combat windows and from deaths, then mapped onto bounded multipliers that sit
 * on top of the chosen difficulty level. Monster HP uses a per-level snapshot so
 * boss phases and spawned enemies stay consistent while the estimate drifts. */

AdaptiveState adaptive = {.level_hp_scale = 1.0};

static double clamp_unit(double v)
{
    return v < -1.0 ? -1.0 : (v > 1.0 ? 1.0 : v);
}

static double adaptive_skill(void)
{
    return clamp_unit(adaptive.skill);
}

void adaptive_reset(void)
{
    memset(&adaptive, 0, sizeof(adaptive));
    adaptive.level_hp_scale = 1.0;
}

double adaptive_hp_scale(void)
{
    return 1.0 + ADAPTIVE_HP_RANGE * adaptive_skill();
}

double adaptive_damage_scale(void)
{
    return 1.0 + ADAPTIVE_DAMAGE_RANGE * adaptive_skill();
}

double adaptive_cooldown_scale(void)
{
    return 1.0 - ADAPTIVE_COOLDOWN_RANGE * adaptive_skill();
}

int adaptive_extra_spawns(void)
{
    double skill = adaptive_skill();
    return skill > 0.5 ? 1 : (skill < -0.5 ? -1 : 0);
}

void adaptive_begin_level(void)
{
    adaptive.level_hp_scale = adaptive_hp_scale();
    adaptive.window_time = 0.0;
    adaptive.window_damage = 0;
    adaptive.window_shots = 0;
    adaptive.window_hits = 0;
    adaptive.window_kills = 0;
    adaptive.low_health_time = 0.0;
}

int adaptive_monster_hp(const GameState *game, int base_hp)
{
    double scale = adaptive.level_hp_scale;
    if (!(scale >= 1.0 - ADAPTIVE_HP_RANGE && scale <= 1.0 + ADAPTIVE_HP_RANGE)) scale = 1.0;
    int hp = scale_monster_hp_for_difficulty(base_hp, game ? game->difficulty : DIFFICULTY_NORMAL);
    int scaled = (int)(hp * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

int adaptive_enemy_damage(int damage)
{
    int scaled = (int)(damage * adaptive_damage_scale() + 0.5);
    return scaled > 0 ? scaled : 1;
}

void adaptive_note_damage_taken(int damage)
{
    if (damage > 0) adaptive.window_damage += damage;
}

void adaptive_note_shot(void)
{
    adaptive.window_shots++;
}

void adaptive_note_hit(void)
{
    adaptive.window_hits++;
}

void adaptive_note_kill(void)
{
    adaptive.window_kills++;
}

void adaptive_note_death(void)
{
    adaptive.deaths++;
    adaptive.skill = clamp_unit(adaptive.skill - ADAPTIVE_DEATH_PENALTY);
    adaptive.window_time = 0.0;
    adaptive.window_damage = 0;
    adaptive.window_shots = 0;
    adaptive.window_hits = 0;
    adaptive.window_kills = 0;
    adaptive.low_health_time = 0.0;
}

void adaptive_update(const GameState *game, double dt)
{
    if (!game || game->trainer || game->game_over || game->victory || dt <= 0.0) return;
    int max_health = player_max_health(game);
    if (max_health <= 0) return;
    double health_frac = (double)game->player_health / max_health;
    adaptive.window_time += dt;
    if (health_frac < 0.3) adaptive.low_health_time += dt;
    if (adaptive.window_time < ADAPTIVE_WINDOW_SECONDS) return;

    int active = adaptive.window_shots > 0 || adaptive.window_damage > 0 || adaptive.window_kills > 0;
    if (active) {
        double damage_frac = (double)adaptive.window_damage / max_health;
        double health_term = (health_frac - 0.55) * 1.2;
        double damage_term = damage_frac > 0.25 ? -(damage_frac - 0.25) * 2.4 : (0.25 - damage_frac) * 1.6;
        double accuracy_term = adaptive.window_shots >= 4
            ? ((double)adaptive.window_hits / adaptive.window_shots - 0.5) * 0.8 : 0.0;
        int kills = adaptive.window_kills > 4 ? 4 : adaptive.window_kills;
        double kill_term = kills * 0.08;
        double low_term = -(adaptive.low_health_time / adaptive.window_time) * 0.6;
        double performance = clamp_unit(health_term + damage_term + accuracy_term + kill_term + low_term);
        adaptive.skill = clamp_unit(adaptive.skill + (performance - adaptive.skill) * ADAPTIVE_EMA_RATE);
        adaptive.evaluations++;
    }
    adaptive.window_time = 0.0;
    adaptive.window_damage = 0;
    adaptive.window_shots = 0;
    adaptive.window_hits = 0;
    adaptive.window_kills = 0;
    adaptive.low_health_time = 0.0;
}

int adaptive_state_valid(const AdaptiveState *state)
{
    return state && isfinite(state->skill) && state->skill >= -1.0 && state->skill <= 1.0 &&
           isfinite(state->level_hp_scale) && state->level_hp_scale >= 1.0 - ADAPTIVE_HP_RANGE - 1e-9 &&
           state->level_hp_scale <= 1.0 + ADAPTIVE_HP_RANGE + 1e-9 &&
           isfinite(state->window_time) && state->window_time >= 0.0 &&
           isfinite(state->low_health_time) && state->low_health_time >= 0.0 &&
           state->window_damage >= 0 && state->window_shots >= 0 && state->window_hits >= 0 &&
           state->window_kills >= 0 && state->deaths >= 0 && state->evaluations >= 0;
}
