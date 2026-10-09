#include "dioom.h"

static void awaken_story_witness(GameState *game, const Camera *cam);
static void draw_story_lines(int x, int y, const char *text, int scale);

static const char *story_texts[STORY_ENTRY_COUNT] = {
    "Wrociles do osady, lecz nikt nie\nodpowiada. Nawet ptaki milcza.\n\nNa bramie ktos wyryl: ZLOZ DZWON.\nJego cztery fragmenty ukryto w kryptach.\nPieczecie otwiera sie znakami.\n\nPrzeszukuj domy i czytaj inskrypcje.\nJ otwiera dziennik. F bada obiekty.",
    "Nad wejsciem wisial kiedys dzwon.\nPozostal tylko slad po sznurze.\n\nTrzy kamienie nosza znaki slonca,\nkropli i ksiezyca. Relikwie chroni\npieczec: przywolaj znaki we wlasciwej\nkolejnosci. Wskazowka jest na kamieniach.",
    "Na wszystkich grobach skuto imiona.\nZmarli nie mogli uslyszec wezwania.\n\nDruga pieczec pamieta pogrzeb, ogien\ni narodziny. Zbadaj trzy kamienie.\nTa krypta ma inna kolejnosc znakow.",
    "Kamienne rynny prowadza do korzeni.\nNie sluzyly do odprowadzania deszczu.\n\nTrzecia pieczec opowiada o krwi,\nodkopanym grobie i spaleniu szczatkow.\nPrzywolaj jej trzy znaki.",
    "Woda odbija twarz, ktorej nie poznajesz.\nZ glebi dobiega dzwiek twoich krokow.\n\nOstatnia pieczec przywraca pamiec.\nPopiol odslania grob, a krew oddaje\nzmarlemu imie. Zbadaj trzy kamienie.",
    "Nie bijcie w dzwon po zachodzie.\nTo, co spi pod lasem, uczy sie naszych\nglosow. Kazdy fragment budzi korzenie.\n\nJesli popelnisz blad przy pieczeci,\nprzestan szeptac. Ktos juz cie uslyszal.",
    "Nie chowalismy cial. Chowalismy imiona.\nDzwonnik mowil, ze bez imion zmarli\nnie znajda drogi do swoich domow.\n\nNazwal to ochrona. My nazwalismy to\nwiezieniem. Klucz podzielil na cztery.",
    "Gdy ostatni glos zamilkl, straznik\nzszedl pod brame. Jego twarz zniknela,\nale nadal trzyma noz dzwonnika.\n\nZlozony dzwon otworzy wejscie.\nStraznik nie odda glosow dobrowolnie.",
    "Pod rozkazem zamkniecia krypt widzisz\nwlasny podpis. Nie pamietasz, dlaczego\nwybrales cisze zamiast smierci.\n\nTeraz wiesz, dlaczego las cie wpuscil.\nMasz dokonczyc to, czego nie zrobiles.",
    "W metalu zapisano pierwszy glos:\nPOZWOL NAM WROCIC.\n\nGdy podnosisz fragment, korzenie drza.\nCisza osady byla zamknieta pieczecia.\nTo, co otwierasz, czuje kazdy twoj ruch.",
    "Drugi fragment brzmi jak oddech.\nRozpoznajesz glos grabarza.\n\nLudzie nie odeszli z osady. Ich glosy\nukryto w dzwonie, a ciala zostawiono\nkorzeniom. Straznik nadal pilnuje bramy.",
    "Na trzecim fragmencie jest data\nzamkniecia osady. Twoja ostatnia noc.\n\nZgodziles sie ocalic ludzi za cene\npamieci. Korzenie zabraly wiecej,\nniz obiecal dzwonnik.",
    "Cztery fragmenty drza jednym tonem.\nPrzypominasz sobie imiona zmarlych.\n\nWroc do bramy w lesie. Zloz dzwon\ni odbierz glosy ostatniemu straznikowi.\nNie pozwol mu ponownie zamknac osady.",
    "Straznik pada. Po raz pierwszy od lat\ndzwon rozbrzmiewa pelnym glosem.\n\nKorzenie rozluzniaja uscisk. Imiona\nwracaja na groby, a w lesie odzywa sie\npierwszy ptak. Pamietasz juz wszystko.\n\nTym razem zostawiasz brame otwarta.",
    "Schody prowadza pod brame, do sali,\nktorej nie ma na zadnej mapie osady.\n\nStraznik czeka w glebi. Trzyma noz\ndzwonnika i powtarza twoje slowa.\nZlozony dzwon drzy przy kazdym kroku.\n\nGdy oslabnie, obudzi korzenie i ciala,\nktore im zostawiono. Nie przerywaj.\nGlosy czekaly na ciebie zbyt dlugo."
};

static const char *story_titles[STORY_ENTRY_COUNT] = {
    "LAS BEZ GLOSU", "KAPLICA POPIOLU", "GROB BEZ IMIENIA", "DOM OSTATNIEJ KRWI", "STUDNIA PAMIECI",
    "LIST DZWONNIKA", "ZAPIS GRABARZA", "OSTATNIA STRONA", "TWOJ PODPIS",
    "PIERWSZY FRAGMENT", "DRUGI FRAGMENT", "TRZECI FRAGMENT", "OSTATNI FRAGMENT", "CISZA PO DZWONIE",
    "BRAMA POD LASEM"
};

static const char *crypt_riddles[RELIC_COUNT] = {
    "Najpierw slonce rozgrzalo popiol.\nPotem w ciele obudzila sie krew.\nNa koncu ksiezyc zobaczyl tylko kosci.",
    "Najpierw pochowano kosci.\nPotem ogien zostawil po nich popiol.\nNa koncu nowa krew przerwala cisze.",
    "Najpierw ziemia wypila krew.\nPotem odkopano biale kosci.\nNa koncu stos zamienil je w popiol.",
    "Najpierw wiatr przyniosl popiol.\nPotem odkryl ukryte pod nim kosci.\nNa koncu krew oddala zmarlemu imie."
};

static const char *crypt_names[RELIC_COUNT] = {
    "KAPLICA POPIOLU", "GROB BEZ IMIENIA", "DOM OSTATNIEJ KRWI", "STUDNIA PAMIECI"
};

const int seal_orders[RELIC_COUNT][3] = {{0, 1, 2}, {2, 0, 1}, {1, 2, 0}, {0, 2, 1}};

const char *seal_names[3] = {"POPIOL / SLONCE", "KREW / KROPLA", "KOSCI / KSIEZYC"};

double story_notice_time = 0.0;

const char *story_notice = NULL;

double story_dread = 0.0;

int story_popup = -1;

StoryProgress story;

void story_discover(int entry)
{
    if (entry < 0 || entry >= STORY_ENTRY_COUNT || (story.notes_mask & (1u << entry))) return;
    story.notes_mask |= 1u << entry;
    story_popup = entry;
}

void story_message(const char *text)
{
    story_notice = text;
    story_notice_time = 4.0;
}

int story_prop_entry(const GameState *game, const Prop *prop)
{
    if (game->generator_mode != GENERATOR_HOUSE || game->current_house_variant < 0 || game->current_house_variant > 2 || prop->loot_slot < 0) return -1;
    return 5 + (game->current_house_variant == 2 ? 2 : 0) + (prop->loot_slot == 4 ? 1 : 0);
}

int story_relic_unsealed(const GameState *game, const Item *item)
{
    int relic = item->relic_index;
    return game->trainer || relic < 0 || relic >= RELIC_COUNT || game->dungeon_relic_index != relic ||
           (story.solved_mask & (1u << relic));
}

int place_story_seals(GameState *game)
{
    int relic = game->dungeon_relic_index;
    if (relic < 0 || relic >= RELIC_COUNT || (game->relic_mask & (1u << relic))) return 1;
    const int slots[3] = {2, 6, 7};
    for (int i = 0; i < 3; ++i) game->items[slots[i]].active = 0;
    unsigned char reachable[MAP_H][MAP_W];
    mark_dungeon_reachable_tiles(game, reachable);
    double farthest = 0.0;
    for (int y = 1; y < MAP_H - 1; ++y) {
        for (int x = 1; x < MAP_W - 1; ++x) {
            if (reachable[y][x] && generated_floor(x, y)) farthest = fmax(farthest, sqrt(start_dist2(x, y)));
        }
    }
    for (int symbol = 0; symbol < 3; ++symbol) {
        double target = farthest * (0.22 + symbol * 0.28);
        double best = 1e30;
        int bx = -1, by = -1;
        for (int y = 1; y < MAP_H - 1; ++y) {
            for (int x = 1; x < MAP_W - 1; ++x) {
                if (!reachable[y][x] || !generated_floor(x, y) || occupied_spawn_tile(game, x, y) || start_dist2(x, y) < 9.0) continue;
                int crowded = 0;
                for (int j = 0; j < symbol; ++j) {
                    Vec2 p = game->items[slots[j]].pos;
                    double dx = p.x - x - 0.5, dy = p.y - y - 0.5;
                    if (dx * dx + dy * dy < 4.0) crowded = 1;
                }
                if (crowded) continue;
                double score = fabs(sqrt(start_dist2(x, y)) - target);
                if (score < best) { best = score; bx = x; by = y; }
            }
        }
        if (bx < 0) {
            fprintf(stderr, "error: crypt %d cannot place reachable ritual stone %d\n", relic + 1, symbol + 1);
            return 0;
        }
        game->items[slots[symbol]] = (Item){1, ITEM_SEAL, symbol, {bx + 0.5, by + 0.5}};
    }
    return 1;
}

int active_seal_index(const GameState *game, const Camera *cam)
{
    int best = -1;
    double nearest = 1.75;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *item = &game->items[i];
        if (!item->active || item->type != ITEM_SEAL || item->relic_index < 0 || item->relic_index >= 3) continue;
        Vec2 v = {item->pos.x - cam->pos.x, item->pos.y - cam->pos.y};
        double d = vec_len(v);
        if (d > nearest || (d > 0.35 && vec_dot(vec_norm(v), cam->dir) < 0.65) ||
            !has_line_of_sight(cam->pos, item->pos)) continue;
        nearest = d;
        best = i;
    }
    return best;
}

static void awaken_story_witness(GameState *game, const Camera *cam)
{
    /* Alert nearby patrols; a dead enemy slot can become one visible omen.
     * Never create an enemy on the player's tile or across a sealed wall. */
    for (int i = 0; i < game->monster_count; ++i) {
        Monster *monster = &game->monsters[i];
        double dx = monster->pos.x - cam->pos.x, dy = monster->pos.y - cam->pos.y;
        if (monster->active && dx * dx + dy * dy < 64.0) {
            monster->last_seen = cam->pos;
            monster->alert_timer = 5.0;
            monster->ai_state = 1;
        }
    }
    int slot = -1;
    for (int i = 0; i < game->monster_count; ++i) {
        if (!game->monsters[i].active && !game->monsters[i].is_boss) { slot = i; break; }
    }
    if (slot < 0) return;
    Vec2 spawn = {0.0, 0.0};
    for (int y = 1; y < MAP_H - 1 && spawn.x == 0.0; ++y) {
        for (int x = 1; x < MAP_W - 1; ++x) {
            Vec2 p = {x + 0.5, y + 0.5};
            double dx = p.x - cam->pos.x, dy = p.y - cam->pos.y;
            double d = dx * dx + dy * dy;
            if (d < 9.0 || d > 30.0 || occupied_spawn_tile(game, x, y) ||
                !can_occupy(p.x, p.y, 0.28) || !has_line_of_sight(cam->pos, p)) continue;
            spawn = p; break;
        }
    }
    if (spawn.x == 0.0) return;
    game->monsters[slot] = (Monster){
        .active = 1, .hp = adaptive_monster_hp(game, monster_max_hp(1)),
        .pos = spawn, .type = 1, .facing = {0.0, -1.0}, .route = slot,
        .patrol = {spawn, cam->pos}, .patrol_count = 2, .target_waypoint = 1,
        .last_seen = cam->pos, .alert_timer = 5.0, .ai_state = 1, .shoot_timer = 1.0,
    };
}

int invoke_story_seal(GameState *game, const Camera *cam, int item_index)
{
    int relic = game->dungeon_relic_index;
    if (relic < 0 || relic >= RELIC_COUNT || item_index < 0 || item_index >= MAX_ITEMS ||
        !game->items[item_index].active || game->items[item_index].type != ITEM_SEAL) return 0;
    if (story.solved_mask & (1u << relic)) {
        story_message("PIECZEC OTWARTA. ODSZUKAJ RELIKWIE.");
        return 1;
    }
    int step = story.ritual_steps[relic];
    if (step < 0 || step > 2) return 0;
    if (game->items[item_index].relic_index != seal_orders[relic][step]) {
        story.ritual_steps[relic] = 0;
        if (story.failures[relic] < 999) story.failures[relic]++;
        story_dread = 4.0;
        game->screen_shake_timer = 0.25;
        game->screen_shake_strength = 0.04;
        if (story.failures[relic] <= 3) awaken_story_witness(game, cam);
        story_message("ZLY ZNAK. CISZA PEKLA. ZACZNIJ OD NOWA.");
        play_sfx(SFX_LOCKED, 0.56);
        return 0;
    }
    story.ritual_steps[relic]++;
    if (story.ritual_steps[relic] == 3) {
        story.solved_mask |= 1u << relic;
        story_message("PIECZEC PEKLA. RELIKWIA JEST WOLNA.");
        play_sfx(SFX_RELIC, 0.56);
    } else {
        story_message(story.ritual_steps[relic] == 1 ? "PIERWSZY ZNAK ODPOWIEDZIAL." : "DRUGI ZNAK ODPOWIEDZIAL.");
        play_sfx(SFX_SHRINE, 0.34);
    }
    return 1;
}

void open_story_panel(Runtime *rt, int panel, int entry, int seal)
{
    rt->story_panel = panel;
    rt->story_entry = entry;
    rt->story_seal = seal;
    rt->paused = 1;
}

void consume_story_popup(Runtime *rt)
{
    if (story_popup >= 0 && rt->game_started && !rt->menu_open && !rt->shop_open && !rt->story_panel) {
        open_story_panel(rt, 1, story_popup, -1);
        story_popup = -1;
    }
}

static void draw_story_lines(int x, int y, const char *text, int scale)
{
    char line[45];
    int row = 0;
    while (*text && row < 17) {
        int n = 0;
        while (text[n] && text[n] != '\n' && n < 44) n++;
        int used = n;
        if (n == 44 && text[n] && text[n] != '\n') {
            while (used > 0 && text[used] != ' ') used--;
            if (used == 0) used = n;
        }
        memcpy(line, text, (size_t)used);
        line[used] = '\0';
        for (int i = 0; i < used; ++i) {
            if (line[i] >= 'a' && line[i] <= 'z') line[i] = (char)(line[i] - 'a' + 'A');
        }
        draw_scaled_text(x, y + row * 18 * scale, line, rgb(204, 193, 170), 2 * scale);
        text += used;
        if (*text == '\n' || *text == ' ') text++;
        row++;
    }
}

void render_story_panel(const Runtime *rt)
{
    int scale = SCREEN_H / 480;
    int w = 576 * scale, h = 408 * scale;
    int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;
    blend_rect(0, 0, SCREEN_W, SCREEN_H, rgb(0, 0, 0), 0.76);
    fill_rect(x, y, w, h, rgb(18, 17, 15));
    fill_rect(x + 8 * scale, y + 8 * scale, w - 16 * scale, h - 16 * scale, rgb(26, 24, 20));
    fill_rect(x + 20 * scale, y + 44 * scale, w - 40 * scale, scale, rgb(116, 92, 52));
    const char *title;
    char body[1600];
    const char *footer;
    if (rt->story_panel == 2) {
        int relic = rt->game.dungeon_relic_index;
        int symbol = rt->game.items[rt->story_seal].relic_index;
        title = seal_names[symbol];
        snprintf(body, sizeof(body), "%s\n\nINSKRYPCJA:\n%s\n\nPrzywolane znaki: %d/3\n\n%s", crypt_names[relic], crypt_riddles[relic],
                 story.ritual_steps[relic], (story.solved_mask & (1u << relic)) ? "Pieczec otwarta. Odszukaj relikwie." : "Zbadaj pozostale kamienie przed wyborem.");
        footer = "ENTER PRZYWOLAJ  J DZIENNIK  ESC WROC";
    } else {
        int entry = rt->story_entry;
        title = story_titles[entry];
        if (entry >= 1 && entry <= 4) snprintf(body, sizeof(body), "%s\n\nINSKRYPCJA:\n%s", story_texts[entry], crypt_riddles[entry - 1]);
        else snprintf(body, sizeof(body), "%s", story_texts[entry]);
        footer = rt->story_panel == 3 ? "A/D ZAPISY  ENTER LUB J ZAMKNIJ" : "ENTER WROC DO GRY  J DZIENNIK";
    }
    int title_w = (int)strlen(title) * 18 * scale;
    draw_scaled_text(SCREEN_W / 2 - title_w / 2, y + 17 * scale, title, rgb(230, 190, 116), 3 * scale);
    draw_story_lines(x + 24 * scale, y + 56 * scale, body, scale);
    draw_scaled_text(x + 24 * scale, y + h - 25 * scale, footer, rgb(171, 147, 101), 2 * scale);
    if (rt->story_panel == 3) {
        int found = 0;
        for (int i = 0; i < STORY_ENTRY_COUNT; ++i) found += (story.notes_mask >> i) & 1u;
        char counter[24];
        snprintf(counter, sizeof(counter), "ZAPIS %d/%d", found, STORY_ENTRY_COUNT);
        int counter_w = (int)strlen(counter) * 12 * scale;
        draw_scaled_text(x + w - 24 * scale - counter_w, y + h - 25 * scale, counter, rgb(171, 147, 101), 2 * scale);
    }
}
