#pragma once

struct Player {
    double x, y;           // Pozice
    double dirX, dirY;     // Směr pohledu
    double planeX, planeY; // Kamerová rovina (FOV)
    double z = 0.0;        // Výška (skákání)
    double vz = 0.0;       // Vertikální rychlost
    int hp = 100;          // Zdraví hráče
    int maxHp = 100;
    
    int currentWeapon = 0;
    bool hasWeapon[5] = {true, false, false, false, false};
    int ammo[5] = { -1, 50, 20, 10, 0 }; // -1 = pěst/základní zbraň nepotřebuje náboje
};