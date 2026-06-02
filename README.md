# Doom Clone - Raycasting FPS v C++ a SDL3

## O projektu
Tento projekt je semestrální práce z předmětu zaměřeného na programování v C++. Jedná se o klon klasické retro střílečky z 90. let (jako je Doom nebo Wolfenstein 3D). Hra je kompletně napsaná v jazyce C++ s využitím knihovny SDL3 pro vykreslování obrazu a práci se zvukem. Vykreslování 3D prostředí probíhá pomocí vlastní implementace Raycastingu.

Hra obsahuje:
- Raycasting pro generování 3D labyrintu.
- Správu zbraní (Pistole, Brokovnice, Rotační kulomet), každá má vlastní logiku, rychlost palby a poškození.
- Umělou inteligenci nepřátel, kteří hledají cestu k hráči.
- Interaktivní prostředí (otevírání dveří, sbírání munice a lékárniček).
- Dynamické stíny na podlaze a texturování zdí/stropů s využitím z-bufferu.

## Jak to funguje a jak hru spustit
Projekt využívá jako sestavovací systém CMake. 

Kompilace:
```bash
cmake -B build
cmake --build build --config Release
```

Spuštění:
Vzniklý spustitelný soubor (např. `raycasting_engine.exe`) je nutné spustit ze složky, ze které má přístup ke složce `assets/`, která obsahuje veškeré textury a zvuky.

## Návrh architektury
Původní návrh počítal s jednodušším procedurálním přístupem, ale s přibývající složitostí byl kód refaktorován do plně objektového modelu (OOP). Bylo dbáno na silné zapouzdření (encapsulation) a rozdělení logiky do separátních modulů:

- **Engine (`engine.hpp`, `engine_core.cpp`, `engine_update.cpp`, `engine_render.cpp`, `engine_assets.cpp`)**: Třída zodpovídající za herní smyčku, načítání textur a ošetření vstupů. Její rozdělení do vícero `.cpp` souborů zabraňuje vzniku masivního nepřehledného "špagetového" kódu.
- **Raycaster (`raycaster.hpp`, `raycaster.cpp`)**: Zapouzdřená matematická logika pro výpočet vzdáleností parsek, texturování zdí, podlah a vykreslování spritů se z-bufferem.
- **Datové struktury (`Sprite.hpp`, `player.hpp`)**: Udržují stav jednotlivých entit (hráč, nepřátelé, předměty).

Tento přístup byl zvolen pro maximální čitelnost a oddělení "herní logiky" (model) od "vykreslování" (view).

## Zajímavé problémy a řešení
1. **Z-Buffer a vykreslování spritů**: Standardní raycasting snadno vykreslí zdi, ale dynamické řazení entit a nepřátel, aby se nepřekrývali navzájem, bylo poměrně komplexní. K vyřešení byl naprogramován 1D Z-Buffer a následné třídění (sort) objektů podle vzdálenosti.
2. **Přesné načasování animací zbraní**: Při rapidním střílení (jako u Chaingun) bylo obtížné synchronizovat animace se samotnými výstřely a zamezit "sekání" obrazu. Problém vyřešil přesný časovač pracující s hodnotou `deltaTime`, který správně inkrementuje `frameIndex`.
3. **Offsety ve spritových animacích (.PNG)**: Doomovské sprity zbraní používají rozdílná rozlišení u jednotlivých snímků a posuny (`grAb` offset chunk v PNG). K úspěšnému vyřešení bylo nutné naprogramovat vlastní detekci tohoto bloku přímo uvnitř PNG souborů během načítání přes `fread` a korekce následně promítnout do Raycasteru.

## Co mě potrápilo a co by šlo udělat lépe
Poměrně mě potrápila detekce kolizí s posuvnými dveřmi, protože bylo snadné, aby hráč "prošel" skrz dveře těsně před jejich uzavřením. 

Do budoucna by se engine dal znatelně vylepšit použitím Spatial Partitioning stromu (např. BSP), díky čemuž by hra dokázala renderovat mnohem komplikovanější víceúrovňové mapy místo jednoduché sítě (mřížky). Také by prospělo přesunout herní nastavení z pevného zakódování (hardcode) do externích konfiguračních `.json` nebo `.ini` souborů.
