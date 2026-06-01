#include "engine.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "map.hpp"
#include "font.hpp"

void Engine::drawRect(int startX, int startY, int width, int height, uint32_t color) {
    for (int y = startY; y < startY + height; y++) {
        for (int x = startX; x < startX + width; x++) {
            // Kontrola hranic obrazovky
            if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
                framebuffer[y * screenWidth + x] = color;
            }
        }
    }
}


void Engine::drawText(const std::string& text, int x, int y, uint32_t color, int scale) {
    int cursorX = x;

    for (char c : text) {
        // Kontrola, jestli znak vůbec máme ve fontu (od 32 do 127)
        if (c >= 32 && c <= 126) {
            int charIndex = c - 32;

            // Projdeme mřížku 8x8 pixelů pro dané písmeno
            for (int row = 0; row < 8; row++) {
                uint8_t rowData = font8x8[charIndex][row];
                
                for (int col = 0; col < 8; col++) {
                    // Pomocí bitových operací (masky) zjistíme, jestli je daný pixel zapnutý
                    if (rowData & (1 << (7 - col))) {
                        // Kreslíme čtvereček o velikosti "scale"
                        drawRect(cursorX + col * scale, y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        // Posuneme kurzor pro další písmeno (8 pixelů + 2 pixely mezera) * scale
        cursorX += (8 + 2) * scale; 
    }
}


void Engine::drawRectClipped(int startX, int startY, int width, int height, uint32_t color, int clipX, int clipY, int clipW, int clipH) {
    for (int y = startY; y < startY + height; y++) {
        for (int x = startX; x < startX + width; x++) {
            if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight && 
                x >= clipX && x < clipX + clipW && y >= clipY && y < clipY + clipH) {
                framebuffer[y * screenWidth + x] = color;
            }
        }
    }
}

void Engine::drawTextClipped(const std::string& text, int x, int y, int clipX, int clipY, int clipW, int clipH, uint32_t color, int scale) {
    int cursorX = x;

    for (char c : text) {
        if (c >= 32 && c <= 126) {
            int charIndex = c - 32;
            for (int row = 0; row < 8; row++) {
                uint8_t rowData = font8x8[charIndex][row];
                for (int col = 0; col < 8; col++) {
                    if (rowData & (1 << (7 - col))) {
                        drawRectClipped(cursorX + col * scale, y + row * scale, scale, scale, color, clipX, clipY, clipW, clipH);
                    }
                }
            }
        }
        cursorX += (8 + 2) * scale; 
    }
}


void Engine::drawMinimap() {
    int blockSize = 8;      // Velikost jednoho bloku mapy v pixelech
    int mapOffsetX = 10;    // Odsazení od levého okraje obrazovky
    int mapOffsetY = 10;    // Odsazení od horního okraje

    // Tmavé poloprůhledné pozadí minimapy
    for (int y = mapOffsetY - 5; y < mapOffsetY + MAP_HEIGHT * blockSize + 5; y++) {
        for (int x = mapOffsetX - 5; x < mapOffsetX + MAP_WIDTH * blockSize + 5; x++) {
            if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
                uint32_t bgPixel = framebuffer[y * screenWidth + x];
                framebuffer[y * screenWidth + x] = (bgPixel >> 1) & 0x7F7F7F7F; // Ztmaví pozadí
            }
        }
    }

    // 1. Vykreslení bloků mapy
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (worldMap[x][y] > 0) {
                // Zdi modře, dveře tyrkysově
                uint32_t color = (worldMap[x][y] == 4) ? 0xFF00FFFF : 0xFF0088FF;
                drawRect(mapOffsetX + x * blockSize, mapOffsetY + y * blockSize, blockSize - 1, blockSize - 1, color);
            }
        }
    }

    // 2. Vykreslení hráče
    int playerSize = 4;
    // Přepočet z herních souřadnic na pixely na obrazovce
    int playerPixelX = mapOffsetX + (int)(player.x * blockSize);
    int playerPixelY = mapOffsetY + (int)(player.y * blockSize);
    
    // Hráč bude zářivě zelený obdélníček
    drawRect(playerPixelX - playerSize / 2, playerPixelY - playerSize / 2, playerSize, playerSize, 0xFF00FF00);

    // 3. Směrový ukazatel (kam se hráč dívá)
    int dirDotX = playerPixelX + (int)(player.dirX * blockSize);
    int dirDotY = playerPixelY + (int)(player.dirY * blockSize);
    drawRect(dirDotX - 1, dirDotY - 1, 3, 3, 0xFFFF0000); // Červená tečka pro směr pohledu
}


void Engine::drawMenu() {
    // 1. Vykreslení menu pozadí, pokud existuje
    if (!menuBgTexture.empty() && menuBgTexWidth > 0 && menuBgTexHeight > 0) {
        for (int y = 0; y < screenHeight; y++) {
            for (int x = 0; x < screenWidth; x++) {
                int srcX = (x * menuBgTexWidth) / screenWidth;
                int srcY = (y * menuBgTexHeight) / screenHeight;
                uint32_t texColor = menuBgTexture[srcY * menuBgTexWidth + srcX];
                if ((texColor & 0xFF000000) != 0) { // Pokud není plně průhledný
                    framebuffer[y * screenWidth + x] = texColor;
                }
            }
        }
    } else {
        // Fallback: Ztmavíme celou obrazovku
        for (uint32_t& pixel : framebuffer) {
            pixel = (pixel >> 1) & 0x7F7F7F7F; 
        }
    }

    // Nápis MENU nahoře
    drawText("MAIN MENU", screenWidth / 2 - 90, screenHeight / 2 - 120, 0xFFFFFFFF, 4);
    drawText("HIGH SCORE: " + std::to_string(highScore), screenWidth / 2 - 90, screenHeight / 2 - 80, 0xFF00FF00, 2);

    // 2. Definice rozměrů pro tlačítka
    int btnW = 240; // Trochu rozšíříme kvůli textu
    int btnH = 40;
    int spacing = 50;
    int startX = screenWidth / 2 - btnW / 2;
    int startY = screenHeight / 2 - 40; 

    // Názvy našich tlačítek
    std::string buttonTexts[3] = {"PLAY GAME", "SETTINGS", "QUIT"};

    // 3. Vykreslení 3 tlačítek
    for (int i = 0; i < 3; i++) {
        // Pokud je tlačítko vybrané, uděláme ho světlejší
        uint32_t color = (menuSelection == i) ? 0xFF666666 : 0xFF333333;
        
        // Vykreslení těla tlačítka
        drawRect(startX, startY + (i * spacing), btnW, btnH, color);

        // Vykreslení textu na tlačítku
        // (Vypočítáme si drobný posun, aby byl text zhruba vycentrovaný)
        uint32_t textColor = (menuSelection == i) ? 0xFFFFFF00 : 0xFFDDDDDD; // Žlutý text při najetí
        drawText(buttonTexts[i], startX + 20, startY + (i * spacing) + 12, textColor, 2);
    }
}


void Engine::render() {
    // =========================================================
    // 1. ZÁKLADNÍ VRSTVA: Vykreslení 3D světa
    // =========================================================
    // Vykreslujeme vždy, i když jsme v menu, aby hra zůstala "zamrzlá" na pozadí
    raycaster.render(framebuffer, screenWidth, screenHeight, player, textures, zBuffer, doorOffsets);
    raycaster.renderSprites(framebuffer, screenWidth, screenHeight, player, sprites, zBuffer, enemyTypes, projectileTypes, itemTypes);

    // 3D text rendering moved below the weapon rendering

    // =========================================================
    // 2. UI VRSTVA: Rozhodnutí podle stavu hry
    // =========================================================
    if (currentState == GameState::PLAYING) {
        
        // --- A. MINIMAPA ---
        drawMinimap(); 

        // --- B. VYKRESLENÍ ZBRANĚ Z POHLEDU PRVNÍ OSOBY ---
        int bobOffset = isMoving ? (int)(std::abs(std::sin(weaponBobTime * 8.0)) * 20.0) : 0;
        int recoilOffset = isShooting ? 40 : 0; // Zbraň se posune dolů při výstřelu

        if (!weaponFrames.empty() && weaponFrameIndex < weaponFrames.size()) {
            auto& frame = weaponFrames[weaponFrameIndex];
            
            if (frame.w > 0 && frame.h > 0) {
                // Doom sprity jsou dělané na 320x200 rozlišení. My máme screenWidth.
                double scale = (double)screenWidth / 320.0; 
                
                int scaledW = frame.w * scale;
                int scaledH = frame.h * scale;
                
                // Zbraň je vždy na spodním okraji a uprostřed
                int startX = (screenWidth / 2) - (scaledW / 2);
                int startY = screenHeight - scaledH + bobOffset + recoilOffset;
                
                for (int y = 0; y < scaledH; y++) {
                    for (int x = 0; x < scaledW; x++) {
                        int drawX = startX + x;
                        int drawY = startY + y;
                        if (drawX >= 0 && drawX < screenWidth && drawY >= 0 && drawY < screenHeight) {
                            int srcX = (x * frame.w) / scaledW;
                            int srcY = (y * frame.h) / scaledH;
                            uint32_t color = frame.pixels[srcY * frame.w + srcX];
                            // Vykreslíme pixel, pouze pokud jeho Alpha kanál není 0
                            if ((color & 0xFF000000) != 0) {
                                framebuffer[drawY * screenWidth + drawX] = color;
                            }
                        }
                    }
                }
            }
        }


        // --- C. VYKRESLENÍ HUDu ---
        drawRect(20, screenHeight - 40, 200, 20, 0xFFFF0000); 
        int hpWidth = (player.hp > 0) ? (player.hp * 2) : 0;
        drawRect(20, screenHeight - 40, hpWidth, 20, 0xFF00FF00); 
        drawText("HP: " + std::to_string(player.hp), 25, screenHeight - 38, 0xFFFFFFFF, 2);
        
        int wpn = player.currentWeapon;
        if (wpn >= 0 && wpn < 5 && player.ammo[wpn] != -1) {
            std::string ammoStr = "AMMO: " + std::to_string(player.ammo[wpn]);
            drawText(ammoStr, 250, screenHeight - 38, 0xFFFFFF00, 2); // Yellow color for ammo
        }

        std::string scoreStr = "SCORE: " + std::to_string(playerScore);
        drawText(scoreStr, screenWidth - (scoreStr.length() * 10 * 2) - 20, screenHeight - 38, 0xFF00FFFF, 2);

        // Zčervenání obrazovky při zranění
        if (playerDamageTimer > 0) {
            for (uint32_t& pixel : framebuffer) {
                uint32_t r = (pixel >> 16) & 0xFF;
                r = std::min(255u, r + 50u);
                pixel = (pixel & 0xFF00FFFF) | (r << 16);
            }
        }
    } 
    else if (currentState == GameState::MENU) {
        
        // --- C. HERNÍ MENU ---
        // Vykreslí poloprůhledné pozadí a 3 velká tlačítka přes hru
        drawMenu();
    }
    else if (currentState == GameState::SETTINGS) {
        drawSettings();
    }
    else if (currentState == GameState::GAME_OVER) {
        // --- D. DEATH SCREEN ---
        if (!deathTexture.empty() && deathTexWidth > 0 && deathTexHeight > 0) {
            for (int y = 0; y < screenHeight; y++) {
                for (int x = 0; x < screenWidth; x++) {
                    int srcX = (x * deathTexWidth) / screenWidth;
                    int srcY = (y * deathTexHeight) / screenHeight;
                    framebuffer[y * screenWidth + x] = deathTexture[srcY * deathTexWidth + srcX];
                }
            }
        } else {
            for (int i = 0; i < screenWidth * screenHeight; i++) framebuffer[i] = 0xFF440000;
        }

        drawText("YOU DIED", screenWidth / 2 - 80, screenHeight / 2 - 50, 0xFFFF0000, 4);
        drawText("PRESS ANY KEY TO RETURN", screenWidth / 2 - 110, screenHeight / 2 + 50, 0xFFFFFFFF, 2);
    }

    // =========================================================
    // 3. HARDWAROVÁ AKCELERACE: Odeslání na obrazovku
    // =========================================================
    // Překopírování našeho pole z RAM na Grafickou kartu (GPU)
    SDL_UpdateTexture(frameTexture, nullptr, framebuffer.data(), screenWidth * sizeof(uint32_t));
    
    // Vyčištění starého snímku a vykreslení nového
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, frameTexture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Engine::drawSettings() {
    // Tmavší překryv
    for (uint32_t& pixel : framebuffer) {
        pixel = (pixel >> 1) & 0x7F7F7F7F; 
    }

    drawText("SETTINGS", screenWidth / 2 - 80, screenHeight / 2 - 140, 0xFFFFFFFF, 4);

    int btnW = 400;
    int btnH = 40;
    int spacing = 50;
    int startX = screenWidth / 2 - btnW / 2;
    int startY = screenHeight / 2 - 60;

    std::string options[4];
    
    // Sensitivity
    char sensStr[32];
    snprintf(sensStr, sizeof(sensStr), "%.2f", mouseSensitivity);
    options[0] = "< MOUSE SENSITIVITY: " + std::string(sensStr) + " >";

    // Resolution
    options[1] = "< RESOLUTION: " + std::to_string(screenWidth) + "x" + std::to_string(screenHeight) + " >";

    // Volume
    options[2] = "< VOLUME: " + std::to_string(int(globalVolume * 100)) + "% >";

    // Back
    options[3] = "BACK TO MENU";

    for (int i = 0; i < 4; i++) {
        uint32_t color = (settingsSelection == i) ? 0xFF666666 : 0xFF333333;
        drawRect(startX, startY + (i * spacing), btnW, btnH, color);

        uint32_t textColor = (settingsSelection == i) ? 0xFFFFFF00 : 0xFFDDDDDD;
        
        int textLength = options[i].length() * 10 * 2; // (8 px znak + 2 px mezera) * scale
        int maxTextWidth = btnW - 40; // Odsazení 20 z obou stran
        int textX = startX + 20;
        int textY = startY + (i * spacing) + 12;

        if (textLength > maxTextWidth && settingsSelection == i) {
            // Posun textu založený na čase
            int shift = (SDL_GetTicks() / 15) % (textLength + 40);
            
            drawTextClipped(options[i], textX - shift, textY, textX, startY + (i * spacing), maxTextWidth, btnH, textColor, 2);
            
            // Pro efekt smyčky vykreslíme text znovu za ním, pokud ujede dostatečně doleva
            if (shift > textLength - maxTextWidth) {
                drawTextClipped(options[i], textX - shift + textLength + 40, textY, textX, startY + (i * spacing), maxTextWidth, btnH, textColor, 2);
            }
        } else if (textLength > maxTextWidth) {
            // Pokud není vybrané, ale přetéká, tak to staticky ořízneme
            drawTextClipped(options[i], textX, textY, textX, startY + (i * spacing), maxTextWidth, btnH, textColor, 2);
        } else {
            // Normální vykreslení, pokud se text vejde
            drawText(options[i], textX, textY, textColor, 2);
        }
    }
}
