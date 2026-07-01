# Dioom

Dioom to minimalny raycaster w C w stylu Wolfensteina. Cała scena jest renderowana softwareowo do bufora `uint32_t`, a SDL2 służy tylko do okna, inputu, audio i pokazania gotowej tekstury.

Tekstury ścian, podłogi i sufitu są ładowane z `assets/textures.ppm`: atlas 4x2, osiem kafli po 64x64 px. Aktualny zestaw miesza ciemny gotycki dungeon z kaflami mrocznego lasu: ziemia, mech, korzenie, kora i czarny baldachim drzew. Brak lub błędny atlas kończy program błędem.

Potwory są ładowane z `assets/monsters.ppm`: atlas 4x24, sześć typów przeciwników po cztery kierunki i cztery klatki animacji 64x64 px. W każdym bloku typu rzędy to front, prawy bok, tył i lewy bok. Front używa klatek idle/przygotowanie/atak/gest, a pozostałe kierunki traktują kolumny jako warianty chodzenia lub ruchu. Magenta `#ff00ff` jest traktowana jako kolor przezroczysty.

Wielki szkielet i boss są ładowane z osobnych atlasów 4x4: `assets/giant_skeleton.ppm` i `assets/boss.ppm`, cztery kierunki po cztery klatki animacji 128x128 px. Magenta `#ff00ff` jest przezroczysta. Brak atlasu kończy program błędem.

Drzewa lasu są ładowane z `assets/trees.ppm`: atlas 4x1, cztery warianty billboardów 64x64 px. Magenta `#ff00ff` jest przezroczysta. Brak atlasu kończy program błędem.

Kapliczki cmentarne w lesie są ładowane z `assets/houses.ppm`: atlas 2x2 po 64x64 px dla frontu, boków, tyłu i dachu. Meble we wnętrzach kapliczek są prostymi teksturowanymi bryłami, a ich powierzchnie są ładowane z `assets/furniture.ppm`: atlas 8x1, osiem kafli po 64x64 px. Brak któregoś atlasu kończy program błędem.

Relikwie kultu są ładowane z `assets/relics.ppm`: atlas 4x1, cztery sprity pickupów 32x32 px. Magenta `#ff00ff` jest przezroczysta. Brak atlasu kończy program błędem.

Dziesięć potworów patroluje poziom. Przeciwnicy wykrywają gracza po line-of-sight i FOV, zapamiętują ostatnią pozycję, ścigają, trzymają dystans i reagują na pobliskiego potwora, który widzi gracza. Szkielet strzela ognistymi pociskami, a pozostałe typy, w tym zjawa, atakują głównie ciosami z bliska.

Gracz startuje z nożem do krótkiego dystansu. Pistolet hitscan i fireball z osobną amunicją są odblokowywane pickupami. Fireball leci jako projectile, wybucha na ścianie albo przeciwniku i zadaje obrażenia obszarowe. Trafione potwory mają krótki pain flash, a śmierć robi proceduralny pomarańczowy pop. W tle gra eksperymentalny programowy syntezator FM; las używa cichego ponurego dronu, a każda krypta relikwii wybiera inny utwór MIDI z `assets/music/`.

Gra startuje w mrocznym lesie i z nożem w ręku; pistolet i fireball trzeba znaleźć jako pickupy. Level jest generowany runtime: pokoje, korytarze, drzwi, sekrety, pochodnie albo ogniska, itemy i potwory powstają z seeda przy starcie runu. Są cztery tryby generatora: klasyczny układ pokoi i korytarzy, ciasny jednokaflowy labirynt z wejściem i dalekim wyjściem, boss level z labiryntem i komnatą bossa oraz mroczny las jako duży ogrodzony teren z drzewami, ogniskami, kapliczkami cmentarnymi, potworami, itemami i wejściami do dungeonów. Las ma cztery wejścia do dungeonów z relikwiami kultu oraz osobną bramę bossa. Kapliczki blokują ruch, a `F` przy frontowych drzwiach przenosi do osobnego wnętrza z meblami, skrytkami i jednorazowym lootem. Wejście zapisuje aktualny stan lasu, a wyjście w dungeonie albo kapliczce odtwarza ten sam las i kamerę przy wejściu. Restart `R` tworzy następny seed w aktualnym trybie. `make dump` używa stałego seeda testowego, żeby render i walidacje były powtarzalne.

Na mapie są itemy: apteczki, amunicja, rapid fire, damage boost, klucz, fireball ammo/unlock, złoto, kapliczki i cztery relikwie kultu. Pickupy są rozłożone także w bocznych odnogach, żeby eksploracja dawała zasoby do walki. Potwory wyrzucają małe kupki złota, kapliczki dają jednorazowy efekt w stylu Diablo, a stosy kości budują klimat i nie są podnoszone. Są proste drzwi, zamknięte drzwi na klucz i sekretne ściany otwierane interakcją. W prawym górnym rogu jest minimapa z fog-of-war, odkrywająca teren wokół gracza, a `Tab` pokazuje większą automapę.

Zebranie kompletu czterech relikwii odblokowuje bramę bossa w lesie. Boss jest dużo większy i twardszy od zwykłych przeciwników, ma mocniejszy atak z bliska i wolniejszy, bolesny pocisk dystansowy. Zwycięstwo następuje dopiero po zabiciu bossa. Walka ma lekkie odpychanie przy kolizji z potworami, sample dźwiękowe strzałów/pickupów/eksplozji oraz pauzę, restart i fullscreen. Poziomy trudności to łatwy, normalny, trudny i nightmare; wyższe poziomy podbijają HP oraz obrażenia przeciwników.

Render ma eksperymentalną mgłę wolumetryczną: dystansowy fog na ścianach, podłodze, suficie i sprite'ach oraz animowany, kłębiasty pass po buforze świata oparty o `z_buffer`. Mgła jest cięższa przy ziemi.

Na ścianach są pochodnie renderowane jako softwareowe sprite'y z poświatą. Ich ciepłe światło wpływa lokalnie na ściany, podłogę, sufit i wolumetryczną mgłę, a same pochodnie są sortowane i zasłaniane przez kolumnowy `z_buffer`. Wewnętrzne korytarze mają dodatkowe pochodnie, więc mrok zostaje, ale mapa nie ma długich martwych czarnych odcinków.

Render ma dodatkowe softwareowe passy: light buffer/pseudo-deferred composite, bloom z blur buforem, threshold bloom dla jasnych pikseli, selektywny edge antialiasing, proceduralny bump/normal lighting ścian, dynamiczne cienie pod sprite'ami, scorch decale po eksplozjach, soft particles dla dymu/iskier i końcowy color grading z vignette. Cięższe passy post-processingu są przełączane przez `--effects full` albo `F6`.

## Build

```sh
make
./dioom
```

Makefile domyślnie używa SDL2 z Homebrew:

```sh
SDL2_PREFIX=/opt/homebrew/opt/sdl2 make
```

## Sterowanie

- `W/S` albo strzałki góra/dół: przód/tył
- `A/D` albo strzałki lewo/prawo: obrót
- `Q/E`: strafing
- `1/2/3`: nóż / pistolet po podniesieniu / fireball po podniesieniu
- `4/5/6/7`: generator pokoi / ciasny labirynt / boss level / mroczny las
- `Spacja` albo lewy przycisk myszy: strzał z wybranej broni
- `F`: interakcja z drzwiami, sekretami, wejściami i wyjściami dungeonów/kapliczek oraz lootowalnymi meblami
- `H`: pokazuje/ukrywa podpowiedź celu
- `Tab`: pełna automapa
- `P`: pauza
- `R`: restart runu
- `F3/F4/F5/F6`: FPS / timing / jakość renderu / post-processing
- `F8/F9`: menu zapisu / odczytu gry z 8 slotami `dioom_slot1.sav`..`dioom_slot8.sav`
- `F11`: fullscreen
- `Esc`: menu gry
- Menu: `W/S` albo strzałki wybierają pozycję, `Enter`/`Spacja` zatwierdza, a `Esc` wraca poziom wyżej. Ustawienia są w osobnym podmenu, gdzie `A/D` zmienia trudność, jakość, post-processing, suwaki głośności FX/muzyki i fullscreen. Menu główne zawiera start/wznowienie, restart runu, zapis, odczyt, ustawienia i wyjście.

Ustawienia menu są zapisywane do `dioom.ini` przy wyjściu i po zmianie opcji. Obsługiwane klucze to `difficulty=easy|normal|hard|nightmare`, `quality=pbr|fast`, `post_process=full|off`, `fullscreen=0|1`, `sfx_volume=0..8`, `music_volume=0..8` i ukryte `trainer=0|1`. Trainer blokuje obrażenia gracza, nie zużywa amunicji ani fireball ammo oraz startuje z kompletem czterech relikwii.

## Render test

Bez otwierania okna można zapisać pojedynczą klatkę:

```sh
make dump
open frame.ppm
```

Do kontroli wnętrza kapliczki można wyrenderować osobny kadr:

```sh
./dioom --dump-house /tmp/house.ppm
```
