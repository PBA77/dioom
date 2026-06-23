# Software Raycaster

Minimalny raycaster w C w stylu Wolfensteina. Cała scena jest renderowana softwareowo do bufora `uint32_t`, a SDL2 służy tylko do okna, inputu i pokazania gotowej tekstury.

Tekstury ścian, podłogi i sufitu są ładowane z `assets/textures.ppm`: atlas 4x2, osiem kafli po 64x64 px. Aktualny zestaw ma ciemny gotycki klimat krypty/dungeonu. Brak lub błędny atlas kończy program błędem.

Potwory są ładowane z `assets/monsters.ppm`: atlas 4x3, trzy typy przeciwników po cztery klatki 64x64 px w kolejności front, prawy bok, tył, lewy bok. Magenta `#ff00ff` jest traktowana jako kolor przezroczysty.

Dziesięć potworów patroluje poziom, większość z nich to szkielety. Przeciwnicy wykrywają gracza po line-of-sight, zapamiętują ostatnią pozycję, ścigają, trzymają dystans, strafują podczas ataku i strzelają ognistymi pociskami tylko w stanie ataku. Pociski są billboardami renderowanymi softwareowo, kolidują ze ścianami i dają czerwony flash na trafienie gracza. Gracz ma pistolet z animacją odrzutu, smugą strzału, większe zdrowie, amunicję i wygrywa po ubiciu wszystkich potworów.

Na mapie są itemy: apteczki, amunicja, rapid fire i damage boost. W prawym górnym rogu jest minimapa z fog-of-war, odkrywająca teren wokół gracza.

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
- `Spacja` albo lewy przycisk myszy: strzał
- `Esc`: wyjście

## Render test

Bez otwierania okna można zapisać pojedynczą klatkę:

```sh
make dump
open frame.ppm
```
