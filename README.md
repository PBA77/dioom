# Software Raycaster

Minimalny raycaster w C w stylu Wolfensteina. Cała scena jest renderowana softwareowo do bufora `uint32_t`, a SDL2 służy tylko do okna, inputu, audio i pokazania gotowej tekstury.

Tekstury ścian, podłogi i sufitu są ładowane z `assets/textures.ppm`: atlas 4x2, osiem kafli po 64x64 px. Aktualny zestaw ma ciemny gotycki klimat krypty/dungeonu. Brak lub błędny atlas kończy program błędem.

Potwory są ładowane z `assets/monsters.ppm`: atlas 4x3, trzy typy przeciwników po cztery klatki 64x64 px w kolejności front, prawy bok, tył, lewy bok. Magenta `#ff00ff` jest traktowana jako kolor przezroczysty.

Dziesięć potworów patroluje poziom, większość z nich to szkielety. Przeciwnicy wykrywają gracza po line-of-sight, zapamiętują ostatnią pozycję, ścigają, trzymają dystans, strafują podczas ataku i strzelają ognistymi pociskami tylko w stanie ataku. Trzy typy przeciwników mają role: strzelec, szybki harasser i wolniejszy bruiser. Pociski są billboardami renderowanymi softwareowo, kolidują ze ścianami i dają czerwony flash na trafienie gracza.

Gracz ma pistolet hitscan oraz odblokowywany fireball z osobną amunicją. Fireball leci jako projectile, wybucha na ścianie albo przeciwniku i zadaje obrażenia obszarowe. Trafione potwory mają krótki pain flash, a śmierć robi proceduralny pomarańczowy pop.

Level jest generowany runtime: pokoje, korytarze, drzwi, sekrety, pochodnie, itemy i potwory powstają z seeda przy starcie runu. Są trzy tryby generatora: klasyczny układ pokoi i korytarzy, ciasny jednokaflowy labirynt z wejściem i dalekim wyjściem oraz boss level z labiryntem i komnatą bossa. Restart `R` tworzy następny seed w aktualnym trybie. `make dump` używa stałego seeda testowego, żeby render i walidacje były powtarzalne.

Na mapie są itemy: apteczki, amunicja, rapid fire, damage boost, klucz i fireball ammo/unlock. Pickupy są rozłożone także w bocznych odnogach, żeby eksploracja dawała zasoby do walki. Są proste drzwi, zamknięte drzwi na klucz i sekretne ściany otwierane interakcją. W prawym górnym rogu jest minimapa z fog-of-war, odkrywająca teren wokół gracza, a `Tab` pokazuje większą automapę.

Na końcu poziomu czeka prosty mini-boss: większy, twardszy bruiser z mocniejszym pociskiem. Walka ma lekkie odpychanie przy kolizji z potworami, proceduralne dźwięki strzałów/pickupów/eksplozji oraz pauzę, restart i fullscreen.

Render ma eksperymentalną mgłę wolumetryczną: dystansowy fog na ścianach, podłodze, suficie i sprite'ach oraz animowany, kłębiasty pass po buforze świata oparty o `z_buffer`. Mgła jest cięższa przy ziemi.

Na ścianach są pochodnie renderowane jako softwareowe sprite'y z poświatą. Ich ciepłe światło wpływa lokalnie na ściany, podłogę, sufit i wolumetryczną mgłę, a same pochodnie są sortowane i zasłaniane przez kolumnowy `z_buffer`. Wewnętrzne korytarze mają dodatkowe pochodnie, więc mrok zostaje, ale mapa nie ma długich martwych czarnych odcinków.

Render ma dodatkowe softwareowe passy: light buffer/pseudo-deferred composite, bloom z blur buforem, proceduralny bump/normal lighting ścian, dynamiczne cienie pod sprite'ami, scorch decale po eksplozjach, soft particles dla dymu/iskier i końcowy color grading z vignette.

## Build

```sh
make
./raycaster
```

Makefile domyślnie używa SDL2 z Homebrew:

```sh
SDL2_PREFIX=/opt/homebrew/opt/sdl2 make
```

## Sterowanie

- `W/S` albo strzałki góra/dół: przód/tył
- `A/D` albo strzałki lewo/prawo: obrót
- `Q/E`: strafing
- `1/2`: pistolet/fireball po odblokowaniu
- `3/4/5`: generator pokoi / ciasny labirynt / boss level
- `Spacja` albo lewy przycisk myszy: strzał z wybranej broni
- `F`: interakcja z drzwiami i sekretami
- `Tab`: pełna automapa
- `P`: pauza
- `R`: restart runu
- `F11`: fullscreen
- `Esc`: wyjście

## Render test

Bez otwierania okna można zapisać pojedynczą klatkę:

```sh
make dump
open frame.ppm
```
