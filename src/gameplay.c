#include "dioom.h"

static void apply_player_damage_from(GameState *game, int damage, Vec2 source, Vec2 player_pos, double shake_strength);
static int move_monster_toward(Monster *monster, Vec2 target, double speed, double dt);
static void spawn_explosion(GameState *game, Vec2 pos, double radius);
static void spawn_gold_drop(GameState *game, Vec2 pos, int amount);
static void explode_player_fireball(GameState *game, Vec2 pos, int damage, double radius);
static int spawn_player_fireball(GameState *game, const Camera *cam);
static void pickup_item(GameState *game, Item *item);
#ifndef __EMSCRIPTEN__
static int weapon_available(const GameState *game, int weapon);
#endif
static void copy_player_progress(GameState *dst, const GameState *src);
static void enter_dungeon_from_forest(GameState *game, Camera *cam, const Portal *portal);
static void enter_house_from_forest(GameState *game, Camera *cam, int house_index);
static void return_to_saved_forest(GameState *game, Camera *cam);
static void mark_current_house_looted(GameState *game, int loot_slot);
static void execute_monster_attack(GameState *game, Monster *monster, const Camera *cam, double player_dist);
static int spawn_boss_minion(GameState *game, const Monster *boss, int type, Vec2 alert_pos);
static void advance_boss_phase(GameState *game, Monster *boss, int phase, Vec2 source);

/* The gate guardian fights in three phases derived from its remaining health, so
 * no extra state has to be stored in saves. Each new phase wakes bodies around it. */
int boss_phase(const GameState *game, const Monster *monster)
{
    if (!monster || !monster->is_boss || !monster->active) return 0;
    int max_hp = adaptive_monster_hp(game, BOSS_HP);
    if (max_hp <= 0) return 0;
    if (monster->hp * 3 <= max_hp) return 2;
    if (monster->hp * 3 <= max_hp * 2) return 1;
    return 0;
}

const Monster *engaged_boss(const GameState *game)
{
    if (!game || game->generator_mode != GENERATOR_BOSS) return NULL;
    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        if (!monster->active || !monster->is_boss) continue;
        int max_hp = adaptive_monster_hp(game, BOSS_HP);
        return monster->ai_state != 0 || monster->hp < max_hp ? monster : NULL;
    }
    return NULL;
}

static int spawn_boss_minion(GameState *game, const Monster *boss, int type, Vec2 alert_pos)
{
    int slot = -1;
    for (int i = 0; i < game->monster_count; ++i) {
        if (!game->monsters[i].active && !game->monsters[i].is_boss) { slot = i; break; }
    }
    if (slot < 0) return 0;
    Vec2 spawn = {0.0, 0.0};
    double best = 1e30;
    for (int y = 1; y < MAP_H - 1; ++y) {
        for (int x = 1; x < MAP_W - 1; ++x) {
            Vec2 p = {x + 0.5, y + 0.5};
            double bx = p.x - boss->pos.x, by = p.y - boss->pos.y;
            double boss_d2 = bx * bx + by * by;
            double px = p.x - alert_pos.x, py = p.y - alert_pos.y;
            if (boss_d2 < 2.25 || boss_d2 > 30.0 || px * px + py * py < 4.0 || occupied_spawn_tile(game, x, y) ||
                !can_occupy(p.x, p.y, 0.28) || !has_line_of_sight(boss->pos, p)) continue;
            if (boss_d2 < best) { best = boss_d2; spawn = p; }
        }
    }
    if (spawn.x == 0.0) return 0;
    game->monsters[slot] = (Monster){
        .active = 1, .hp = adaptive_monster_hp(game, monster_max_hp(type)),
        .pos = spawn, .type = type, .facing = {0.0, -1.0}, .route = slot,
        .patrol = {spawn, boss->pos}, .patrol_count = 2, .target_waypoint = 1,
        .last_seen = alert_pos, .alert_timer = 4.0, .ai_state = 1, .shoot_timer = 0.9,
        .strafe_timer = 0.4, .strafe_dir = (slot & 1) ? 1 : -1,
    };
    spawn_decal(game, spawn, 15, 0.7, 40.0, 0.0);
    return 1;
}

static void advance_boss_phase(GameState *game, Monster *boss, int phase, Vec2 source)
{
    story_dread = 3.0;
    game->screen_shake_timer = 0.35;
    game->screen_shake_strength = 0.05;
    play_sfx(SFX_LOCKED, 0.5);
    if (phase == 1) {
        spawn_boss_minion(game, boss, 3, source);
        spawn_boss_minion(game, boss, 2, source);
        story_message("STRAZNIK BUDZI KORZENIE. CIALA WSTAJA.");
    } else {
        spawn_boss_minion(game, boss, MONSTER_FLYING_HEAD, source);
        story_message("STRAZNIK: GLOSY SA MOJE. ZOSTAN W CISZY.");
    }
}

int player_max_health(const GameState *game)
{
    int upgrades = game ? game->max_health_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_HEALTH_UPGRADES) upgrades = MAX_HEALTH_UPGRADES;
    return PLAYER_MAX_HEALTH + upgrades * HEALTH_UPGRADE_AMOUNT;
}

int pistol_ammo_cap(const GameState *game)
{
    int upgrades = game ? game->ammo_cap_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_AMMO_CAP_UPGRADES) upgrades = MAX_AMMO_CAP_UPGRADES;
    return MAX_PISTOL_AMMO + upgrades * AMMO_CAP_UPGRADE_AMOUNT;
}

int weapon_damage_bonus(const GameState *game)
{
    int upgrades = game ? game->damage_upgrades : 0;
    if (upgrades < 0) upgrades = 0;
    if (upgrades > MAX_DAMAGE_UPGRADES) upgrades = MAX_DAMAGE_UPGRADES;
    return upgrades;
}

int scale_monster_hp_for_difficulty(int hp, int difficulty)
{
    double scale = 1.0;
    switch (normalize_difficulty(difficulty)) {
    case DIFFICULTY_EASY:
        scale = 0.75;
        break;
    case DIFFICULTY_HARD:
        scale = 1.35;
        break;
    case DIFFICULTY_NIGHTMARE:
        scale = 1.80;
        break;
    default:
        break;
    }
    int scaled = (int)(hp * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

int scale_enemy_damage_for_difficulty(const GameState *game, int damage)
{
    double scale = 1.0;
    switch (normalize_difficulty(game ? game->difficulty : DIFFICULTY_NORMAL)) {
    case DIFFICULTY_EASY:
        scale = 0.70;
        break;
    case DIFFICULTY_HARD:
        scale = 1.35;
        break;
    case DIFFICULTY_NIGHTMARE:
        scale = 1.75;
        break;
    default:
        break;
    }
    int scaled = (int)(damage * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

static void apply_player_damage_from(GameState *game, int damage, Vec2 source, Vec2 player_pos, double shake_strength)
{
    if (game->trainer) {
        return;
    }
    int dealt = adaptive_enemy_damage(scale_enemy_damage_for_difficulty(game, damage));
    game->player_health -= dealt;
    adaptive_note_damage_taken(dealt);
    Vec2 dir = {
        source.x - player_pos.x,
        source.y - player_pos.y,
    };
    double dir_len = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (dir_len > 0.0001) {
        dir.x /= dir_len;
        dir.y /= dir_len;
    } else {
        dir = (Vec2){0.0, 0.0};
    }
    game->damage_dir_x = dir.x;
    game->damage_dir_y = dir.y;
    game->hit_flash = 0.18;
    game->player_damage_flash = PLAYER_DAMAGE_FLASH_TIME;
    game->screen_shake_timer = SCREEN_SHAKE_TIME;
    if (shake_strength > game->screen_shake_strength) {
        game->screen_shake_strength = shake_strength;
    }
    if (game->player_health <= 0) {
        game->player_health = 0;
        game->game_over = 1;
        adaptive_note_death();
    }
}

void apply_player_damage(GameState *game, int damage)
{
    apply_player_damage_from(game, damage, (Vec2){0.0, 0.0}, (Vec2){0.0, 0.0}, 0.10);
}

int move_monster_by(Monster *monster, Vec2 delta)
{
    int moved = 0;
    double nx = monster->pos.x + delta.x;
    double ny = monster->pos.y + delta.y;

    if (can_occupy(nx, ny, 0.28) && can_step_between(monster->pos, (Vec2){nx, ny}, 0.28)) {
        monster->pos.x = nx;
        monster->pos.y = ny;
        moved = 1;
    } else {
        if (delta.x != 0.0 && can_occupy(nx, monster->pos.y, 0.28) && can_step_between(monster->pos, (Vec2){nx, monster->pos.y}, 0.28)) {
            monster->pos.x = nx;
            moved = 1;
        }
        if (delta.y != 0.0 && can_occupy(monster->pos.x, ny, 0.28) && can_step_between(monster->pos, (Vec2){monster->pos.x, ny}, 0.28)) {
            monster->pos.y = ny;
            moved = 1;
        }
    }

    if (moved && monster->facing_lock <= 0.0 && (delta.x != 0.0 || delta.y != 0.0)) {
        monster->facing = vec_norm(delta);
    }
    return moved;
}

static int move_monster_toward(Monster *monster, Vec2 target, double speed, double dt)
{
    Vec2 to_target = {
        target.x - monster->pos.x,
        target.y - monster->pos.y,
    };
    double dist = vec_len(to_target);
    if (dist < 0.05) {
        return 1;
    }

    Vec2 dir = vec_norm(to_target);
    double step = speed * dt;
    if (step > dist) {
        step = dist;
    }
    Vec2 delta = {dir.x * step, dir.y * step};
    if (move_monster_by(monster, delta)) {
        return 1;
    }

    Vec2 side = {-dir.y * monster->strafe_dir * step, dir.x * monster->strafe_dir * step};
    if (move_monster_by(monster, side)) {
        return 1;
    }

    monster->strafe_dir *= -1;
    side.x = -side.x;
    side.y = -side.y;
    return move_monster_by(monster, side);
}

void update_monster(GameState *game, int monster_index, const Camera *cam, double dt)
{
    Monster *monster = &game->monsters[monster_index];
    if (!monster->active) {
        return;
    }
    const Vec2 *waypoints = monster->patrol;
    int waypoint_count = monster->patrol_count;
    if (waypoint_count <= 0) {
        return;
    }

    Vec2 to_player = {
        cam->pos.x - monster->pos.x,
        cam->pos.y - monster->pos.y,
    };
    double player_dist = 0.0;
    int sees_player = monster_can_directly_see_player(monster, cam, &player_dist);
    int nearby_witness = !sees_player && monster_has_nearby_witness(game, monster_index, cam);

    if (sees_player) {
        monster->ai_state = player_dist < 6.5 ? 2 : 1;
        monster->last_seen = cam->pos;
        monster->alert_timer = 3.2;
        if (monster->facing_lock <= 0.0) {
            monster->facing = vec_norm(to_player);
        }
    } else if (nearby_witness) {
        monster->ai_state = 1;
        monster->last_seen = cam->pos;
        if (monster->alert_timer < 1.8) {
            monster->alert_timer = 1.8;
        }
        if (monster->facing_lock <= 0.0) {
            monster->facing = vec_norm(to_player);
        }
    } else if (monster->alert_timer > 0.0) {
        monster->alert_timer -= dt;
        if (monster->alert_timer < 0.0) {
            monster->alert_timer = 0.0;
        }
        monster->ai_state = 1;
    } else {
        monster->ai_state = 0;
    }

    monster->strafe_timer -= dt;
    if (monster->strafe_timer <= 0.0) {
        monster->strafe_timer = (monster->type == 2 ? 0.38 : (monster->type == 0 ? 1.05 : 0.70)) + 0.09 * (monster->route % 4);
        monster->strafe_dir *= -1;
    }

    if (monster->ai_state == 2 && sees_player) {
        Vec2 dir_to_player = vec_norm(to_player);
        double preferred = monster->is_boss ? 3.4 : monster_preferred_distance(monster->type);
        double speed_scale = monster_is_behind_player(monster, cam) ? 0.46 : 1.0;
        if (player_dist > preferred + 0.7) {
            double attack_speed = monster->is_boss ? 0.95 + 0.25 * boss_phase(game, monster) : monster_attack_speed(monster->type);
            move_monster_toward(monster, cam->pos, attack_speed * speed_scale, dt);
        } else if (player_dist < preferred - 0.8) {
            Vec2 away = {
                monster->pos.x - dir_to_player.x,
                monster->pos.y - dir_to_player.y,
            };
            move_monster_toward(monster, away, monster_retreat_speed(monster->type) * speed_scale, dt);
        } else {
            double strafe_speed = (monster->is_boss ? 0.50 : (monster->type == 2 ? 1.55 : (monster->type == 0 ? 0.72 : 1.05))) * speed_scale;
            Vec2 side = {
                -dir_to_player.y * monster->strafe_dir * strafe_speed * dt,
                dir_to_player.x * monster->strafe_dir * strafe_speed * dt,
            };
            move_monster_by(monster, side);
        }
        if (monster->facing_lock <= 0.0) {
            monster->facing = dir_to_player;
        }
        return;
    }

    if (monster->ai_state == 1) {
        double speed_scale = monster_is_behind_player(monster, cam) ? 0.42 : (nearby_witness ? 0.58 : 0.72);
        if (move_monster_toward(monster, monster->last_seen, monster_chase_speed(monster->type) * speed_scale, dt)) {
            Vec2 to_last_seen = {
                monster->last_seen.x - monster->pos.x,
                monster->last_seen.y - monster->pos.y,
            };
            if (!sees_player && vec_len(to_last_seen) < 0.25) {
                monster->alert_timer = 0.0;
                monster->ai_state = 0;
            }
        }
        return;
    }

    Vec2 target = waypoints[monster->target_waypoint];
    Vec2 to_target = {
        target.x - monster->pos.x,
        target.y - monster->pos.y,
    };
    if (vec_len(to_target) < 0.08) {
        monster->target_waypoint = (monster->target_waypoint + 1) % waypoint_count;
        return;
    }
    if (!move_monster_toward(monster, target, monster_patrol_speed(monster->type), dt)) {
        monster->target_waypoint = (monster->target_waypoint + 1) % waypoint_count;
    }
}

void spawn_decal(GameState *game, Vec2 pos, int variant, double radius, double life, double angle)
{
    int slot = -1;
    double weakest_life = 1e30;
    for (int i = 0; i < MAX_DECALS; ++i) {
        Decal *decal = &game->decals[i];
        if (!decal->active) {
            slot = i;
            break;
        }
        if (decal->life < weakest_life) {
            weakest_life = decal->life;
            slot = i;
        }
    }
    if (slot < 0) {
        return;
    }
    if (radius < 0.08) {
        radius = 0.08;
    }
    if (life < 0.25) {
        life = 0.25;
    }

    Decal *decal = &game->decals[slot];
    memset(decal, 0, sizeof(*decal));
    decal->active = 1;
    decal->type = variant;
    decal->variant = variant % DECAL_COUNT;
    if (decal->variant < 0) {
        decal->variant = 0;
    }
    decal->pos = pos;
    decal->radius = radius;
    decal->life = life;
    decal->max_life = life;
    decal->angle = angle;
}

static void spawn_explosion(GameState *game, Vec2 pos, double radius)
{
    play_sfx(SFX_EXPLOSION, 0.52);
    game->screen_shake_timer = SCREEN_SHAKE_TIME;
    double shake = clamp01(radius * 0.10) * 0.20;
    if (shake > game->screen_shake_strength) {
        game->screen_shake_strength = shake;
    }
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_NONE;
            p->type = PROJECTILE_EXPLOSION;
            p->pos = pos;
            p->life = 0.22;
            p->radius = radius;
            break;
        }
    }

    spawn_decal(game, pos, 2 + (game->kills % 2) * 10, radius * 0.92, 42.0, game->time * 0.73);

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < MAX_PARTICLES; ++j) {
            Particle *p = &game->particles[j];
            if (!p->active) {
                double a = i * (M_PI * 2.0 / 10.0);
                double speed = 0.28 + (i % 3) * 0.10;
                p->active = 1;
                p->type = PARTICLE_SMOKE;
                p->pos = pos;
                p->vel = (Vec2){cos(a) * speed, sin(a) * speed};
                p->life = 0.85 + (i % 4) * 0.08;
                p->max_life = p->life;
                p->size = 0.32 + (i % 3) * 0.08;
                p->color = i & 1 ? rgb(96, 74, 58) : rgb(146, 70, 34);
                break;
            }
        }
    }
}

static void spawn_gold_drop(GameState *game, Vec2 pos, int amount)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            item->active = 1;
            item->type = ITEM_GOLD;
            item->relic_index = amount;
            item->pos = pos;
            return;
        }
    }
}

void damage_monster(GameState *game, Monster *monster, int damage, Vec2 source)
{
    if (!monster->active) {
        return;
    }

    int phase_before = boss_phase(game, monster);
    monster->hp -= damage;
    monster->pain_timer = monster->is_boss ? 0.32 : 0.18;
    monster->alert_timer = 5.0;
    Vec2 push = vec_norm((Vec2){
        monster->pos.x - source.x,
        monster->pos.y - source.y,
    });
    if (push.x != 0.0 || push.y != 0.0) {
        move_monster_by(monster, (Vec2){push.x * 0.08, push.y * 0.08});
    }
    spawn_decal(
        game,
        monster->pos,
        monster->hp <= 0 ? 15 : (damage > 3 ? 0 : 1),
        monster->is_boss ? 1.15 : (0.38 + damage * 0.05),
        monster->hp <= 0 ? 55.0 : 34.0,
        atan2(push.y, push.x));

    if (monster->hp <= 0) {
        int boss_killed = monster->is_boss && game->generator_mode == GENERATOR_BOSS;
        monster->active = 0;
        game->kills += 1;
        adaptive_note_kill();
        play_sfx(SFX_DEATH, 0.55);
        spawn_explosion(game, monster->pos, boss_killed ? 1.35 : 0.80);
        spawn_gold_drop(game, monster->pos, monster->is_boss ? 70 : (6 + monster->type * 4));
        if (boss_killed) {
            game->victory = 1;
            story_discover(13);
        }
    } else {
        play_sfx(SFX_HURT, 0.42);
        int phase_after = boss_phase(game, monster);
        if (game->generator_mode == GENERATOR_BOSS && phase_after > phase_before) advance_boss_phase(game, monster, phase_after, source);
    }
}

static void explode_player_fireball(GameState *game, Vec2 pos, int damage, double radius)
{
    double radius2 = radius * radius;

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }

        double dx = monster->pos.x - pos.x;
        double dy = monster->pos.y - pos.y;
        double dist2 = dx * dx + dy * dy;
        if (dist2 > radius2) {
            continue;
        }

        double dist = sqrt(dist2);
        int splash = (int)ceil(damage * (1.0 - dist / radius));
        if (splash < 1) {
            splash = 1;
        }
        damage_monster(game, monster, splash, pos);
    }

    spawn_explosion(game, pos, radius);
}

void spawn_monster_shot(GameState *game, const Monster *monster, const Camera *cam)
{
    if (!monster_uses_projectile(monster->type) && !monster->is_boss) {
        return;
    }

    Vec2 dir = vec_norm((Vec2){
        cam->pos.x - monster->pos.x,
        cam->pos.y - monster->pos.y,
    });

    if (dir.x == 0.0 && dir.y == 0.0) {
        return;
    }

    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_ENEMY;
            p->type = PROJECTILE_ENEMY_BOLT;
            p->damage = monster->is_boss ? 18 : monster_projectile_damage(monster->type);
            p->radius = monster->is_boss ? 1.0 : 0.0;
            p->pos = (Vec2){
                monster->pos.x + dir.x * 0.55,
                monster->pos.y + dir.y * 0.55,
            };
            double speed = monster->is_boss ? 3.15 + 0.45 * boss_phase(game, monster) : 4.2;
            p->vel = (Vec2){dir.x * speed, dir.y * speed};
            p->life = monster->is_boss ? 3.0 : 2.2;
            play_sfx(SFX_FIREBALL, monster->is_boss ? 0.38 : 0.24);
            return;
        }
    }
}

static int spawn_player_fireball(GameState *game, const Camera *cam)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            memset(p, 0, sizeof(*p));
            p->active = 1;
            p->owner = PROJECTILE_OWNER_PLAYER;
            p->type = PROJECTILE_PLAYER_FIREBALL;
            p->damage = FIREBALL_SPLASH_DAMAGE + (game->damage_timer > 0.0 ? 2 : 0);
            p->radius = FIREBALL_RADIUS;
            p->pos = (Vec2){
                cam->pos.x + cam->dir.x * 0.48,
                cam->pos.y + cam->dir.y * 0.48,
            };
            p->vel = (Vec2){cam->dir.x * 5.4, cam->dir.y * 5.4};
            p->life = 2.4;
            return 1;
        }
    }
    return 0;
}

void update_projectiles(GameState *game, const Camera *cam, double dt)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) {
            continue;
        }

        p->life -= dt;
        if (p->type == PROJECTILE_EXPLOSION) {
            if (p->life <= 0.0) {
                p->active = 0;
            }
            continue;
        }

        Vec2 old_pos = p->pos;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;

        if (p->life <= 0.0 ||
            map_at((int)p->pos.x, (int)p->pos.y) != 0 ||
            houses_block_area(game, p->pos.x, p->pos.y, 0.08) ||
            props_block_area(game, p->pos.x, p->pos.y, 0.08)) {
            if (p->type == PROJECTILE_PLAYER_FIREBALL) {
                explode_player_fireball(game, old_pos, p->damage, p->radius);
            }
            p->active = 0;
            continue;
        }

        if (p->owner == PROJECTILE_OWNER_ENEMY && !game->victory && !game->game_over) {
            double dx = p->pos.x - cam->pos.x;
            double dy = p->pos.y - cam->pos.y;
            if (dx * dx + dy * dy < 0.18 && game->hit_flash <= 0.0) {
                p->active = 0;
                apply_player_damage_from(game, p->damage, p->pos, cam->pos, p->radius > 0.5 ? 0.22 : 0.10);
                play_sfx(SFX_HURT, 0.44);
            }
        } else if (p->owner == PROJECTILE_OWNER_PLAYER) {
            for (int m = 0; m < game->monster_count; ++m) {
                Monster *monster = &game->monsters[m];
                if (!monster->active) {
                    continue;
                }
                double dx = p->pos.x - monster->pos.x;
                double dy = p->pos.y - monster->pos.y;
                double hit_radius2 = monster->is_boss ? 1.05 * 1.05 : 0.36;
                if (dx * dx + dy * dy < hit_radius2) {
                    int direct = FIREBALL_DIRECT_DAMAGE + (game->damage_timer > 0.0 ? 2 : 0);
                    damage_monster(game, monster, direct, p->pos);
                    explode_player_fireball(game, p->pos, p->damage, p->radius);
                    p->active = 0;
                    break;
                }
            }
        }
    }
}

static void pickup_item(GameState *game, Item *item)
{
    if (item->type == ITEM_SEAL || (item->type == ITEM_RELIC && !story_relic_unsealed(game, item))) return;
    switch (item->type) {
    case ITEM_HEALTH:
        game->player_health += 45;
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        break;
    case ITEM_AMMO:
        game->ammo += 18;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        break;
    case ITEM_RAPID:
        game->rapid_timer = 12.0;
        break;
    case ITEM_DAMAGE:
        game->damage_timer = 12.0;
        break;
    case ITEM_FIREBALL:
        game->fireball_unlocked = 1;
        game->selected_weapon = WEAPON_FIREBALL;
        game->fireball_ammo += 6;
        if (game->fireball_ammo > MAX_FIREBALL_AMMO) {
            game->fireball_ammo = MAX_FIREBALL_AMMO;
        }
        break;
    case ITEM_PISTOL:
        game->pistol_unlocked = 1;
        game->selected_weapon = WEAPON_PISTOL;
        game->ammo += 12;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        break;
    case ITEM_GOLD:
        game->gold += item->relic_index > 0 ? item->relic_index : 5;
        if (game->gold > 999) {
            game->gold = 999;
        }
        break;
    case ITEM_SHRINE:
        if (item->relic_index == 0) {
            game->player_health += 70;
            if (game->player_health > player_max_health(game)) {
                game->player_health = player_max_health(game);
            }
        } else if (item->relic_index == 1) {
            game->damage_timer = 18.0;
            game->rapid_timer = 10.0;
        } else {
            game->fireball_unlocked = 1;
            game->fireball_ammo += 4;
            if (game->fireball_ammo > MAX_FIREBALL_AMMO) {
                game->fireball_ammo = MAX_FIREBALL_AMMO;
            }
        }
        play_sfx(SFX_SHRINE, 0.44);
        break;
    case ITEM_RELIC:
        if (item->relic_index >= 0 && item->relic_index < RELIC_COUNT) {
            game->relic_mask |= 1 << item->relic_index;
            sync_relic_progress(game);
            story_discover(8 + game->relic_count);
            if (saved_forest.valid) {
                saved_forest.game.relic_mask = game->relic_mask;
                saved_forest.game.relic_count = game->relic_count;
                saved_forest.game.boss_unlocked = game->boss_unlocked;
                saved_forest.game.relic_flash = 1.45;
                saved_forest.game.relic_notice_count = game->relic_count;
            }
            game->relic_flash = 1.45;
            game->relic_notice_count = game->relic_count;
        }
        break;
    default:
        game->keys += 1;
        break;
    }

    game->pickup_flash = 0.22;
    if (item->type == ITEM_RELIC) {
        play_sfx(SFX_RELIC, 0.72);
    } else {
        play_sfx(SFX_PICKUP, item->type == ITEM_KEY ? 0.52 : 0.44);
    }
    item->active = 0;
}

void update_items(GameState *game, const Camera *cam)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game->items[i];
        if (!item->active) {
            continue;
        }
        if (item->type == ITEM_BONEPILE || item->type == ITEM_SEAL) {
            continue;
        }

        double dx = item->pos.x - cam->pos.x;
        double dy = item->pos.y - cam->pos.y;
        if (dx * dx + dy * dy < 0.34) {
            if (item->type == ITEM_RELIC && !story_relic_unsealed(game, item)) {
                story_message("RELIKWIA ZAMKNIETA. ZBADAJ TRZY ZNAKI.");
                continue;
            }
            pickup_item(game, item);
        }
    }
}

void select_weapon(GameState *game, int weapon)
{
    if (weapon == WEAPON_PISTOL && !game->pistol_unlocked) {
        return;
    }
    if (weapon == WEAPON_FIREBALL && !game->fireball_unlocked) {
        return;
    }
    if (weapon == WEAPON_SHOTGUN && !game->shotgun_unlocked) {
        return;
    }
    game->selected_weapon = weapon;
}

#ifndef __EMSCRIPTEN__
static int weapon_available(const GameState *game, int weapon)
{
    if (weapon == WEAPON_KNIFE) return 1;
    if (weapon == WEAPON_PISTOL) return game->pistol_unlocked;
    if (weapon == WEAPON_FIREBALL) return game->fireball_unlocked;
    if (weapon == WEAPON_SHOTGUN) return game->shotgun_unlocked;
    return 0;
}
#endif

#ifndef __EMSCRIPTEN__
void cycle_weapon(GameState *game, int dir)
{
    static const int order[] = {WEAPON_KNIFE, WEAPON_PISTOL, WEAPON_SHOTGUN, WEAPON_FIREBALL};
    int count = (int)(sizeof(order) / sizeof(order[0]));
    int current = 0;
    for (int i = 0; i < count; ++i) {
        if (order[i] == game->selected_weapon) {
            current = i;
            break;
        }
    }
    for (int step = 1; step <= count; ++step) {
        int next = (current + dir * step + count * 4) % count;
        if (weapon_available(game, order[next])) {
            game->selected_weapon = order[next];
            return;
        }
    }
}
#endif

int portal_matches(const Portal *portal, int tx, int ty, int px, int py)
{
    if (!portal->active) {
        return 0;
    }
    if ((portal->x == tx && portal->y == ty) ||
        (portal->x == px && portal->y == py)) {
        return 1;
    }

    double dx = px + 0.5 - (portal->x + 0.5);
    double dy = py + 0.5 - (portal->y + 0.5);
    return dx * dx + dy * dy <= 1.75 * 1.75;
}

int close_help_on_key(GameState *game)
{
    if (game->help_timer <= 0.0 && !game->show_help) {
        return 0;
    }
    game->help_timer = 0.0;
    game->show_help = 0;
    return 1;
}

static void copy_player_progress(GameState *dst, const GameState *src)
{
    dst->player_health = src->player_health;
    dst->ammo = src->ammo;
    dst->fireball_ammo = src->fireball_ammo;
    dst->selected_weapon = src->selected_weapon;
    dst->pistol_unlocked = src->pistol_unlocked;
    dst->fireball_unlocked = src->fireball_unlocked;
    dst->shotgun_unlocked = src->shotgun_unlocked;
    dst->max_health_upgrades = src->max_health_upgrades;
    dst->damage_upgrades = src->damage_upgrades;
    dst->ammo_cap_upgrades = src->ammo_cap_upgrades;
    dst->gold = src->gold;
    dst->rapid_timer = src->rapid_timer;
    dst->damage_timer = src->damage_timer;
    dst->difficulty = src->difficulty;
    dst->trainer = src->trainer;
}

static void enter_dungeon_from_forest(GameState *game, Camera *cam, const Portal *portal)
{
    GameState player_progress = *game;
    int relic_mask = game->relic_mask;
    int relic_count = game->relic_count;
    int boss_unlocked = game->boss_unlocked;
    int boss_gate = portal->boss_gate;
    int relic_index = boss_gate ? -1 : portal->relic_index;

    saved_forest.valid = 1;
    saved_forest.game = *game;
    saved_forest.camera = *cam;
    memcpy(saved_forest.map, level_map, sizeof(level_map));
    memcpy(saved_forest.torches, torches, sizeof(torches));

    init_game_seed(game, runtime_level_seed++, portal->target_mode);
    copy_player_progress(game, &player_progress);
    game->in_dungeon = 1;
    game->relic_mask = relic_mask;
    game->relic_count = relic_count;
    game->boss_unlocked = boss_unlocked;
    game->dungeon_relic_index = relic_index;
    sync_relic_progress(game);
    place_dungeon_exit_portal(game);
    place_dungeon_relic(game);
    if (!place_story_seals(game)) exit(EXIT_FAILURE);
    if (relic_index >= 0 && relic_index < RELIC_COUNT) story_discover(1 + relic_index);
    else if (boss_gate) story_discover(STORY_ENTRY_GATE);
    set_active_music_track(music_track_for_relic(relic_index));
    *cam = (Camera){
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    reveal_fog(game, cam);
    play_sfx(SFX_PORTAL, 0.58);
}

static void enter_house_from_forest(GameState *game, Camera *cam, int house_index)
{
    if (house_index < 0 || house_index >= MAX_HOUSES || !game->houses[house_index].active) {
        return;
    }

    GameState player_progress = *game;
    int relic_mask = game->relic_mask;
    int relic_count = game->relic_count;
    int boss_unlocked = game->boss_unlocked;
    int variant = game->houses[house_index].variant;
    uint32_t loot_mask = game->houses[house_index].loot_mask;

    saved_forest.valid = 1;
    saved_forest.game = *game;
    saved_forest.camera = *cam;
    memcpy(saved_forest.map, level_map, sizeof(level_map));
    memcpy(saved_forest.torches, torches, sizeof(torches));
    saved_forest.game.houses[house_index].visited = 1;

    game->current_house_index = house_index;
    game->current_house_variant = variant;
    game->current_house_loot_mask = loot_mask;
    init_game_seed(game, runtime_level_seed++, GENERATOR_HOUSE);
    copy_player_progress(game, &player_progress);
    game->in_dungeon = 1;
    game->relic_mask = relic_mask;
    game->relic_count = relic_count;
    game->boss_unlocked = boss_unlocked;
    game->dungeon_relic_index = -1;
    game->help_timer = 0.0;
    sync_relic_progress(game);
    set_active_music_track(MUSIC_TRACK_DIES_IRAE);
    *cam = (Camera){
        .pos = {8.35, 12.50},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    reveal_fog(game, cam);
    play_sfx(SFX_DOOR, 0.48);
}

static void return_to_saved_forest(GameState *game, Camera *cam)
{
    if (!saved_forest.valid) {
        return;
    }

    copy_player_progress(&saved_forest.game, game);
    *game = saved_forest.game;
    *cam = saved_forest.camera;
    memcpy(level_map, saved_forest.map, sizeof(level_map));
    build_sector_heights(game);
    memcpy(torches, saved_forest.torches, sizeof(torches));
    active_game = game;
    sync_relic_progress(game);
    adaptive_begin_level();
    apply_forest_relic_escalation(game);
    reveal_fog(game, cam);
    saved_forest.valid = 0;
    set_active_music_track(MUSIC_TRACK_FOREST);
    play_sfx(SFX_PORTAL, 0.52);
}

static void mark_current_house_looted(GameState *game, int loot_slot)
{
    if (!saved_forest.valid || loot_slot < 0 || loot_slot >= 31) {
        return;
    }
    int house_index = game->current_house_index;
    if (house_index < 0 || house_index >= MAX_HOUSES) {
        return;
    }
    uint32_t bit = 1u << loot_slot;
    game->current_house_loot_mask |= bit;
    saved_forest.game.houses[house_index].loot_mask |= bit;
}

void loot_prop(GameState *game, Prop *prop)
{
    if (!prop->active || prop->loot_slot < 0) {
        return;
    }

    int note = story_prop_entry(game, prop);
    if (note >= 0) story_discover(note);
    if (prop->looted) return;
    prop->looted = 1;
    mark_current_house_looted(game, prop->loot_slot);
    Item loot = {
        .active = 1,
        .type = prop->loot_type,
        .relic_index = prop->loot_amount,
        .pos = prop->pos,
    };
    pickup_item(game, &loot);
}

int can_buy_merchant_shop_item(const GameState *game, int item)
{
    if (!game) {
        return 0;
    }
    if (item == SHOP_ITEM_AMMO) {
        return game->gold >= SHOP_AMMO_PRICE && game->ammo < pistol_ammo_cap(game);
    }
    if (item == SHOP_ITEM_HEALTH) {
        return game->gold >= SHOP_HEALTH_PRICE && game->player_health < player_max_health(game);
    }
    if (item == SHOP_ITEM_MAX_HP) {
        return game->gold >= SHOP_MAX_HP_PRICE && game->max_health_upgrades < MAX_HEALTH_UPGRADES;
    }
    if (item == SHOP_ITEM_DAMAGE) {
        return game->gold >= SHOP_DAMAGE_PRICE && game->damage_upgrades < MAX_DAMAGE_UPGRADES;
    }
    if (item == SHOP_ITEM_AMMO_CAP) {
        return game->gold >= SHOP_AMMO_CAP_PRICE && game->ammo_cap_upgrades < MAX_AMMO_CAP_UPGRADES;
    }
    if (item == SHOP_ITEM_SHOTGUN) {
        return game->gold >= SHOP_SHOTGUN_PRICE && !game->shotgun_unlocked;
    }
    return 0;
}

int buy_merchant_shop_item(GameState *game, int item)
{
    if (!can_buy_merchant_shop_item(game, item)) {
        return 0;
    }
    if (item == SHOP_ITEM_AMMO) {
        game->gold -= SHOP_AMMO_PRICE;
        game->ammo += SHOP_AMMO_AMOUNT;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_HEALTH) {
        game->gold -= SHOP_HEALTH_PRICE;
        game->player_health += SHOP_HEALTH_AMOUNT;
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_MAX_HP) {
        game->gold -= SHOP_MAX_HP_PRICE;
        game->max_health_upgrades += 1;
        game->player_health += HEALTH_UPGRADE_AMOUNT;
        if (game->player_health > player_max_health(game)) {
            game->player_health = player_max_health(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_DAMAGE) {
        game->gold -= SHOP_DAMAGE_PRICE;
        game->damage_upgrades += 1;
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_AMMO_CAP) {
        game->gold -= SHOP_AMMO_CAP_PRICE;
        game->ammo_cap_upgrades += 1;
        game->ammo += AMMO_CAP_UPGRADE_AMOUNT;
        if (game->ammo > pistol_ammo_cap(game)) {
            game->ammo = pistol_ammo_cap(game);
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    if (item == SHOP_ITEM_SHOTGUN) {
        game->gold -= SHOP_SHOTGUN_PRICE;
        game->shotgun_unlocked = 1;
        game->selected_weapon = WEAPON_SHOTGUN;
        if (game->ammo < SHOTGUN_AMMO_COST * 2) {
            game->ammo = SHOTGUN_AMMO_COST * 2;
        }
        game->pickup_flash = 0.22;
        return 1;
    }
    return 0;
}

void interact_world(GameState *game, Camera *cam)
{
    int tx = (int)(cam->pos.x + cam->dir.x * 0.95);
    int ty = (int)(cam->pos.y + cam->dir.y * 0.95);
    int px = (int)cam->pos.x;
    int py = (int)cam->pos.y;

    for (int i = 0; i < MAX_PORTALS; ++i) {
        Portal *portal = &game->portals[i];
        if (!portal_matches(portal, tx, ty, px, py)) {
            continue;
        }
        if (portal->exit_to_forest) {
            return_to_saved_forest(game, cam);
        } else if (game->generator_mode == GENERATOR_FOREST) {
            if (portal->boss_gate && !game->boss_unlocked) {
                game->relic_flash = 1.45;
                game->relic_notice_count = 0;
                play_sfx(SFX_LOCKED, 0.42);
                return;
            }
            enter_dungeon_from_forest(game, cam, portal);
        }
        return;
    }

    int house_index = -1;
    if (active_house_prompt(game, cam, &house_index)) {
        if (is_merchant_house_index(house_index)) {
            story_discover(5);
            play_sfx(SFX_SHRINE, 0.28);
            return;
        }
        enter_house_from_forest(game, cam, house_index);
        return;
    }

    int prop_index = active_prop_index(game, cam);
    if (prop_index >= 0) {
        loot_prop(game, &game->props[prop_index]);
        return;
    }

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->x == tx && door->y == ty && !door->open && !door->opening) {
            if (!door->locked || game->keys > 0) {
                door->opening = 1;
                door->locked = 0;
                play_sfx(SFX_DOOR, 0.45);
            } else {
                play_sfx(SFX_LOCKED, 0.42);
            }
            return;
        }
    }

    for (int i = 0; i < MAX_SECRETS; ++i) {
        Secret *secret = &game->secrets[i];
        if (secret->x == tx && secret->y == ty && !secret->open) {
            secret->opening = 1;
            play_sfx(SFX_DOOR, 0.38);
            return;
        }
    }
}

void player_fire(GameState *game, const Camera *cam)
{
    if (game->game_over || game->victory || game->shot_cooldown > 0.0) {
        return;
    }

    if (game->selected_weapon == WEAPON_FIREBALL) {
        if (!game->fireball_unlocked || (!game->trainer && game->fireball_ammo <= 0)) {
            return;
        }
        if (!spawn_player_fireball(game, cam)) {
            return;
        }
        play_sfx(SFX_FIREBALL, 0.62);
        adaptive_note_shot();
        if (!game->trainer) {
            game->fireball_ammo -= 1;
        }
        game->shot_cooldown = game->rapid_timer > 0.0 ? FIREBALL_COOLDOWN_TIME * 0.70 : FIREBALL_COOLDOWN_TIME;
        game->weapon_flash = FIREBALL_FLASH_TIME;
        game->muzzle_light = MUZZLE_LIGHT_TIME;
        game->shot_trace = 0.0;
        return;
    }

    if (game->selected_weapon == WEAPON_KNIFE) {
        play_sfx(SFX_MELEE, 0.48);
        adaptive_note_shot();
        game->shot_cooldown = game->rapid_timer > 0.0 ? KNIFE_COOLDOWN_TIME * 0.62 : KNIFE_COOLDOWN_TIME;
        game->weapon_flash = WEAPON_FLASH_TIME;
        game->shot_trace = 0.0;

        int best = -1;
        double best_depth = 1e30;
        for (int i = 0; i < game->monster_count; ++i) {
            Monster *monster = &game->monsters[i];
            if (!monster->active) {
                continue;
            }

            int screen_x;
            int sprite_h;
            double depth;
            if (!project_sprite(cam, monster->pos, 1.0, &screen_x, &sprite_h, &depth)) {
                continue;
            }
            if (depth > KNIFE_RANGE) {
                continue;
            }

            int aim_window = sprite_h / 2;
            if (aim_window < 16) aim_window = 16;
            if (abs(screen_x - SCREEN_W / 2) > aim_window) {
                continue;
            }
            if (!has_line_of_sight(cam->pos, monster->pos)) {
                continue;
            }
            if (depth < best_depth) {
                best_depth = depth;
                best = i;
            }
        }

        if (best >= 0) {
            Monster *monster = &game->monsters[best];
            int damage = KNIFE_DAMAGE + weapon_damage_bonus(game) + (game->damage_timer > 0.0 ? 2 : 0);
            damage_monster(game, monster, damage, cam->pos);
            adaptive_note_hit();
        }
        return;
    }

    if (game->selected_weapon == WEAPON_SHOTGUN) {
        if (!game->shotgun_unlocked) {
            return;
        }
        if (!game->trainer && game->ammo < SHOTGUN_AMMO_COST) {
            return;
        }

        if (!game->trainer) {
            game->ammo -= SHOTGUN_AMMO_COST;
        }
        play_sfx(SFX_PISTOL, 0.68);
        adaptive_note_shot();
        game->shot_cooldown = game->rapid_timer > 0.0 ? SHOTGUN_COOLDOWN_TIME * 0.55 : SHOTGUN_COOLDOWN_TIME;
        game->weapon_flash = WEAPON_FLASH_TIME;
        game->muzzle_light = MUZZLE_LIGHT_TIME * 1.25;
        game->shot_trace = WEAPON_FLASH_TIME;

        int hits = 0;
        for (int i = 0; i < game->monster_count; ++i) {
            Monster *monster = &game->monsters[i];
            if (!monster->active) {
                continue;
            }

            int screen_x;
            int sprite_h;
            double depth;
            if (!project_sprite(cam, monster->pos, 1.0, &screen_x, &sprite_h, &depth)) {
                continue;
            }
            if (depth > SHOTGUN_RANGE) {
                continue;
            }

            int aim_window = sprite_h / 2 + 18;
            if (aim_window < 34) aim_window = 34;
            if (abs(screen_x - SCREEN_W / 2) > aim_window) {
                continue;
            }
            if (!has_line_of_sight(cam->pos, monster->pos)) {
                continue;
            }

            int damage = SHOTGUN_DAMAGE + (depth < 2.6 ? 2 : 0) + (game->damage_timer > 0.0 ? 2 : 0);
            damage_monster(game, monster, damage, cam->pos);
            monster->hit_rim_timer = HIT_RIM_TIME;
            hits++;
        }
        if (hits > 0) {
            game->hit_marker = HIT_MARKER_TIME;
            adaptive_note_hit();
        }
        return;
    }

    if (!game->pistol_unlocked) {
        return;
    }
    if (!game->trainer && game->ammo <= 0) {
        return;
    }

    if (!game->trainer) {
        game->ammo -= 1;
    }
    play_sfx(SFX_PISTOL, 0.50);
    adaptive_note_shot();
    game->shot_cooldown = game->rapid_timer > 0.0 ? SHOT_COOLDOWN_TIME * 0.48 : SHOT_COOLDOWN_TIME;
    game->weapon_flash = WEAPON_FLASH_TIME;
    game->muzzle_light = MUZZLE_LIGHT_TIME;
    game->shot_trace = WEAPON_FLASH_TIME;

    int best = -1;
    double best_depth = 1e30;
    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }

        int screen_x;
        int sprite_h;
        double depth;
        if (!project_sprite(cam, monster->pos, 1.0, &screen_x, &sprite_h, &depth)) {
            continue;
        }

        int aim_window = sprite_h / 3;
        if (aim_window < 10) aim_window = 10;
        if (abs(screen_x - SCREEN_W / 2) > aim_window) {
            continue;
        }
        if (!has_line_of_sight(cam->pos, monster->pos)) {
            continue;
        }
        if (depth < best_depth) {
            best_depth = depth;
            best = i;
        }
    }

    if (best >= 0) {
        Monster *monster = &game->monsters[best];
        int damage = PLAYER_DAMAGE + weapon_damage_bonus(game) + (game->damage_timer > 0.0 ? 2 : 0);
        damage_monster(game, monster, damage, cam->pos);
        monster->hit_rim_timer = HIT_RIM_TIME;
        game->hit_marker = HIT_MARKER_TIME;
        adaptive_note_hit();
    }
}

static void execute_monster_attack(GameState *game, Monster *monster, const Camera *cam, double player_dist)
{
    if ((monster_uses_projectile(monster->type) || monster->is_boss) &&
        player_dist < (monster->is_boss ? 7.2 : 8.0) &&
        (!monster->is_boss || player_dist > 1.35)) {
        spawn_monster_shot(game, monster, cam);
    } else if (!monster_uses_projectile(monster->type) &&
               player_dist < (monster->is_boss ? 1.25 : monster_melee_range(monster->type))) {
        apply_player_damage_from(
            game,
            monster->is_boss ? 24 : monster_melee_damage(monster->type),
            monster->pos,
            cam->pos,
            monster->is_boss ? 0.24 : 0.12);
        play_sfx(SFX_MELEE, 0.46);
        play_sfx(SFX_HURT, 0.34);
    }
}

void update_game(GameState *game, const Camera *cam, double dt)
{
    game->time += dt;
    if (story_dread > 0.0) story_dread = fmax(0.0, story_dread - dt);
    if (story_notice_time > 0.0) story_notice_time = fmax(0.0, story_notice_time - dt);
    reveal_fog(game, cam);
    int combat_active = !game->victory && !game->game_over;
    adaptive_update(game, dt);

    for (int i = 0; i < MAX_DOORS; ++i) {
        Door *door = &game->doors[i];
        if (door->opening && !door->open) {
            door->open_amount += dt * DOOR_OPEN_SPEED;
            if (door->open_amount >= 1.0) {
                door->open_amount = 1.0;
                door->open = 1;
            }
        }
    }
    for (int i = 0; i < MAX_SECRETS; ++i) {
        Secret *secret = &game->secrets[i];
        if (secret->opening && !secret->open) {
            secret->open_amount += dt * 1.6;
            if (secret->open_amount >= 1.0) {
                secret->open_amount = 1.0;
                secret->open = 1;
                level_map[secret->y][secret->x] = 0;
            }
        }
    }

    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        if (combat_active) {
            update_monster(game, i, cam, dt);
        }
        if (monster->facing_lock > 0.0) {
            monster->facing_lock -= dt;
            if (monster->facing_lock < 0.0) {
                monster->facing_lock = 0.0;
            }
        }
        if (monster->pain_timer > 0.0) {
            monster->pain_timer -= dt;
            if (monster->pain_timer < 0.0) {
                monster->pain_timer = 0.0;
            }
        }
        if (monster->hit_rim_timer > 0.0) {
            monster->hit_rim_timer -= dt;
            if (monster->hit_rim_timer < 0.0) {
                monster->hit_rim_timer = 0.0;
            }
        }
        if (monster->attack_anim_timer > 0.0) {
            monster->attack_anim_timer -= dt;
            if (monster->attack_anim_timer < 0.0) {
                monster->attack_anim_timer = 0.0;
            }
        }
        int windup_ready = 0;
        if (monster->attack_windup_timer > 0.0) {
            monster->attack_windup_timer -= dt;
            if (monster->attack_windup_timer <= 0.0) {
                monster->attack_windup_timer = 0.0;
                windup_ready = 1;
            }
        }

        if (!monster->active) {
            continue;
        }
        if (!combat_active) {
            continue;
        }
        Vec2 to_player = {
            cam->pos.x - monster->pos.x,
            cam->pos.y - monster->pos.y,
        };
        double player_dist = 0.0;
        int can_attack = monster->ai_state == 2 &&
                         monster_can_directly_see_player(monster, cam, &player_dist) &&
                         vec_dot(vec_norm(monster->facing), vec_norm(to_player)) > 0.55;

        if (windup_ready) {
            if (can_attack) {
                execute_monster_attack(game, monster, cam, player_dist);
            }
            continue;
        }
        if (monster->attack_windup_timer > 0.0) {
            continue;
        }

        monster->shoot_timer -= dt;
        if (monster->shoot_timer <= 0.0 && can_attack) {
            monster->facing = vec_norm((Vec2){
                cam->pos.x - monster->pos.x,
                cam->pos.y - monster->pos.y,
            });
            double windup = monster->is_boss ? BOSS_WINDUP_TIME : MONSTER_WINDUP_TIME;
            monster->facing_lock = windup + 0.12;
            monster->attack_anim_timer = windup;
            monster->attack_windup_timer = windup;
            monster->shoot_timer = (monster->is_boss ? 1.65 - 0.35 * boss_phase(game, monster) : monster_shot_cooldown(monster->type, i)) * adaptive_cooldown_scale();
        }
        if (monster->shoot_timer <= 0.0) {
            monster->shoot_timer = 0.25;
        }
    }

    update_projectiles(game, cam, dt);
    if (game->shot_cooldown > 0.0) {
        game->shot_cooldown -= dt;
        if (game->shot_cooldown < 0.0) game->shot_cooldown = 0.0;
    }
    if (game->weapon_flash > 0.0) {
        game->weapon_flash -= dt;
        if (game->weapon_flash < 0.0) game->weapon_flash = 0.0;
    }
    if (game->muzzle_light > 0.0) {
        game->muzzle_light -= dt;
        if (game->muzzle_light < 0.0) game->muzzle_light = 0.0;
    }
    if (game->shot_trace > 0.0) {
        game->shot_trace -= dt;
        if (game->shot_trace < 0.0) game->shot_trace = 0.0;
    }
    if (game->hit_marker > 0.0) {
        game->hit_marker -= dt;
        if (game->hit_marker < 0.0) game->hit_marker = 0.0;
    }
    if (game->pickup_flash > 0.0) {
        game->pickup_flash -= dt;
        if (game->pickup_flash < 0.0) game->pickup_flash = 0.0;
    }
    if (game->relic_flash > 0.0) {
        game->relic_flash -= dt;
        if (game->relic_flash < 0.0) game->relic_flash = 0.0;
    }
    if (game->help_timer > 0.0 && !game->show_help) {
        game->help_timer -= dt;
        if (game->help_timer < 0.0) game->help_timer = 0.0;
    }
    if (game->rapid_timer > 0.0) {
        game->rapid_timer -= dt;
        if (game->rapid_timer < 0.0) game->rapid_timer = 0.0;
    }
    if (game->damage_timer > 0.0) {
        game->damage_timer -= dt;
        if (game->damage_timer < 0.0) game->damage_timer = 0.0;
    }
    if (game->hit_flash > 0.0) {
        game->hit_flash -= dt;
        if (game->hit_flash < 0.0) {
            game->hit_flash = 0.0;
        }
    }
    if (game->player_damage_flash > 0.0) {
        game->player_damage_flash -= dt;
        if (game->player_damage_flash < 0.0) {
            game->player_damage_flash = 0.0;
        }
    }
    if (game->screen_shake_timer > 0.0) {
        game->screen_shake_timer -= dt;
        if (game->screen_shake_timer <= 0.0) {
            game->screen_shake_timer = 0.0;
            game->screen_shake_strength = 0.0;
        }
    }
    for (int i = 0; i < MAX_DECALS; ++i) {
        Decal *decal = &game->decals[i];
        if (decal->active) {
            decal->life -= dt;
            if (decal->life <= 0.0) {
                decal->active = 0;
            }
        }
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        Particle *particle = &game->particles[i];
        if (!particle->active) {
            continue;
        }
        particle->life -= dt;
        if (particle->life <= 0.0) {
            particle->active = 0;
            continue;
        }
        particle->pos.x += particle->vel.x * dt;
        particle->pos.y += particle->vel.y * dt;
        particle->vel.x *= 1.0 - dt * 0.45;
        particle->vel.y *= 1.0 - dt * 0.45;
        if (!can_occupy(particle->pos.x, particle->pos.y, 0.05)) {
            particle->active = 0;
        }
    }

}

void move_camera(Camera *cam, const GameState *game, double forward, double strafe, double dt)
{
    double speed = 3.2 * dt;
    double nx = cam->pos.x + cam->dir.x * forward * speed + cam->plane.x * strafe * speed;
    double ny = cam->pos.y + cam->dir.y * forward * speed + cam->plane.y * strafe * speed;

    if (can_move(nx, cam->pos.y) && can_step_between(cam->pos, (Vec2){nx, cam->pos.y}, 0.18)) {
        cam->pos.x = nx;
    }
    if (can_move(cam->pos.x, ny) && can_step_between(cam->pos, (Vec2){cam->pos.x, ny}, 0.18)) {
        cam->pos.y = ny;
    }

    for (int i = 0; i < game->monster_count; ++i) {
        const Monster *monster = &game->monsters[i];
        if (!monster->active) {
            continue;
        }
        Vec2 diff = {cam->pos.x - monster->pos.x, cam->pos.y - monster->pos.y};
        double dist = vec_len(diff);
        double min_dist = monster->is_boss ? 0.95 : 0.48;
        if (dist > 0.001 && dist < min_dist) {
            Vec2 push = vec_norm(diff);
            double amount = (min_dist - dist) * 0.45;
            double px = cam->pos.x + push.x * amount;
            double py = cam->pos.y + push.y * amount;
            if (can_move(px, cam->pos.y) && can_step_between(cam->pos, (Vec2){px, cam->pos.y}, 0.18)) {
                cam->pos.x = px;
            }
            if (can_move(cam->pos.x, py) && can_step_between(cam->pos, (Vec2){cam->pos.x, py}, 0.18)) {
                cam->pos.y = py;
            }
        }
    }
}

void rotate_camera(Camera *cam, double amount)
{
    double old_dir_x = cam->dir.x;
    double old_plane_x = cam->plane.x;
    double s = sin(amount);
    double c = cos(amount);

    cam->dir.x = cam->dir.x * c - cam->dir.y * s;
    cam->dir.y = old_dir_x * s + cam->dir.y * c;
    cam->plane.x = cam->plane.x * c - cam->plane.y * s;
    cam->plane.y = old_plane_x * s + cam->plane.y * c;
}

void reset_run(GameState *game, Camera *cam)
{
    memset(&story, 0, sizeof(story));
    story_popup = -1;
    story_notice_time = 0.0;
    story_dread = 0.0;
    saved_forest.valid = 0;
    *cam = (Camera){
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    init_game_seed(game, runtime_level_seed++, runtime_level_mode);
    reveal_fog(game, cam);
}
