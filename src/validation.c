#include "dioom.h"
#include "prompt_font.h"

static int verify_monster_path(void);
static int verify_player_weapon(void);
static int verify_monster_shot_direction(void);
static int verify_monster_ai_reacts(void);
static int verify_monster_safety_rules(void);
static int verify_items_and_fog(void);
static int verify_fireball_weapon(void);
static int verify_projectile_ownership(void);
static int verify_monster_melee_attack(void);
static int verify_monster_roles(void);
static int setup_interaction_camera(Camera *cam, int tx, int ty);
static int verify_doors_and_secrets(void);
static int verify_generator_modes(void);
static int verify_forest_dungeon_transition(void);
static int setup_prop_interaction_camera(const GameState *game, Camera *cam, const Prop *prop);
static int verify_forest_house_transition(void);
static int verify_merchant_shop(void);
static int find_relic_item(const GameState *game, int relic_index);
static int verify_generated_dungeon_relic_reachability(void);
static int verify_relic_story_progress(void);
static int verify_music_track_mapping(void);
static int verify_fm_instrument_distribution(void);
static int verify_render_effect_presets(void);
static int verify_help_key_close(void);
static int verify_prompt_font_glyphs(void);
static int verify_difficulty_scaling(void);
static int verify_boss_sprite_asset(void);
static int verify_furniture_sprite_asset(void);
static int verify_monster_sprite_assets(void);
static int verify_trainer_mode(void);
static int verify_sprite_sort_order(void);
static int verify_torches(void);
static int verify_volumetric_fog(void);
static int verify_tree_visible_at_collision_range(void);
static int write_ppm(const char *path);
static int verify_story_puzzles(void);
static int verify_boss_phases(void);
static int verify_adaptive_difficulty(void);
static int verify_sector_geometry(void);

static int verify_monster_path(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    for (int i = 0; i < 60 * 12; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
        for (int m = 0; m < game.monster_count; ++m) {
            Monster *monster = &game.monsters[m];
            if (monster->active && !can_occupy(monster->pos.x, monster->pos.y, 0.28)) {
                fprintf(stderr, "error: monster %d path entered a wall at %.2f %.2f\n", m, monster->pos.x, monster->pos.y);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_player_weapon(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    if (game.selected_weapon != WEAPON_KNIFE || game.pistol_unlocked || game.fireball_unlocked) {
        fprintf(stderr, "error: player did not start with locked weapons and knife selected\n");
        return 0;
    }
    select_weapon(&game, WEAPON_PISTOL);
    if (game.selected_weapon != WEAPON_KNIFE) {
        fprintf(stderr, "error: locked pistol weapon was selected\n");
        return 0;
    }
    select_weapon(&game, WEAPON_SHOTGUN);
    if (game.selected_weapon != WEAPON_KNIFE) {
        fprintf(stderr, "error: locked shotgun weapon was selected\n");
        return 0;
    }

    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 3,
        .pos = {3.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{3.5, 22.5}},
    };

    player_fire(&game, &cam);
    if (game.ammo != START_AMMO || game.monsters[0].active || game.kills != 1) {
        fprintf(stderr, "error: knife weapon failed close hit verification\n");
        return 0;
    }

    int pistol_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_PISTOL) {
            pistol_item = i;
            break;
        }
    }
    if (pistol_item < 0) {
        fprintf(stderr, "error: no pistol pickup available for verification\n");
        return 0;
    }
    int ammo_after_knife = game.ammo;
    cam.pos = game.items[pistol_item].pos;
    update_items(&game, &cam);
    if (game.items[pistol_item].active || !game.pistol_unlocked || game.selected_weapon != WEAPON_PISTOL || game.ammo <= ammo_after_knife) {
        fprintf(stderr, "error: pistol pickup verification failed\n");
        return 0;
    }

    cam.pos = (Vec2){2.5, 22.5};
    game.kills = 0;
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    int ammo_before_pistol = game.ammo;
    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != ammo_before_pistol - 1 || game.monsters[0].hp != 2 || !game.monsters[0].active) {
        fprintf(stderr, "error: pistol weapon failed first hit verification\n");
        return 0;
    }
    if (game.muzzle_light <= 0.0 || game.hit_marker <= 0.0 || game.monsters[0].hit_rim_timer <= 0.0) {
        fprintf(stderr, "error: pistol hit feedback timers were not armed\n");
        return 0;
    }

    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != ammo_before_pistol - 2 || game.monsters[0].active || game.kills != 1) {
        fprintf(stderr, "error: pistol weapon failed kill verification\n");
        return 0;
    }

    game.shotgun_unlocked = 1;
    game.selected_weapon = WEAPON_SHOTGUN;
    game.ammo = SHOTGUN_AMMO_COST * 2;
    game.kills = 0;
    game.shot_cooldown = 0.0;
    game.monster_count = 2;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 8,
        .pos = {4.5, 22.22},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{4.5, 22.22}},
    };
    game.monsters[1] = (Monster){
        .active = 1,
        .hp = 8,
        .pos = {4.5, 22.78},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{4.5, 22.78}},
    };
    player_fire(&game, &cam);
    if (game.ammo != SHOTGUN_AMMO_COST ||
        (game.monsters[0].hp >= 8 && game.monsters[1].hp >= 8) ||
        game.hit_marker <= 0.0) {
        fprintf(stderr, "error: shotgun spread verification failed\n");
        return 0;
    }

    return 1;
}

static int verify_monster_shot_direction(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    Monster *monster = &game.monsters[0];

    spawn_monster_shot(&game, monster, &cam);
    Projectile *shot = NULL;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active) {
            shot = &game.projectiles[i];
            break;
        }
    }
    if (!shot) {
        fprintf(stderr, "error: monster shot verification did not spawn a projectile\n");
        return 0;
    }

    Vec2 to_player = {
        cam.pos.x - monster->pos.x,
        cam.pos.y - monster->pos.y,
    };
    double dot = shot->vel.x * to_player.x + shot->vel.y * to_player.y;
    double monster_dist = vec_len(to_player);
    double shot_dist = vec_len((Vec2){
        cam.pos.x - shot->pos.x,
        cam.pos.y - shot->pos.y,
    });

    if (dot <= 0.0 || shot_dist >= monster_dist) {
        fprintf(stderr, "error: monster projectile is not moving toward the player\n");
        return 0;
    }

    return 1;
}

static int verify_monster_ai_reacts(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.5, 22.5},
        .type = 0,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{5.5, 22.5}},
    };
    Monster *monster = &game.monsters[0];

    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (monster->ai_state == 0 || monster->alert_timer <= 0.0) {
        fprintf(stderr, "error: monster AI did not react to a visible player\n");
        return 0;
    }

    double dx = monster->last_seen.x - cam.pos.x;
    double dy = monster->last_seen.y - cam.pos.y;
    if (dx * dx + dy * dy > 0.01) {
        fprintf(stderr, "error: monster AI did not remember the player position\n");
        return 0;
    }

    return 1;
}

static int verify_monster_safety_rules(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));

    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {11.5, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{11.5, 22.5}},
    };
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state != 0) {
        fprintf(stderr, "error: monster saw the player from too far away\n");
        return 0;
    }

    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {6.0, 22.5},
        .type = 1,
        .facing = {1.0, 0.0},
        .patrol_count = 1,
        .patrol = {{6.0, 22.5}},
    };
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state != 0) {
        fprintf(stderr, "error: monster saw the player behind its back from medium range\n");
        return 0;
    }

    game.monsters[0].pos = (Vec2){4.5, 22.5};
    game.monsters[0].facing = (Vec2){1.0, 0.0};
    update_monster(&game, 0, &cam, 1.0 / 60.0);
    if (game.monsters[0].ai_state == 0) {
        fprintf(stderr, "error: close monster did not notice the player behind it\n");
        return 0;
    }

    game.monster_count = 2;
    memset(game.monsters, 0, sizeof(game.monsters));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.0, 22.5},
        .type = 1,
        .facing = {-1.0, 0.0},
        .ai_state = 2,
        .patrol_count = 1,
        .patrol = {{5.0, 22.5}},
    };
    game.monsters[1] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {5.0, 20.5},
        .type = 1,
        .facing = {0.0, -1.0},
        .patrol_count = 1,
        .patrol = {{5.0, 20.5}},
    };
    update_monster(&game, 1, &cam, 1.0 / 60.0);
    if (game.monsters[1].ai_state != 1 || game.monsters[1].alert_timer <= 0.0) {
        fprintf(stderr, "error: nearby witness did not alert another monster\n");
        return 0;
    }

    return 1;
}

static int verify_items_and_fog(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *item = &game.items[i];
        if (item->active && !can_occupy(item->pos.x, item->pos.y, 0.12)) {
            fprintf(stderr, "error: item %d is placed inside a wall at %.2f %.2f\n", i, item->pos.x, item->pos.y);
            return 0;
        }
    }

    int ammo_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_AMMO) {
            ammo_item = i;
            break;
        }
    }
    game.ammo = 10;
    if (ammo_item < 0) {
        fprintf(stderr, "error: no ammo pickup available for verification\n");
        return 0;
    }
    cam.pos = game.items[ammo_item].pos;
    update_items(&game, &cam);
    if (game.items[ammo_item].active || game.ammo <= 10 || game.pickup_flash <= 0.0) {
        fprintf(stderr, "error: item pickup verification failed\n");
        return 0;
    }

    int found_fireball = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_FIREBALL) {
            cam.pos = game.items[i].pos;
            update_items(&game, &cam);
            found_fireball = 1;
            break;
        }
    }
    if (!found_fireball || !game.fireball_unlocked || game.selected_weapon != WEAPON_FIREBALL || game.fireball_ammo <= 0) {
        fprintf(stderr, "error: fireball pickup verification failed\n");
        return 0;
    }

    int gold_item = -1;
    int shrine_item = -1;
    int bonepile_item = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game.items[i].active && game.items[i].type == ITEM_GOLD && gold_item < 0) {
            gold_item = i;
        } else if (game.items[i].active && game.items[i].type == ITEM_SHRINE && shrine_item < 0) {
            shrine_item = i;
        } else if (game.items[i].active && game.items[i].type == ITEM_BONEPILE && bonepile_item < 0) {
            bonepile_item = i;
        }
    }
    if (gold_item < 0 || shrine_item < 0 || bonepile_item < 0) {
        fprintf(stderr, "error: generated level is missing Diablo-style pickups or props\n");
        return 0;
    }

    int gold_before = game.gold;
    cam.pos = game.items[gold_item].pos;
    update_items(&game, &cam);
    if (game.items[gold_item].active || game.gold <= gold_before) {
        fprintf(stderr, "error: gold pickup verification failed\n");
        return 0;
    }

    game.player_health = 50;
    game.rapid_timer = 0.0;
    game.damage_timer = 0.0;
    int fireball_before = game.fireball_ammo;
    int shrine_kind = game.items[shrine_item].relic_index;
    cam.pos = game.items[shrine_item].pos;
    update_items(&game, &cam);
    if (game.items[shrine_item].active ||
        (shrine_kind == 0 && game.player_health <= 50) ||
        (shrine_kind == 1 && (game.rapid_timer <= 0.0 || game.damage_timer <= 0.0)) ||
        (shrine_kind == 2 && (!game.fireball_unlocked || game.fireball_ammo <= fireball_before))) {
        fprintf(stderr, "error: shrine pickup verification failed\n");
        return 0;
    }

    game.pickup_flash = 0.0;
    cam.pos = game.items[bonepile_item].pos;
    update_items(&game, &cam);
    if (!game.items[bonepile_item].active || game.pickup_flash > 0.0) {
        fprintf(stderr, "error: bone pile prop should not be picked up\n");
        return 0;
    }

    cam.pos = (Vec2){2.5, 22.5};
    reveal_fog(&game, &cam);
    if (!game.discovered[22][2] || !game.discovered[22][4]) {
        fprintf(stderr, "error: fog-of-war did not reveal the starting area\n");
        return 0;
    }

    return 1;
}

static int verify_fireball_weapon(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    select_weapon(&game, WEAPON_FIREBALL);
    if (game.selected_weapon != WEAPON_KNIFE) {
        fprintf(stderr, "error: locked fireball weapon was selected\n");
        return 0;
    }

    game.fireball_unlocked = 1;
    game.fireball_ammo = 2;
    select_weapon(&game, WEAPON_FIREBALL);
    if (game.selected_weapon != WEAPON_FIREBALL) {
        fprintf(stderr, "error: unlocked fireball weapon was not selected\n");
        return 0;
    }

    game.monster_count = 2;
    game.monsters[0].active = 1;
    game.monsters[0].hp = 10;
    game.monsters[0].pos = (Vec2){6.0, 22.5};
    game.monsters[1].active = 1;
    game.monsters[1].hp = 10;
    game.monsters[1].pos = (Vec2){6.2, 22.8};
    memset(game.projectiles, 0, sizeof(game.projectiles));

    player_fire(&game, &cam);
    if (game.fireball_ammo != 1 || game.shot_cooldown <= 0.0) {
        fprintf(stderr, "error: fireball did not consume ammo and set cooldown\n");
        return 0;
    }

    for (int i = 0; i < 90; ++i) {
        update_projectiles(&game, &cam, 1.0 / 60.0);
    }

    if (game.monsters[0].hp >= 10 || game.monsters[1].hp >= 10) {
        fprintf(stderr, "error: fireball did not apply direct and splash damage\n");
        return 0;
    }

    return 1;
}

static int verify_projectile_ownership(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    game.monster_count = 1;
    game.monsters[0].active = 1;
    game.monsters[0].hp = 10;
    game.monsters[0].pos = (Vec2){3.0, 22.5};
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.projectiles[0] = (Projectile){
        .active = 1,
        .owner = PROJECTILE_OWNER_ENEMY,
        .type = PROJECTILE_ENEMY_BOLT,
        .damage = 99,
        .pos = game.monsters[0].pos,
        .vel = {0.0, 0.0},
        .life = 1.0,
    };
    update_projectiles(&game, &cam, 1.0 / 60.0);
    if (game.monsters[0].hp != 10) {
        fprintf(stderr, "error: enemy projectile damaged a monster\n");
        return 0;
    }

    int health = game.player_health;
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.monster_count = 0;
    game.projectiles[0] = (Projectile){
        .active = 1,
        .owner = PROJECTILE_OWNER_PLAYER,
        .type = PROJECTILE_PLAYER_FIREBALL,
        .damage = 99,
        .radius = FIREBALL_RADIUS,
        .pos = cam.pos,
        .vel = {0.0, 0.0},
        .life = 1.0,
    };
    update_projectiles(&game, &cam, 1.0 / 60.0);
    if (game.player_health != health) {
        fprintf(stderr, "error: player fireball damaged the player\n");
        return 0;
    }

    return 1;
}

static int verify_monster_melee_attack(void)
{
    GameState game;
    init_game(&game);
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    game.monster_count = 1;
    memset(game.monsters, 0, sizeof(game.monsters));
    memset(game.projectiles, 0, sizeof(game.projectiles));
    game.monsters[0] = (Monster){
        .active = 1,
        .hp = 4,
        .pos = {3.15, 22.5},
        .type = 0,
        .facing = {-1.0, 0.0},
        .ai_state = 2,
        .shoot_timer = 0.0,
        .patrol_count = 1,
        .patrol = {{3.15, 22.5}},
    };

    int health = game.player_health;
    update_game(&game, &cam, 1.0 / 60.0);
    if (game.player_health != health || game.monsters[0].attack_windup_timer <= 0.0) {
        fprintf(stderr, "error: melee monster did not telegraph before damage\n");
        return 0;
    }
    for (int i = 0; i < 30; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (game.player_health >= health || game.player_damage_flash <= 0.0 || game.screen_shake_timer <= 0.0) {
        fprintf(stderr, "error: melee monster did not damage the player\n");
        return 0;
    }
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active && game.projectiles[i].owner == PROJECTILE_OWNER_ENEMY) {
            fprintf(stderr, "error: melee monster spawned an enemy projectile\n");
            return 0;
        }
    }

    return 1;
}

static int verify_monster_roles(void)
{
    GameState game;
    init_game(&game);

    for (int i = 0; i < game.monster_count; ++i) {
        Monster *monster = &game.monsters[i];
        if (monster->is_boss) {
            if (monster->type != MONSTER_BOSS_BUTCHER || monster->hp != scale_monster_hp_for_difficulty(BOSS_HP, game.difficulty)) {
                fprintf(stderr, "error: boss monster role is not deterministic\n");
                return 0;
            }
            continue;
        }
        if (monster->hp != monster_max_hp(monster->type)) {
            fprintf(stderr, "error: monster %d hp does not match its role\n", i);
            return 0;
        }
    }

    if (!(monster_chase_speed(2) > monster_chase_speed(1) &&
          monster_chase_speed(1) > monster_chase_speed(3))) {
        fprintf(stderr, "error: monster role speeds are not ordered as expected\n");
        return 0;
    }
    if (!(monster_uses_projectile(1) &&
          monster_uses_projectile(MONSTER_FLYING_HEAD) &&
          !monster_uses_projectile(0) &&
          !monster_uses_projectile(2) &&
          !monster_uses_projectile(3) &&
          monster_projectile_damage(1) > 0 &&
          monster_projectile_damage(MONSTER_FLYING_HEAD) > monster_projectile_damage(1))) {
        fprintf(stderr, "error: monster attack roles are not deterministic\n");
        return 0;
    }

    return 1;
}

static int setup_interaction_camera(Camera *cam, int tx, int ty)
{
    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; ++i) {
        int px = tx + dirs[i][0];
        int py = ty + dirs[i][1];
        if (!generated_floor(px, py)) {
            continue;
        }
        cam->pos = (Vec2){px + 0.5, py + 0.5};
        cam->dir = (Vec2){-dirs[i][0], -dirs[i][1]};
        cam->plane = (Vec2){-cam->dir.y * 0.66, cam->dir.x * 0.66};
        return 1;
    }
    return 0;
}

static int verify_doors_and_secrets(void)
{
    GameState game;
    init_game(&game);
    Camera cam;
    int door_index = -1;

    for (int i = 0; i < MAX_DOORS; ++i) {
        if (game.doors[i].locked) {
            door_index = i;
            break;
        }
    }
    if (door_index < 0 || !setup_interaction_camera(&cam, game.doors[door_index].x, game.doors[door_index].y)) {
        fprintf(stderr, "error: generated level has no usable locked door\n");
        return 0;
    }

    if (map_at(game.doors[door_index].x, game.doors[door_index].y) == 0) {
        fprintf(stderr, "error: locked door does not block before interaction\n");
        return 0;
    }

    interact_world(&game, &cam);
    if (game.doors[door_index].opening || game.doors[door_index].open) {
        fprintf(stderr, "error: locked door opened without a key\n");
        return 0;
    }

    game.keys = 1;
    interact_world(&game, &cam);
    for (int i = 0; i < 12; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.doors[door_index].opening || game.doors[door_index].open ||
        game.doors[door_index].open_amount <= 0.0 || map_at(game.doors[door_index].x, game.doors[door_index].y) == 0) {
        fprintf(stderr, "error: keyed door has no visible opening phase\n");
        return 0;
    }
    for (int i = 0; i < 64; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.doors[door_index].open || map_at(game.doors[door_index].x, game.doors[door_index].y) != 0) {
        fprintf(stderr, "error: keyed door did not open cleanly\n");
        return 0;
    }

    int secret_index = -1;
    for (int i = 0; i < MAX_SECRETS; ++i) {
        if (setup_interaction_camera(&cam, game.secrets[i].x, game.secrets[i].y)) {
            secret_index = i;
            break;
        }
    }
    if (secret_index < 0) {
        fprintf(stderr, "error: generated level has no usable secret\n");
        return 0;
    }
    if (map_at(game.secrets[secret_index].x, game.secrets[secret_index].y) == 0) {
        fprintf(stderr, "error: secret does not block before interaction\n");
        return 0;
    }
    interact_world(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (!game.secrets[secret_index].open || map_at(game.secrets[secret_index].x, game.secrets[secret_index].y) != 0) {
        fprintf(stderr, "error: secret wall did not open cleanly\n");
        return 0;
    }

    return 1;
}

static int verify_generator_modes(void)
{
    for (int mode = 0; mode < GENERATOR_COUNT; ++mode) {
        GameState game;
        init_game_seed(&game, LEVEL_TEST_SEED + (uint32_t)mode * 97u, mode);

        double entrance_x = mode == GENERATOR_HOUSE ? 8.35 : 2.5;
        double entrance_y = mode == GENERATOR_HOUSE ? 12.50 : 22.5;
        double check_x = mode == GENERATOR_HOUSE ? 8.5 : 4.5;
        double check_y = mode == GENERATOR_HOUSE ? 12.5 : 22.5;
        if (game.generator_mode != mode || !can_move(entrance_x, entrance_y) || !can_occupy(check_x, check_y, 0.12)) {
            fprintf(stderr, "error: generator mode %d did not create a valid entrance\n", mode);
            return 0;
        }

        for (int i = 0; i < MAX_ITEMS; ++i) {
            if (game.items[i].active && !can_occupy(game.items[i].pos.x, game.items[i].pos.y, 0.12)) {
                fprintf(stderr, "error: generator mode %d placed item %d inside a wall\n", mode, i);
                return 0;
            }
        }
        for (int i = 0; i < game.monster_count; ++i) {
            if (game.monsters[i].active && !can_occupy(game.monsters[i].pos.x, game.monsters[i].pos.y, 0.28)) {
                fprintf(stderr, "error: generator mode %d placed monster %d inside a wall\n", mode, i);
                return 0;
            }
        }
        if (mode == GENERATOR_HOUSE) {
            int props = 0;
            int lootable = 0;
            for (int i = 0; i < MAX_PROPS; ++i) {
                if (!game.props[i].active) {
                    continue;
                }
                props++;
                if (!generated_floor((int)game.props[i].pos.x, (int)game.props[i].pos.y)) {
                    fprintf(stderr, "error: house generator placed prop %d outside floor\n", i);
                    return 0;
                }
                if (game.props[i].loot_slot >= 0) {
                    lootable++;
                }
            }
            if (game.monster_count != 0 || props < 7 || lootable < 4 || !game.portals[0].exit_to_forest) {
                fprintf(stderr, "error: house generator did not create a quiet lootable interior\n");
                return 0;
            }
        }

        if (mode == GENERATOR_TIGHT && !generated_floor(21, 2)) {
            fprintf(stderr, "error: tight generator did not create a far exit room\n");
            return 0;
        }
        if (mode == GENERATOR_FOREST && !generated_floor(20, 2)) {
            fprintf(stderr, "error: forest generator did not create a far clearing\n");
            return 0;
        }
        if (mode == GENERATOR_FOREST) {
            int portals = 0;
            int boss_gates = 0;
            int relic_bits = 0;
            int trees = 0;
            int houses = 0;
            for (int i = 0; i < MAX_PORTALS; ++i) {
                if (!game.portals[i].active || game.portals[i].exit_to_forest || !generated_floor(game.portals[i].x, game.portals[i].y)) {
                    continue;
                }
                if (game.portals[i].boss_gate) {
                    boss_gates++;
                } else {
                    if (game.portals[i].relic_index < 0 || game.portals[i].relic_index >= RELIC_COUNT) {
                        fprintf(stderr, "error: forest portal has invalid relic index\n");
                        return 0;
                    }
                    relic_bits |= 1 << game.portals[i].relic_index;
                    portals++;
                }
            }
            for (int i = 0; i < MAX_TREES; ++i) {
                if (!game.trees[i].active) {
                    continue;
                }
                trees++;
                if (!generated_floor((int)game.trees[i].pos.x, (int)game.trees[i].pos.y)) {
                    fprintf(stderr, "error: forest generator placed tree %d outside floor\n", i);
                    return 0;
                }
            }
            for (int i = 0; i < MAX_HOUSES; ++i) {
                if (!game.houses[i].active) {
                    continue;
                }
                houses++;
                if (!generated_floor((int)game.houses[i].pos.x, (int)game.houses[i].pos.y)) {
                    fprintf(stderr, "error: forest generator placed house %d outside floor\n", i);
                    return 0;
                }
                if (can_occupy(game.houses[i].pos.x, game.houses[i].pos.y, 0.12)) {
                    fprintf(stderr, "error: forest house %d does not block movement\n", i);
                    return 0;
                }
            }
            if (portals != RELIC_COUNT || relic_bits != RELIC_MASK_ALL || boss_gates != 1) {
                fprintf(stderr, "error: forest generator did not create relic entrances and boss gate\n");
                return 0;
            }
            if (trees < 24) {
                fprintf(stderr, "error: forest generator did not create enough tree billboards\n");
                return 0;
            }
            if (houses < 2) {
                fprintf(stderr, "error: forest generator did not create enough blocking houses\n");
                return 0;
            }
        }
        if (mode == GENERATOR_BOSS) {
            Monster *boss = &game.monsters[game.monster_count - 1];
            if (!boss->is_boss || boss->type != MONSTER_BOSS_BUTCHER || boss->hp != BOSS_HP ||
                boss->pos.x < 15.0 || boss->pos.x > 23.0 ||
                boss->pos.y < 1.0 || boss->pos.y > 8.0 ||
                !game.doors[0].locked || game.doors[0].x != 18 || game.doors[0].y != 8) {
                fprintf(stderr, "error: boss generator did not create boss chamber setup\n");
                return 0;
            }
        }
    }

    return 1;
}

static int verify_forest_dungeon_transition(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 404u, GENERATOR_FOREST);

    int portal_index = -1;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        if (game.portals[i].active && !game.portals[i].exit_to_forest && !game.portals[i].boss_gate) {
            portal_index = i;
            break;
        }
    }
    if (portal_index < 0 || !setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: forest dungeon entrance is not interactable\n");
        return 0;
    }

    Vec2 return_pos = cam.pos;
    game.pistol_unlocked = 1;
    game.selected_weapon = WEAPON_PISTOL;
    game.ammo = 37;
    game.shotgun_unlocked = 1;
    game.max_health_upgrades = 1;
    game.damage_upgrades = 1;
    game.ammo_cap_upgrades = 1;
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.generator_mode == GENERATOR_FOREST || !game.portals[0].exit_to_forest) {
        fprintf(stderr, "error: forest entrance did not create a dungeon with an exit\n");
        return 0;
    }
    if (!game.pistol_unlocked || !game.shotgun_unlocked || game.selected_weapon != WEAPON_PISTOL ||
        game.ammo != 37 || game.max_health_upgrades != 1 || game.damage_upgrades != 1 || game.ammo_cap_upgrades != 1) {
        fprintf(stderr, "error: dungeon entry did not preserve player weapon state\n");
        return 0;
    }
    int screen_x = 0;
    int sprite_h = 0;
    double depth = 0.0;
    Vec2 exit_pos = {game.portals[0].x + 0.5, game.portals[0].y + 0.5};
    if (!generated_floor(game.portals[0].x, game.portals[0].y) ||
        !project_sprite(&cam, exit_pos, 0.70, &screen_x, &sprite_h, &depth) ||
        screen_x < SCREEN_W / 4 || screen_x > SCREEN_W * 3 / 4) {
        fprintf(stderr, "error: dungeon exit is not visible from the crypt entrance\n");
        return 0;
    }

    game.ammo = 29;
    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: dungeon exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    double dx = cam.pos.x - return_pos.x;
    double dy = cam.pos.y - return_pos.y;
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || game.in_dungeon || dx * dx + dy * dy > 0.001) {
        fprintf(stderr, "error: dungeon exit did not restore the forest state and position\n");
        return 0;
    }
    if (!game.pistol_unlocked || !game.shotgun_unlocked || game.selected_weapon != WEAPON_PISTOL ||
        game.ammo != 29 || game.max_health_upgrades != 1 || game.damage_upgrades != 1 || game.ammo_cap_upgrades != 1) {
        fprintf(stderr, "error: dungeon exit did not preserve player weapon progress\n");
        return 0;
    }

    return 1;
}

static int setup_prop_interaction_camera(const GameState *game, Camera *cam, const Prop *prop)
{
    static const double dirs[4][2] = {{-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}};
    for (int i = 0; i < 4; ++i) {
        double half_w = prop_is_cylinder(prop) ? prop_footprint_radius(prop) : prop->half_w;
        double half_d = prop_is_cylinder(prop) ? prop_footprint_radius(prop) : prop->half_d;
        double px = prop->pos.x + dirs[i][0] * (half_w + 0.62);
        double py = prop->pos.y + dirs[i][1] * (half_d + 0.62);
        if (!generated_floor((int)px, (int)py) || !can_move(px, py)) {
            continue;
        }
        cam->pos = (Vec2){px, py};
        cam->dir = vec_norm((Vec2){prop->pos.x - px, prop->pos.y - py});
        cam->plane = (Vec2){-cam->dir.y * 0.66, cam->dir.x * 0.66};
        (void)game;
        return 1;
    }
    return 0;
}

static int verify_forest_house_transition(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 606u, GENERATOR_FOREST);

    int house_index = -1;
    for (int i = 0; i < MAX_HOUSES; ++i) {
        if (game.houses[i].active && !is_merchant_house_index(i)) {
            house_index = i;
            break;
        }
    }
    if (house_index < 0) {
        fprintf(stderr, "error: forest has no house to enter\n");
        return 0;
    }

    const House *house = &game.houses[house_index];
    cam = (Camera){
        .pos = {house_min_x(house) - 0.70, house->pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!active_house_prompt(&game, &cam, NULL)) {
        fprintf(stderr, "error: forest house entrance is not interactable\n");
        return 0;
    }

    Vec2 return_pos = cam.pos;
    game.gold = 0;
    interact_world(&game, &cam);
    if (!saved_forest.valid || game.generator_mode != GENERATOR_HOUSE || !game.in_dungeon || game.current_house_index != house_index || !game.portals[0].exit_to_forest) {
        fprintf(stderr, "error: house entrance did not create an interior level\n");
        return 0;
    }
    if (cam.dir.x <= 0.9 || fabs(cam.dir.y) > 0.001 ||
        map_at(game.portals[0].x, game.portals[0].y) != WALL_DOOR) {
        fprintf(stderr, "error: house entrance did not face into the interior with a valid exit door behind it\n");
        return 0;
    }

    int prop_index = -1;
    for (int i = 0; i < MAX_PROPS; ++i) {
        if (game.props[i].active && game.props[i].loot_slot >= 0 && !game.props[i].looted) {
            prop_index = i;
            break;
        }
    }
    if (prop_index < 0 || !setup_prop_interaction_camera(&game, &cam, &game.props[prop_index])) {
        fprintf(stderr, "error: house interior has no interactable loot prop\n");
        return 0;
    }
    int loot_slot = game.props[prop_index].loot_slot;
    int gold_before = game.gold;
    int ammo_before = game.ammo;
    int health_before = game.player_health;
    int fireball_before = game.fireball_unlocked;
    interact_world(&game, &cam);
    if (!game.props[prop_index].looted ||
        !(saved_forest.game.houses[house_index].loot_mask & (1u << loot_slot)) ||
        (game.gold == gold_before && game.ammo == ammo_before && game.player_health == health_before && game.fireball_unlocked == fireball_before && game.pickup_flash <= 0.0)) {
        fprintf(stderr, "error: house loot prop did not grant and persist loot\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: house exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    double dx = cam.pos.x - return_pos.x;
    double dy = cam.pos.y - return_pos.y;
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || game.in_dungeon || dx * dx + dy * dy > 0.001 ||
        !(game.houses[house_index].loot_mask & (1u << loot_slot))) {
        fprintf(stderr, "error: house exit did not restore forest with looted state\n");
        return 0;
    }

    cam = (Camera){
        .pos = {house_min_x(&game.houses[house_index]) - 0.70, game.houses[house_index].pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    interact_world(&game, &cam);
    if (!saved_forest.valid || game.generator_mode != GENERATOR_HOUSE) {
        fprintf(stderr, "error: house re-entry failed after looting\n");
        return 0;
    }
    int persisted = 0;
    for (int i = 0; i < MAX_PROPS; ++i) {
        if (game.props[i].active && game.props[i].loot_slot == loot_slot && game.props[i].looted) {
            persisted = 1;
            break;
        }
    }
    if (!persisted) {
        fprintf(stderr, "error: looted house prop reset after re-entry\n");
        return 0;
    }

    saved_forest.valid = 0;
    return 1;
}

static int verify_merchant_shop(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 616u, GENERATOR_FOREST);

    if (!game.houses[0].active) {
        fprintf(stderr, "error: forest merchant house was not generated\n");
        return 0;
    }

    const House *house = &game.houses[0];
    cam = (Camera){
        .pos = {house_min_x(house) - 0.70, house->pos.y},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!active_merchant_house_prompt(&game, &cam)) {
        fprintf(stderr, "error: merchant house is not interactable\n");
        return 0;
    }

    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST) {
        fprintf(stderr, "error: merchant house entered an interior instead of using the shop screen\n");
        return 0;
    }

    game.gold = SHOP_AMMO_PRICE + SHOP_HEALTH_PRICE;
    game.ammo = 80;
    game.player_health = 100;
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != SHOP_HEALTH_PRICE ||
        game.ammo != 98) {
        fprintf(stderr, "error: merchant ammo purchase failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_HEALTH) ||
        game.gold != 0 ||
        game.player_health != 145) {
        fprintf(stderr, "error: merchant health purchase failed\n");
        return 0;
    }
    if (buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != 0 ||
        game.ammo != 98) {
        fprintf(stderr, "error: merchant allowed a purchase without enough gold\n");
        return 0;
    }

    game.gold = 999;
    game.ammo = pistol_ammo_cap(&game);
    if (buy_merchant_shop_item(&game, SHOP_ITEM_AMMO) ||
        game.gold != 999 ||
        game.ammo != pistol_ammo_cap(&game)) {
        fprintf(stderr, "error: merchant sold ammo past the ammo cap\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_MAX_HP) ||
        game.max_health_upgrades != 1 ||
        player_max_health(&game) != PLAYER_MAX_HEALTH + HEALTH_UPGRADE_AMOUNT ||
        game.player_health != 165) {
        fprintf(stderr, "error: merchant max health upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_DAMAGE) ||
        game.damage_upgrades != 1 ||
        weapon_damage_bonus(&game) != 1) {
        fprintf(stderr, "error: merchant damage upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_AMMO_CAP) ||
        game.ammo_cap_upgrades != 1 ||
        pistol_ammo_cap(&game) != MAX_PISTOL_AMMO + AMMO_CAP_UPGRADE_AMOUNT ||
        game.ammo != pistol_ammo_cap(&game)) {
        fprintf(stderr, "error: merchant ammo cap upgrade failed\n");
        return 0;
    }
    if (!buy_merchant_shop_item(&game, SHOP_ITEM_SHOTGUN) ||
        !game.shotgun_unlocked ||
        game.selected_weapon != WEAPON_SHOTGUN) {
        fprintf(stderr, "error: merchant shotgun purchase failed\n");
        return 0;
    }
    if (buy_merchant_shop_item(&game, SHOP_ITEM_SHOTGUN)) {
        fprintf(stderr, "error: merchant sold shotgun twice\n");
        return 0;
    }
    return 1;
}

static int find_relic_item(const GameState *game, int relic_index)
{
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (game->items[i].active && game->items[i].type == ITEM_RELIC && game->items[i].relic_index == relic_index) {
            return i;
        }
    }
    return -1;
}

static int verify_generated_dungeon_relic_reachability(void)
{
    static const int modes[] = {GENERATOR_ROOMS, GENERATOR_TIGHT};
    for (int i = 0; i < (int)(sizeof(modes) / sizeof(modes[0])); ++i) {
        GameState game;
        init_game_seed(&game, LEVEL_TEST_SEED + 707u + (uint32_t)i * 101u, modes[i]);
        game.dungeon_relic_index = i;
        place_dungeon_exit_portal(&game);
        place_dungeon_relic(&game);

        int relic_item = find_relic_item(&game, i);
        if (relic_item < 0) {
            fprintf(stderr, "error: generated dungeon mode %d did not place a relic\n", modes[i]);
            return 0;
        }
        int rx = (int)game.items[relic_item].pos.x;
        int ry = (int)game.items[relic_item].pos.y;
        if (!dungeon_tile_reachable_from_entrance(&game, rx, ry)) {
            fprintf(stderr, "error: generated dungeon mode %d placed relic without entrance path\n", modes[i]);
            return 0;
        }
    }
    return 1;
}

static int verify_relic_story_progress(void)
{
    GameState game;
    Camera cam;
    saved_forest.valid = 0;
    init_game_seed(&game, LEVEL_TEST_SEED + 808u, GENERATOR_FOREST);

    int boss_gate = -1;
    int relic_portals = 0;
    int seen_relics = 0;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        Portal *portal = &game.portals[i];
        if (!portal->active || portal->exit_to_forest) {
            continue;
        }
        if (portal->boss_gate) {
            boss_gate = i;
            continue;
        }
        if (portal->relic_index < 0 || portal->relic_index >= RELIC_COUNT || (seen_relics & (1 << portal->relic_index))) {
            fprintf(stderr, "error: forest relic portals are not unique\n");
            return 0;
        }
        seen_relics |= 1 << portal->relic_index;
        relic_portals++;
    }
    if (relic_portals != RELIC_COUNT || seen_relics != RELIC_MASK_ALL || boss_gate < 0) {
        fprintf(stderr, "error: forest did not create 4 relic portals plus a boss gate\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[boss_gate].x, game.portals[boss_gate].y)) {
        fprintf(stderr, "error: boss gate is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST ||
        game.relic_flash <= 1.0 || game.relic_notice_count != 0) {
        fprintf(stderr, "error: locked boss gate allowed entry or gave no feedback\n");
        return 0;
    }

    int portal_index = -1;
    for (int i = 0; i < MAX_PORTALS; ++i) {
        if (game.portals[i].active && !game.portals[i].exit_to_forest && !game.portals[i].boss_gate) {
            portal_index = i;
            break;
        }
    }
    if (portal_index < 0 || !setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: relic dungeon entrance is not interactable\n");
        return 0;
    }
    int relic_index = game.portals[portal_index].relic_index;
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.dungeon_relic_index != relic_index) {
        fprintf(stderr, "error: relic dungeon did not preserve relic index\n");
        return 0;
    }
    if (active_music_track != music_track_for_relic(relic_index)) {
        fprintf(stderr, "error: relic dungeon did not switch to its music track\n");
        return 0;
    }

    int relic_item = find_relic_item(&game, relic_index);
    if (relic_item < 0) {
        fprintf(stderr, "error: relic dungeon did not spawn its relic\n");
        return 0;
    }
    if (!dungeon_tile_reachable_from_entrance(&game, (int)game.items[relic_item].pos.x, (int)game.items[relic_item].pos.y)) {
        fprintf(stderr, "error: relic dungeon placed relic without entrance path\n");
        return 0;
    }
    int relic_guardian = 0;
    for (int i = 0; i < game.monster_count; ++i) {
        Monster *monster = &game.monsters[i];
        if (!monster->active || monster->type != MONSTER_GIANT_SKELETON) {
            continue;
        }
        Vec2 diff = {
            monster->pos.x - game.items[relic_item].pos.x,
            monster->pos.y - game.items[relic_item].pos.y,
        };
        if (vec_len(diff) <= 4.5) {
            relic_guardian = 1;
            break;
        }
    }
    if (!relic_guardian) {
        fprintf(stderr, "error: relic dungeon did not spawn a nearby giant skeleton guardian\n");
        return 0;
    }
    int screen_x = 0;
    int sprite_h = 0;
    double depth = 0.0;
    Vec2 exit_pos = {game.portals[0].x + 0.5, game.portals[0].y + 0.5};
    if (!generated_floor(game.portals[0].x, game.portals[0].y) ||
        !project_sprite(&cam, exit_pos, 0.70, &screen_x, &sprite_h, &depth) ||
        screen_x < SCREEN_W / 4 || screen_x > SCREEN_W * 3 / 4) {
        fprintf(stderr, "error: relic dungeon exit is not visible from the crypt entrance\n");
        return 0;
    }
    cam.pos = game.items[relic_item].pos;
    story.solved_mask &= ~(1u << relic_index);
    story.ritual_steps[relic_index] = 0;
    update_items(&game, &cam);
    if (!game.items[relic_item].active || (game.relic_mask & (1u << relic_index))) {
        fprintf(stderr, "error: relic can be taken before solving its seal\n");
        return 0;
    }
    for (int step = 0; step < 3; ++step) {
        int stone = -1;
        for (int i = 0; i < MAX_ITEMS; ++i) {
            if (game.items[i].active && game.items[i].type == ITEM_SEAL && game.items[i].relic_index == seal_orders[relic_index][step]) stone = i;
        }
        if (stone < 0 || !invoke_story_seal(&game, &cam, stone)) {
            fprintf(stderr, "error: relic seal cannot be solved\n");
            return 0;
        }
    }
    update_items(&game, &cam);
    if (!(game.relic_mask & (1 << relic_index)) ||
        !(saved_forest.game.relic_mask & (1 << relic_index)) ||
        game.relic_count != 1 ||
        game.boss_unlocked ||
        game.relic_flash <= 1.0 ||
        game.relic_notice_count != 1) {
        fprintf(stderr, "error: relic pickup did not persist progress correctly\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: relic dungeon exit is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (saved_forest.valid || game.generator_mode != GENERATOR_FOREST || !(game.relic_mask & (1 << relic_index))) {
        fprintf(stderr, "error: returning to forest lost relic progress\n");
        return 0;
    }
    if (active_music_track != MUSIC_TRACK_FOREST) {
        fprintf(stderr, "error: returning to forest did not restore forest music\n");
        return 0;
    }

    if (!setup_interaction_camera(&cam, game.portals[portal_index].x, game.portals[portal_index].y)) {
        fprintf(stderr, "error: relic dungeon re-entry is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (find_relic_item(&game, relic_index) >= 0) {
        fprintf(stderr, "error: collected relic spawned again on re-entry\n");
        return 0;
    }
    if (!setup_interaction_camera(&cam, game.portals[0].x, game.portals[0].y)) {
        fprintf(stderr, "error: relic dungeon exit after re-entry is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);

    game.relic_mask = RELIC_MASK_ALL;
    sync_relic_progress(&game);
    if (!setup_interaction_camera(&cam, game.portals[boss_gate].x, game.portals[boss_gate].y)) {
        fprintf(stderr, "error: unlocked boss gate is not interactable\n");
        return 0;
    }
    interact_world(&game, &cam);
    if (!saved_forest.valid || !game.in_dungeon || game.generator_mode != GENERATOR_BOSS || game.dungeon_relic_index != -1) {
        fprintf(stderr, "error: unlocked boss gate did not enter boss level\n");
        return 0;
    }
    if (!(story.notes_mask & (1u << STORY_ENTRY_GATE))) {
        fprintf(stderr, "error: boss gate did not reveal the gate journal entry\n");
        return 0;
    }
    if (active_music_track != MUSIC_TRACK_TOCCATA) {
        fprintf(stderr, "error: boss gate did not switch to boss music\n");
        return 0;
    }

    GameState normal;
    init_game(&normal);
    normal.monster_count = 1;
    memset(normal.monsters, 0, sizeof(normal.monsters));
    normal.monsters[0] = (Monster){.active = 1, .hp = 1, .pos = {5.5, 22.5}, .type = 1, .patrol_count = 1};
    damage_monster(&normal, &normal.monsters[0], 1, (Vec2){2.5, 22.5});
    if (normal.victory) {
        fprintf(stderr, "error: normal dungeon victory triggered without boss\n");
        return 0;
    }

    GameState boss_game;
    init_game_seed(&boss_game, LEVEL_TEST_SEED + 909u, GENERATOR_BOSS);
    Monster *boss = &boss_game.monsters[boss_game.monster_count - 1];
    boss->hp = 1;
    damage_monster(&boss_game, boss, 1, (Vec2){2.5, 22.5});
    if (!boss_game.victory) {
        fprintf(stderr, "error: boss death did not trigger victory\n");
        return 0;
    }
    int active_explosion = -1;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (boss_game.projectiles[i].active && boss_game.projectiles[i].type == PROJECTILE_EXPLOSION) {
            active_explosion = i;
            break;
        }
    }
    if (active_explosion < 0) {
        fprintf(stderr, "error: boss death did not spawn a victory explosion\n");
        return 0;
    }
    double explosion_life = boss_game.projectiles[active_explosion].life;
    Camera boss_cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    update_game(&boss_game, &boss_cam, 1.0 / 60.0);
    if (!boss_game.victory || boss_game.game_over || boss_game.projectiles[active_explosion].life >= explosion_life) {
        fprintf(stderr, "error: victory state did not keep post-boss effects responsive\n");
        return 0;
    }

    return 1;
}

static int verify_music_track_mapping(void)
{
    int seen = 0;
    for (int relic = 0; relic < RELIC_COUNT; ++relic) {
        int track = music_track_for_relic(relic);
        if (track < 0 || track >= MUSIC_TRACK_COUNT || (seen & (1 << track))) {
            fprintf(stderr, "error: relic music track mapping is not unique\n");
            return 0;
        }
        if (!midi_tracks[track].events || midi_tracks[track].event_count <= 0) {
            fprintf(stderr, "error: relic music track %d is not loaded\n", track);
            return 0;
        }
        seen |= 1 << track;
        set_active_music_track(track);
        if (active_music_track != track || midi_tracks[track].next_event != 0 || midi_tracks[track].playhead != 0.0) {
            fprintf(stderr, "error: relic music track %d did not reset cleanly\n", track);
            return 0;
        }
    }
    set_active_music_track(MUSIC_TRACK_FOREST);
    if (active_music_track != MUSIC_TRACK_FOREST) {
        fprintf(stderr, "error: forest music track did not activate\n");
        return 0;
    }
    return 1;
}

static int verify_fm_instrument_distribution(void)
{
    int mask = 0;
    static const struct {
        int track;
        int channel;
        int note;
    } checks[] = {
        {MUSIC_TRACK_DIES_IRAE, 0, 36},
        {MUSIC_TRACK_DIES_IRAE, 1, 60},
        {MUSIC_TRACK_MASONIC_FUNERAL, 2, 62},
        {MUSIC_TRACK_PATHETIQUE, 0, 52},
        {MUSIC_TRACK_TOCCATA, 0, 67},
        {MUSIC_TRACK_TOCCATA, 1, 79},
    };
    for (int i = 0; i < (int)(sizeof(checks) / sizeof(checks[0])); ++i) {
        int instrument = fm_instrument_for_note(checks[i].track, checks[i].channel, checks[i].note);
        if (instrument < 0 || instrument >= FM_INST_COUNT) {
            fprintf(stderr, "error: FM instrument selection returned invalid preset\n");
            return 0;
        }
        mask |= 1 << instrument;
    }
    int used = 0;
    for (int i = 0; i < FM_INST_COUNT; ++i) {
        if (mask & (1 << i)) {
            used++;
        }
    }
    if ((mask & (1 << FM_INST_EPIANO)) == 0 || used < 4) {
        fprintf(stderr, "error: FM instrument distribution is too narrow\n");
        return 0;
    }

    set_active_music_track(MUSIC_TRACK_PATHETIQUE);
    memset(midi_voices, 0, sizeof(midi_voices));
    fm_midi_note_on(40, 96, 0);
    fm_midi_note_on(60, 100, 1);
    fm_midi_note_on(79, 84, 2);
    double peak = 0.0;
    for (int i = 0; i < 512; ++i) {
        double sample = fm_midi_voices_sample();
        if (!isfinite(sample)) {
            fprintf(stderr, "error: FM instrument sample became non-finite\n");
            return 0;
        }
        if (fabs(sample) > peak) {
            peak = fabs(sample);
        }
    }
    if (peak <= 0.0001 || peak > 8.0) {
        fprintf(stderr, "error: FM instrument sample peak is invalid\n");
        return 0;
    }
    set_active_music_track(MUSIC_TRACK_FOREST);
    return 1;
}

static int verify_render_effect_presets(void)
{
    static const struct {
        const char *text;
        int expected;
    } checks[] = {
        {"off", RENDER_EFFECTS_OFF},
        {"full", RENDER_EFFECTS_PRESET1},
        {"1", RENDER_EFFECTS_PRESET1},
        {"preset2", RENDER_EFFECTS_PRESET2},
        {"lut3", RENDER_EFFECTS_PRESET3},
    };

    for (int i = 0; i < (int)(sizeof(checks) / sizeof(checks[0])); ++i) {
        int effects = -1;
        if (!parse_render_effects(checks[i].text, &effects) || effects != checks[i].expected) {
            fprintf(stderr, "error: render effects preset parser rejected %s\n", checks[i].text);
            return 0;
        }
    }
    if (strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET1), "preset1") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET2), "preset2") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_PRESET3), "preset3") != 0 ||
        strcmp(render_effects_config_text(RENDER_EFFECTS_OFF), "off") != 0) {
        fprintf(stderr, "error: render effects preset config text is invalid\n");
        return 0;
    }
    if (normalize_render_effects(-1) != RENDER_EFFECTS_OFF ||
        normalize_render_effects(RENDER_EFFECTS_COUNT) != RENDER_EFFECTS_OFF) {
        fprintf(stderr, "error: render effects normalization is invalid\n");
        return 0;
    }
    return 1;
}

static int verify_help_key_close(void)
{
    GameState game;
    init_game(&game);
    if (game.help_timer <= 0.0 || game.show_help) {
        fprintf(stderr, "error: startup help state is invalid\n");
        return 0;
    }
    if (!close_help_on_key(&game) || game.help_timer > 0.0 || game.show_help) {
        fprintf(stderr, "error: key press did not close startup help\n");
        return 0;
    }
    if (close_help_on_key(&game)) {
        fprintf(stderr, "error: hidden help reported as closed\n");
        return 0;
    }

    game.show_help = 1;
    if (!close_help_on_key(&game) || game.show_help || game.help_timer > 0.0) {
        fprintf(stderr, "error: key press did not close toggled help\n");
        return 0;
    }
    return 1;
}

static int verify_prompt_font_glyphs(void)
{
    const char *required = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./-:%!";
    for (const char *c = required; *c; ++c) {
        int has_pixels = 0;
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = prompt_font_glyph(*c, row);
            if (bits & ~31u) {
                fprintf(stderr, "error: prompt font glyph %c row %d exceeds 5 pixels\n", *c, row);
                return 0;
            }
            if (bits) {
                has_pixels = 1;
            }
        }
        if (!has_pixels) {
            fprintf(stderr, "error: prompt font glyph %c is missing\n", *c);
            return 0;
        }
    }
    return 1;
}

static int verify_difficulty_scaling(void)
{
    int easy_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_EASY);
    int normal_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_NORMAL);
    int hard_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_HARD);
    int nightmare_hp = scale_monster_hp_for_difficulty(100, DIFFICULTY_NIGHTMARE);
    if (!(easy_hp < normal_hp && normal_hp < hard_hp && hard_hp < nightmare_hp)) {
        fprintf(stderr, "error: difficulty HP scaling is not ordered\n");
        return 0;
    }

    GameState game;
    memset(&game, 0, sizeof(game));
    game.difficulty = DIFFICULTY_EASY;
    int easy_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_NORMAL;
    int normal_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_HARD;
    int hard_damage = scale_enemy_damage_for_difficulty(&game, 20);
    game.difficulty = DIFFICULTY_NIGHTMARE;
    int nightmare_damage = scale_enemy_damage_for_difficulty(&game, 20);
    if (!(easy_damage < normal_damage && normal_damage < hard_damage && hard_damage < nightmare_damage)) {
        fprintf(stderr, "error: difficulty damage scaling is not ordered\n");
        return 0;
    }
    return 1;
}

static int verify_boss_sprite_asset(void)
{
    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        for (int anim = 0; anim < BOSS_ANIM_FRAMES; ++anim) {
            int visible = 0;
            for (int i = 0; i < BOSS_SPRITE_SIZE * BOSS_SPRITE_SIZE; ++i) {
                if (!is_sprite_key(boss_sprites[frame][anim][i])) {
                    visible++;
                }
            }
            if (visible < 900) {
                fprintf(stderr, "error: boss sprite frame %d anim %d has too few visible pixels\n", frame, anim);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_furniture_sprite_asset(void)
{
    for (int sprite = 0; sprite < FURNITURE_SPRITE_COUNT; ++sprite) {
        int visible = 0;
        for (int i = 0; i < FURNITURE_SIZE * FURNITURE_SIZE; ++i) {
            if (!is_sprite_key(furniture_sprites[sprite][i])) {
                visible++;
            }
        }
        if (visible < 90) {
            fprintf(stderr, "error: furniture sprite %d has too few visible pixels\n", sprite);
            return 0;
        }
    }
    return 1;
}

static int verify_monster_sprite_assets(void)
{
    for (int type = 0; type < MONSTER_TYPES; ++type) {
        for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
            for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
                int visible = 0;
                for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; ++i) {
                    if (!is_sprite_key(monster_sprites[type][frame][anim][i])) {
                        visible++;
                    }
                }
                if (visible < 120) {
                    fprintf(stderr, "error: monster sprite type %d frame %d anim %d has too few visible pixels\n", type, frame, anim);
                    return 0;
                }
            }
        }
    }

    for (int frame = 0; frame < SPRITE_FRAMES; ++frame) {
        for (int anim = 0; anim < MONSTER_ANIM_FRAMES; ++anim) {
            int visible = 0;
            for (int i = 0; i < GIANT_SKELETON_SPRITE_SIZE * GIANT_SKELETON_SPRITE_SIZE; ++i) {
                if (!is_sprite_key(giant_skeleton_sprites[frame][anim][i])) {
                    visible++;
                }
            }
            if (visible < 900) {
                fprintf(stderr, "error: giant skeleton sprite frame %d anim %d has too few visible pixels\n", frame, anim);
                return 0;
            }
        }
    }
    return 1;
}

static int verify_trainer_mode(void)
{
    int previous_trainer = runtime_trainer;
    runtime_trainer = 1;
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED + 1001u, GENERATOR_FOREST);
    runtime_trainer = previous_trainer;

    if (!game.trainer || game.relic_mask != RELIC_MASK_ALL || game.relic_count != RELIC_COUNT || !game.boss_unlocked) {
        fprintf(stderr, "error: trainer mode did not start with all relics collected\n");
        return 0;
    }

    game.player_health = 23;
    apply_player_damage(&game, 999);
    if (game.player_health != 23 || game.game_over) {
        fprintf(stderr, "error: trainer mode allowed player damage\n");
        return 0;
    }

    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    game.pistol_unlocked = 1;
    game.selected_weapon = WEAPON_PISTOL;
    game.ammo = 0;
    game.shot_cooldown = 0.0;
    player_fire(&game, &cam);
    if (game.ammo != 0 || game.shot_cooldown <= 0.0) {
        fprintf(stderr, "error: trainer mode did not allow pistol fire with empty ammo\n");
        return 0;
    }

    game.fireball_unlocked = 1;
    game.selected_weapon = WEAPON_FIREBALL;
    game.fireball_ammo = 0;
    game.shot_cooldown = 0.0;
    memset(game.projectiles, 0, sizeof(game.projectiles));
    player_fire(&game, &cam);
    int fireball_spawned = 0;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (game.projectiles[i].active && game.projectiles[i].owner == PROJECTILE_OWNER_PLAYER) {
            fireball_spawned = 1;
            break;
        }
    }
    if (game.fireball_ammo != 0 || game.shot_cooldown <= 0.0 || !fireball_spawned) {
        fprintf(stderr, "error: trainer mode did not allow fireball fire with empty ammo\n");
        return 0;
    }

    return 1;
}

static int verify_sprite_sort_order(void)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    SpriteDraw draws[2] = {
        {1, 0, 0.0},
        {0, 5, 0.0},
    };
    Vec2 item_pos = {4.5, 22.5};
    Vec2 monster_pos = {7.5, 22.5};

    double item_dx = item_pos.x - cam.pos.x;
    double item_dy = item_pos.y - cam.pos.y;
    double monster_dx = monster_pos.x - cam.pos.x;
    double monster_dy = monster_pos.y - cam.pos.y;
    draws[0].dist = item_dx * item_dx + item_dy * item_dy;
    draws[1].dist = monster_dx * monster_dx + monster_dy * monster_dy;

    if (draws[0].dist < draws[1].dist) {
        SpriteDraw tmp = draws[0];
        draws[0] = draws[1];
        draws[1] = tmp;
    }

    if (draws[0].kind != 0 || draws[1].kind != 1) {
        fprintf(stderr, "error: sprite sort order is not far-to-near across item/monster types\n");
        return 0;
    }
    return 1;
}

static int verify_torches(void)
{
    GameState game;
    init_game(&game);

    for (int i = 0; i < MAX_TORCHES; ++i) {
        Vec2 pos = torches[i].pos;
        int cx = (int)pos.x;
        int cy = (int)pos.y;
        int near_wall =
            map_at(cx - 1, cy) > 0 ||
            map_at(cx + 1, cy) > 0 ||
            map_at(cx, cy - 1) > 0 ||
            map_at(cx, cy + 1) > 0;

        if (!can_occupy(pos.x, pos.y, 0.04)) {
            fprintf(stderr, "error: torch %d is placed inside a wall at %.2f %.2f\n", i, pos.x, pos.y);
            return 0;
        }
        if (!near_wall) {
            fprintf(stderr, "error: torch %d is not mounted next to a wall at %.2f %.2f\n", i, pos.x, pos.y);
            return 0;
        }
    }

    return 1;
}

static int verify_volumetric_fog(void)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.time = 1.0;
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };

    double near_fog = fog_amount(1.0);
    double far_fog = fog_amount(12.0);
    if (far_fog <= near_fog) {
        fprintf(stderr, "error: distance fog does not increase with depth\n");
        return 0;
    }

    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        framebuffer[i] = rgb(90, 72, 56);
        depth_buffer[i] = 12.0;
    }
    for (int x = 0; x < SCREEN_W; ++x) {
        z_buffer[x] = 12.0;
    }

    prepare_world_light_grid(game.time);
    render_volumetric_fog(&cam, &game);

    int changed = 0;
    for (int i = 0; i < SCREEN_W * (SCREEN_H - HUD_HEIGHT); ++i) {
        if (framebuffer[i] != rgb(90, 72, 56)) {
            changed++;
        }
    }

    if (changed == 0) {
        fprintf(stderr, "error: volumetric fog pass did not modify the framebuffer\n");
        return 0;
    }
    return 1;
}

static int verify_dynamic_shadows(void)
{
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED, GENERATOR_ROOMS);
    Camera cam = {.pos = {2.5, 22.5}, .dir = {1.0, 0.0}, .plane = {0.0, 0.66}};
    game.monster_count = 1;
    game.monsters[0] = (Monster){.active = 1, .pos = {4.5, 22.5}};
    memset(game.items, 0, sizeof(game.items));
    uint32_t background = rgb(128, 128, 128);
    for (int occluded = 0; occluded < 2; ++occluded) {
        for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) framebuffer[i] = background;
        for (int x = 0; x < SCREEN_W; ++x) z_buffer[x] = occluded ? 0.1 : 80.0;
        render_dynamic_shadows(&cam, &game);
        int changed = 0;
        for (int i = 0; i < SCREEN_W * (SCREEN_H - HUD_HEIGHT); ++i) changed += framebuffer[i] != background;
        if ((!occluded && changed == 0) || (occluded && changed != 0)) {
            fprintf(stderr, "error: dynamic shadow missing or visible through a wall\n");
            return 0;
        }
    }
    return 1;
}

static int verify_tree_visible_at_collision_range(void)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.generator_mode = GENERATOR_FOREST;
    game.time = 1.0;
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    Tree tree = {
        .active = 1,
        .variant = 0,
        .pos = {2.5 + TREE_COLLISION_RADIUS + 0.04, 22.5},
    };
    uint32_t background = rgb(8, 7, 6);

    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        framebuffer[i] = background;
        depth_buffer[i] = 80.0;
    }
    for (int x = 0; x < SCREEN_W; ++x) {
        z_buffer[x] = 80.0;
    }

    render_tree(&cam, &game, &tree);

    int changed = 0;
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        if (framebuffer[i] != background) {
            changed++;
        }
    }
    if (changed == 0) {
        fprintf(stderr, "error: tree billboard disappeared before collision boundary\n");
        return 0;
    }
    return 1;
}

static int write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return 1;
    }

    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        uint32_t p = framebuffer[i];
        fputc((int)((p >> 16) & 0xFFu), f);
        fputc((int)((p >> 8) & 0xFFu), f);
        fputc((int)(p & 0xFFu), f);
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "cannot close %s: %s\n", path, strerror(errno));
        return 1;
    }
    return 0;
}

static int verify_story_puzzles(void)
{
    StoryProgress previous = story;
    for (int relic = 0; relic < RELIC_COUNT; ++relic) {
        for (int seed = 0; seed < 16; ++seed) {
            GameState game;
            memset(&story, 0, sizeof(story));
            init_game_seed(&game, LEVEL_TEST_SEED + 1901u + (uint32_t)seed * 31u + (uint32_t)relic,
                           (seed & 1) ? GENERATOR_ROOMS : GENERATOR_TIGHT);
            game.trainer = 0;
            game.dungeon_relic_index = relic;
            place_dungeon_exit_portal(&game);
            place_dungeon_relic(&game);
            if (!place_story_seals(&game)) return 0;
            int stones[3] = {-1, -1, -1};
            for (int i = 0; i < MAX_ITEMS; ++i) {
                const Item *item = &game.items[i];
                if (!item->active || item->type != ITEM_SEAL) continue;
                if (item->relic_index < 0 || item->relic_index >= 3 || stones[item->relic_index] != -1 ||
                    !dungeon_tile_reachable_from_entrance(&game, (int)item->pos.x, (int)item->pos.y) ||
                    !can_occupy(item->pos.x, item->pos.y, 0.18)) {
                    fprintf(stderr, "error: ritual stone is duplicated or unreachable\n");
                    return 0;
                }
                stones[item->relic_index] = i;
            }
            for (int i = 0; i < 3; ++i) {
                if (stones[i] < 0) { fprintf(stderr, "error: crypt is missing a ritual symbol\n"); return 0; }
            }
            int relic_item = find_relic_item(&game, relic);
            if (relic_item < 0) { fprintf(stderr, "error: crypt is missing its relic\n"); return 0; }
            Camera cam = {.pos = game.items[relic_item].pos, .dir = {1.0, 0.0}, .plane = {0.0, 0.66}};
            update_items(&game, &cam);
            if (!game.items[relic_item].active || game.relic_mask) {
                fprintf(stderr, "error: crypt relic bypasses its puzzle\n"); return 0;
            }
            if (!invoke_story_seal(&game, &cam, stones[seal_orders[relic][0]]) ||
                invoke_story_seal(&game, &cam, stones[seal_orders[relic][0]]) ||
                story.ritual_steps[relic] != 0 || story.failures[relic] != 1) {
                fprintf(stderr, "error: repeated wrong ritual symbol does not reset the puzzle\n"); return 0;
            }
            for (int step = 0; step < 3; ++step) {
                if (!invoke_story_seal(&game, &cam, stones[seal_orders[relic][step]])) {
                    fprintf(stderr, "error: correct ritual sequence is rejected\n"); return 0;
                }
            }
            update_items(&game, &cam);
            if (game.items[relic_item].active || !(game.relic_mask & (1u << relic)) ||
                !(story.notes_mask & (1u << 9))) {
                fprintf(stderr, "error: solved ritual does not unlock relic and story\n"); return 0;
            }
            StoryProgress completed = story;
            invoke_story_seal(&game, &cam, stones[0]);
            if (memcmp(&completed, &story, sizeof(story))) {
                fprintf(stderr, "error: completed ritual can be activated twice\n"); return 0;
            }
        }
    }
    /* Re-reading an old looted container unlocks the note, but never gold. */
    GameState house = {0};
    house.generator_mode = GENERATOR_HOUSE;
    house.current_house_variant = 2;
    Prop prop = {.active = 1, .loot_slot = 4, .looted = 1, .loot_type = ITEM_GOLD, .loot_amount = 30};
    memset(&story, 0, sizeof(story));
    loot_prop(&house, &prop);
    if (house.gold || !(story.notes_mask & (1u << 8))) {
        fprintf(stderr, "error: old container cannot expose its note without duplicating loot\n"); return 0;
    }
    story = previous;
    story_popup = -1;
    story_notice_time = 0.0;
    story_dread = 0.0;
    return 1;
}

static int verify_boss_phases(void)
{
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED + 909u, GENERATOR_BOSS);
    Monster *boss = NULL;
    for (int i = 0; i < game.monster_count; ++i) {
        if (game.monsters[i].is_boss) boss = &game.monsters[i];
        else game.monsters[i].active = 0;
    }
    if (!boss || !boss->active) { fprintf(stderr, "error: boss level has no boss\n"); return 0; }
    int max_hp = adaptive_monster_hp(&game, BOSS_HP);
    boss->hp = max_hp;
    if (boss_phase(&game, boss) != 0 || engaged_boss(&game)) {
        fprintf(stderr, "error: untouched boss is not in its first phase\n"); return 0;
    }
    Vec2 player = {boss->pos.x + 3.0, boss->pos.y};
    story_notice = NULL;
    story_notice_time = 0.0;
    damage_monster(&game, boss, max_hp / 3 + 1, player);
    int awake = 0;
    for (int i = 0; i < game.monster_count; ++i) {
        const Monster *m = &game.monsters[i];
        if (!m->active || m->is_boss) continue;
        double dx = m->pos.x - player.x, dy = m->pos.y - player.y;
        if (dx * dx + dy * dy < 4.0 || m->ai_state != 1 || !can_occupy(m->pos.x, m->pos.y, 0.28)) {
            fprintf(stderr, "error: boss minion spawned on the player or inside a wall\n"); return 0;
        }
        awake++;
    }
    if (boss_phase(&game, boss) != 1 || awake != 2 || !engaged_boss(&game) || story_notice_time <= 0.0 || game.victory) {
        fprintf(stderr, "error: boss second phase did not wake two bodies (phase %d, awake %d)\n", boss_phase(&game, boss), awake);
        return 0;
    }
    damage_monster(&game, boss, max_hp / 3 + 1, player);
    int heads = 0;
    for (int i = 0; i < game.monster_count; ++i) {
        if (game.monsters[i].active && !game.monsters[i].is_boss && game.monsters[i].type == MONSTER_FLYING_HEAD) heads++;
    }
    if (boss_phase(&game, boss) != 2 || heads != 1 || game.victory) {
        fprintf(stderr, "error: boss third phase did not release a voice\n"); return 0;
    }
    int count_before = 0;
    for (int i = 0; i < game.monster_count; ++i) count_before += game.monsters[i].active;
    damage_monster(&game, boss, 1, player);
    int count_after = 0;
    for (int i = 0; i < game.monster_count; ++i) count_after += game.monsters[i].active;
    if (count_after != count_before || boss_phase(&game, boss) != 2) {
        fprintf(stderr, "error: boss phase woke bodies twice\n"); return 0;
    }
    GameState forest;
    init_game_seed(&forest, LEVEL_TEST_SEED + 910u, GENERATOR_FOREST);
    if (engaged_boss(&forest)) { fprintf(stderr, "error: forest reports a boss bar\n"); return 0; }
    story_notice = NULL;
    story_notice_time = 0.0;
    story_dread = 0.0;
    return 1;
}

static int simulate_adaptive_window(GameState *game, int shots, int hits, int kills, int damage, int health)
{
    game->player_health = health;
    for (int i = 0; i < shots; ++i) adaptive_note_shot();
    for (int i = 0; i < hits; ++i) adaptive_note_hit();
    for (int i = 0; i < kills; ++i) adaptive_note_kill();
    adaptive_note_damage_taken(damage);
    int before = adaptive.evaluations;
    for (int i = 0; i < 25 * 60 && adaptive.evaluations == before && adaptive.window_time < ADAPTIVE_WINDOW_SECONDS + 1.0; ++i) {
        adaptive_update(game, 1.0 / 60.0);
    }
    return adaptive.evaluations != before;
}

static int verify_adaptive_difficulty(void)
{
    adaptive_reset();
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED + 1200u, GENERATOR_ROOMS);
    if (adaptive.skill != 0.0 || fabs(adaptive_hp_scale() - 1.0) > 1e-9 || fabs(adaptive_damage_scale() - 1.0) > 1e-9 ||
        fabs(adaptive_cooldown_scale() - 1.0) > 1e-9 || adaptive_extra_spawns() != 0 ||
        adaptive_monster_hp(&game, BOSS_HP) != scale_monster_hp_for_difficulty(BOSS_HP, game.difficulty)) {
        fprintf(stderr, "error: neutral adaptive difficulty changes the chosen level\n"); return 0;
    }
    /* Exploring without fighting must not move the estimate. */
    game.player_health = player_max_health(&game);
    for (int i = 0; i < 3 * 60 * 60; ++i) adaptive_update(&game, 1.0 / 60.0);
    if (adaptive.skill != 0.0 || adaptive.evaluations != 0) {
        fprintf(stderr, "error: idle exploration drifts the adaptive skill\n"); return 0;
    }
    /* A dominating player: accurate, killing, untouched. */
    for (int i = 0; i < 6; ++i) {
        if (!simulate_adaptive_window(&game, 12, 11, 3, 0, player_max_health(&game))) {
            fprintf(stderr, "error: adaptive window with combat was not evaluated\n"); return 0;
        }
    }
    double strong = adaptive.skill;
    if (strong < 0.5 || adaptive_hp_scale() < 1.15 || adaptive_damage_scale() < 1.15 || adaptive_cooldown_scale() > 0.9 ||
        adaptive_extra_spawns() != 1 || adaptive_enemy_damage(20) <= 20) {
        fprintf(stderr, "error: dominating player did not raise difficulty (skill %.2f)\n", adaptive.skill); return 0;
    }
    for (int i = 0; i < 40; ++i) simulate_adaptive_window(&game, 12, 12, 4, 0, player_max_health(&game));
    if (adaptive.skill > 1.0 || adaptive_hp_scale() > 1.0 + ADAPTIVE_HP_RANGE + 1e-9) {
        fprintf(stderr, "error: adaptive skill escaped its bounds\n"); return 0;
    }
    /* The level snapshot keeps boss phases stable while the estimate moves. */
    adaptive.skill = 0.8;
    GameState boss_game;
    init_game_seed(&boss_game, LEVEL_TEST_SEED + 909u, GENERATOR_BOSS);
    Monster *boss = &boss_game.monsters[boss_game.monster_count - 1];
    if (!boss->is_boss || boss->hp != adaptive_monster_hp(&boss_game, BOSS_HP) ||
        boss->hp <= scale_monster_hp_for_difficulty(BOSS_HP, boss_game.difficulty) || boss_phase(&boss_game, boss) != 0) {
        fprintf(stderr, "error: boss HP ignores the adaptive level snapshot\n"); return 0;
    }
    adaptive.skill = -1.0;
    if (boss_phase(&boss_game, boss) != 0 || boss->hp != adaptive_monster_hp(&boss_game, BOSS_HP)) {
        fprintf(stderr, "error: boss phase shifted without a new level\n"); return 0;
    }
    /* A struggling player: hit often, dying, missing. */
    adaptive_reset();
    init_game_seed(&game, LEVEL_TEST_SEED + 1201u, GENERATOR_ROOMS);
    for (int i = 0; i < 4; ++i) simulate_adaptive_window(&game, 10, 2, 0, player_max_health(&game) / 2, 30);
    double hurt = adaptive.skill;
    adaptive_note_death();
    if (hurt >= 0.0 || adaptive.skill >= hurt || adaptive.deaths != 1 || adaptive.skill < -1.0 ||
        adaptive_hp_scale() > 0.85 || adaptive_damage_scale() > 0.85 || adaptive_cooldown_scale() < 1.1 ||
        adaptive_extra_spawns() != -1 || adaptive_enemy_damage(20) >= 20) {
        fprintf(stderr, "error: struggling player did not lower difficulty (skill %.2f)\n", adaptive.skill); return 0;
    }
    for (int i = 0; i < 10; ++i) adaptive_note_death();
    if (adaptive.skill < -1.0 || adaptive_hp_scale() < 1.0 - ADAPTIVE_HP_RANGE - 1e-9) {
        fprintf(stderr, "error: repeated deaths escaped the adaptive bounds\n"); return 0;
    }
    /* Reduced escalation still spawns at least one enemy, and damage reaches the player scaled. */
    init_game_seed(&game, LEVEL_TEST_SEED + 1202u, GENERATOR_FOREST);
    game.relic_mask = 1;
    sync_relic_progress(&game);
    for (int i = 0; i < game.monster_count; ++i) game.monsters[i].active = 0;
    apply_forest_relic_escalation(&game);
    int spawned = 0;
    for (int i = 0; i < game.monster_count; ++i) spawned += game.monsters[i].active;
    if (spawned != 1) {
        fprintf(stderr, "error: reduced escalation spawned %d enemies instead of one\n", spawned); return 0;
    }
    game.player_health = 100;
    game.trainer = 0;
    apply_player_damage(&game, 20);
    int weak_hit = 100 - game.player_health;
    adaptive_reset();
    game.player_health = 100;
    apply_player_damage(&game, 20);
    int neutral_hit = 100 - game.player_health;
    if (weak_hit >= neutral_hit || neutral_hit != scale_enemy_damage_for_difficulty(&game, 20) || adaptive.window_damage != neutral_hit) {
        fprintf(stderr, "error: adaptive damage scaling is not applied to the player\n"); return 0;
    }
    if (!adaptive_state_valid(&adaptive)) {
        fprintf(stderr, "error: adaptive state fails its own save validation\n"); return 0;
    }
    AdaptiveState broken = adaptive;
    broken.skill = 4.0;
    if (adaptive_state_valid(&broken)) {
        fprintf(stderr, "error: out-of-range adaptive state passes save validation\n"); return 0;
    }
    adaptive_reset();
    return 1;
}

static int verify_sector_geometry(void)
{
    GameState game;
    const int modes[] = {GENERATOR_ROOMS, GENERATOR_TIGHT, GENERATOR_BOSS};
    for (int m = 0; m < 3; ++m) {
        for (int seed = 0; seed < 4; ++seed) {
            init_game_seed(&game, LEVEL_TEST_SEED + (uint32_t)seed, modes[m]);
            int raised = 0;
            for (int y = 1; y < MAP_H - 1; ++y) {
                for (int x = 1; x < MAP_W - 1; ++x) {
                    if (map_at(x, y)) continue;
                    raised += sector_floor[y][x] > 0.0;
                    if (sector_ceiling[y][x] - sector_floor[y][x] < 0.75 ||
                        (!map_at(x + 1, y) && fabs(sector_floor[y][x] - sector_floor[y][x + 1]) > 0.126) ||
                        (!map_at(x, y + 1) && fabs(sector_floor[y][x] - sector_floor[y + 1][x]) > 0.126)) {
                        fprintf(stderr, "error: generated sector has an impassable step or ceiling\n");
                        return 0;
                    }
                }
            }
            if (!raised) {
                fprintf(stderr, "error: dungeon generator did not create raised sectors\n");
                return 0;
            }
            double original_floor[MAP_H][MAP_W], original_ceiling[MAP_H][MAP_W];
            memcpy(original_floor, sector_floor, sizeof(original_floor));
            memcpy(original_ceiling, sector_ceiling, sizeof(original_ceiling));
            for (int i = 0; i < MAX_DOORS; ++i) game.doors[i].open = 1;
            for (int i = 0; i < MAX_SECRETS; ++i) {
                if (game.secrets[i].x > 0 && game.secrets[i].y > 0) {
                    game.secrets[i].open = 1;
                    level_map[game.secrets[i].y][game.secrets[i].x] = 0;
                }
            }
            build_sector_heights(&game);
            if (memcmp(original_floor, sector_floor, sizeof(original_floor)) ||
                memcmp(original_ceiling, sector_ceiling, sizeof(original_ceiling))) {
                fprintf(stderr, "error: restoring open doors/secrets changes sector heights\n");
                return 0;
            }
        }
    }

    memset(&game, 0, sizeof(game));
    game.generator_mode = GENERATOR_ROOMS;
    active_game = &game;
    memset(torches, 0, sizeof(torches));
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) level_map[y][x] = x >= 4 && x <= 12 && y >= 4 && y <= 12 ? 0 : 1;
    }
    build_sector_heights(&game);
    Camera cam = {.pos = {4.5, 8.5}, .dir = {1.0, 0.0}, .plane = {0.0, 0.66}};
    for (int i = 0; i < 75; ++i) move_camera(&cam, &game, 1.0, 0.0, 1.0 / 60.0);
    if (cam.pos.x < 8.4 || fabs(camera_eye_height(&cam) - 0.875) > 0.001) {
        fprintf(stderr, "error: player cannot climb the sector staircase\n");
        return 0;
    }
    Monster monster = {.active = 1, .pos = {4.5, 8.5}};
    for (int i = 0; i < 40; ++i) move_monster_by(&monster, (Vec2){0.1, 0.0});
    if (monster.pos.x < 8.4) {
        fprintf(stderr, "error: monster cannot climb the sector staircase\n");
        return 0;
    }
    sector_floor[8][6] = 0.75;
    if (can_step_between((Vec2){5.5, 8.5}, (Vec2){6.5, 8.5}, 0.18)) {
        fprintf(stderr, "error: player can climb an oversized sector ledge\n");
        return 0;
    }
    monster.pos = (Vec2){5.5, 8.5};
    if (move_monster_by(&monster, (Vec2){1.0, 0.0})) {
        fprintf(stderr, "error: monster can climb an oversized sector ledge\n");
        return 0;
    }

    /* Solid diagnostic billboard: the head must remain visible above a
     * 3/8-unit ledge while the feet retain the nearer riser's depth. */
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            sector_floor[y][x] = 0.0;
            sector_ceiling[y][x] = 1.25;
        }
    }
    sector_floor[8][6] = 0.375;
    cam.pos = (Vec2){4.5, 8.5};
    monster = (Monster){.active = 1, .hp = 10, .pos = {7.5, 8.5}, .type = 0};
    int frame = monster_frame_for_camera(&cam, &monster);
    int anim = monster_anim_frame_for_monster(&monster);
    uint32_t saved_sprite[SPRITE_SIZE * SPRITE_SIZE];
    memcpy(saved_sprite, monster_sprites[0][frame][anim], sizeof(saved_sprite));
    for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; ++i) monster_sprites[0][frame][anim][i] = rgb(224, 40, 32);
    prepare_torch_flicker_cache(0.0);
    reset_render_buffers();
    render_sector_world(&cam, &game);
    int head = (SCREEN_H * 250 / 480) * SCREEN_W + SCREEN_W / 2;
    int feet = (SCREEN_H * 300 / 480) * SCREEN_W + SCREEN_W / 2;
    uint32_t hidden_pixel = framebuffer[feet];
    render_monster(&cam, &monster);
    memcpy(monster_sprites[0][frame][anim], saved_sprite, sizeof(saved_sprite));
    if (fabs(depth_buffer[head] - 3.0) > 0.001 || fabs(depth_buffer[feet] - 1.5) > 0.001 ||
        framebuffer[feet] != hidden_pixel) {
        fprintf(stderr, "error: sector ledge does not correctly occlude a billboard\n");
        return 0;
    }
    return 1;
}

int dump_frame_mode(const char *path, int mode)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!init_assets()) {
        return 1;
    }
    if (!verify_sfx_assets()) {
        return 1;
    }
    if (!verify_monster_path()) {
        return 1;
    }
    if (!verify_player_weapon()) {
        return 1;
    }
    if (!verify_monster_shot_direction()) {
        return 1;
    }
    if (!verify_monster_ai_reacts()) {
        return 1;
    }
    if (!verify_monster_safety_rules()) {
        return 1;
    }
    if (!verify_items_and_fog()) {
        return 1;
    }
    if (!verify_fireball_weapon()) {
        return 1;
    }
    if (!verify_projectile_ownership()) {
        return 1;
    }
    if (!verify_monster_melee_attack()) {
        return 1;
    }
    if (!verify_monster_roles()) {
        return 1;
    }
    if (!verify_doors_and_secrets()) {
        return 1;
    }
    if (!verify_generator_modes()) {
        return 1;
    }
    if (!verify_forest_dungeon_transition()) {
        return 1;
    }
    if (!verify_forest_house_transition()) {
        return 1;
    }
    if (!verify_merchant_shop()) {
        return 1;
    }
    if (!verify_generated_dungeon_relic_reachability()) {
        return 1;
    }
    if (!verify_relic_story_progress()) {
        return 1;
    }
    if (!verify_music_track_mapping()) {
        return 1;
    }
    if (!verify_fm_instrument_distribution()) {
        return 1;
    }
    if (!verify_render_effect_presets()) {
        return 1;
    }
    if (!verify_help_key_close()) {
        return 1;
    }
    if (!verify_prompt_font_glyphs()) {
        return 1;
    }
    if (!verify_difficulty_scaling()) {
        return 1;
    }
    if (!verify_boss_sprite_asset()) {
        return 1;
    }
    if (!verify_furniture_sprite_asset()) {
        return 1;
    }
    if (!verify_monster_sprite_assets()) {
        return 1;
    }
    if (!verify_trainer_mode()) {
        return 1;
    }
    if (!verify_sprite_sort_order()) {
        return 1;
    }
    if (!verify_torches()) {
        return 1;
    }
    if (!verify_volumetric_fog() || !verify_dynamic_shadows()) {
        return 1;
    }
    if (!verify_tree_visible_at_collision_range()) {
        return 1;
    }
    if (!verify_sector_geometry() || !verify_story_puzzles() || !verify_boss_phases() || !verify_adaptive_difficulty()) return 1;
    adaptive_reset();
    printf("adaptive: skill %+.2f hp x%.2f damage x%.2f cooldown x%.2f\n", adaptive.skill, adaptive_hp_scale(), adaptive_damage_scale(), adaptive_cooldown_scale());
    memset(&story, 0, sizeof(story));
    story_popup = -1;
    story_notice_time = 0.0;
    story_dread = 0.0;
    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED, mode);
    if (mode == GENERATOR_HOUSE) {
        cam = (Camera){
            .pos = {8.35, 12.50},
            .dir = {1.0, 0.0},
            .plane = {0.0, 0.66},
        };
    }
    reveal_fog(&game, &cam);
    for (int i = 0; i < 45; ++i) {
        update_game(&game, &cam, 1.0 / 60.0);
    }
    if (mode != GENERATOR_FOREST && mode != GENERATOR_HOUSE) {
        game.fireball_unlocked = 1;
        game.fireball_ammo = 6;
        select_weapon(&game, WEAPON_FIREBALL);
        player_fire(&game, &cam);
        for (int i = 0; i < 18; ++i) {
            update_projectiles(&game, &cam, 1.0 / 60.0);
        }
    }
    render_scene(&cam, &game);
    return write_ppm(path);
}

int dump_frame(const char *path)
{
    return dump_frame_mode(path, GENERATOR_ROOMS);
}

int dump_frame_quality(const char *path, int quality)
{
    int previous_quality = render_quality;
    render_quality = quality;
    int result = dump_frame(path);
    render_quality = previous_quality;
    return result;
}

int parse_generator_mode_name(const char *text, int *out_mode)
{
    if (strcmp(text, "rooms") == 0) {
        *out_mode = GENERATOR_ROOMS;
        return 1;
    }
    if (strcmp(text, "forest") == 0) {
        *out_mode = GENERATOR_FOREST;
        return 1;
    }
    if (strcmp(text, "tight") == 0) {
        *out_mode = GENERATOR_TIGHT;
        return 1;
    }
    if (strcmp(text, "boss") == 0) {
        *out_mode = GENERATOR_BOSS;
        return 1;
    }
    if (strcmp(text, "house") == 0) {
        *out_mode = GENERATOR_HOUSE;
        return 1;
    }
    return 0;
}

int profile_dump_frame(const char *path, int quality, int mode)
{
    RenderProfile profile;
    int previous_quality = render_quality;
    RenderProfile *previous_profile = active_profile;
    render_quality = quality;
    active_profile = &profile;
    int result = dump_frame_mode(path, mode);
    active_profile = previous_profile;
    render_quality = previous_quality;
    if (result == 0) {
        fprintf(stderr,
                "profile total=%.3f floor=%.3f wall=%.3f sprite=%.3f fog=%.3f bloom=%.3f post=%.3f\n",
                profile.total_ms,
                profile.floor_ms,
                profile.wall_ms,
                profile.sprite_ms,
                profile.fog_ms,
                profile.bloom_ms,
                profile.post_ms);
    }
    return result;
}

int dump_forest_forward_frames(const char *prefix)
{
    Camera cam = {
        .pos = {2.5, 22.5},
        .dir = {1.0, 0.0},
        .plane = {0.0, 0.66},
    };
    if (!init_assets()) {
        return 1;
    }

    GameState game;
    init_game_seed(&game, LEVEL_TEST_SEED, GENERATOR_FOREST);
    reveal_fog(&game, &cam);
    game.time = 0.75;
    game.monster_count = 0;
    memset(game.monsters, 0, sizeof(game.monsters));
    memset(game.items, 0, sizeof(game.items));
    memset(game.projectiles, 0, sizeof(game.projectiles));

    for (int frame = 0; frame < 3; ++frame) {
        if (frame > 0) {
            for (int step = 0; step < 18; ++step) {
                move_camera(&cam, &game, 1.0, 0.0, 1.0 / 60.0);
                reveal_fog(&game, &cam);
            }
        }
        render_scene(&cam, &game);

        char path[256];
        int n = snprintf(path, sizeof(path), "%s_%d.ppm", prefix, frame);
        if (n < 0 || n >= (int)sizeof(path)) {
            fprintf(stderr, "error: dump prefix is too long\n");
            return 1;
        }
        if (write_ppm(path) != 0) {
            return 1;
        }
    }
    return 0;
}
