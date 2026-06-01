#include "engine.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "map.hpp"

// Konstanty
const double PI = 3.1415926535;

bool isShooting = false;
double shootTimer = 0.0;
double weaponAnimTimer = 0.0;
int weaponFrameIndex = 0;

bool isMoving = false;
double weaponBobTime = 0.0;
double stepTimer = 0.0;

uint32_t lastTime = 0;
double playerDamageTimer = 0.0;
int playerScore = 0;
double enemySpawnTimer = 4.0;
double gameOverTimer = 0.0;

void Engine::handleEvents(double deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isRunning = false;
        }

        // --- OVLÁDÁNÍ VE HŘE ---
        if (currentState == GameState::PLAYING) {
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_E) {
                // Interakce s dveřmi
                int targetX = int(player.x + player.dirX);
                int targetY = int(player.y + player.dirY);
                if (targetX >= 0 && targetX < MAP_WIDTH && targetY >= 0 && targetY < MAP_HEIGHT) {
                    if (worldMap[targetX][targetY] == 4) {
                        if (doorStates[targetX][targetY] == 0) {
                            doorStates[targetX][targetY] = 1; // Začne otevírat
                            if (doorAudioStream && doorAudioData) {
                                SDL_PutAudioStreamData(doorAudioStream, doorAudioData, doorAudioSamples * sizeof(short));
                                SDL_FlushAudioStream(doorAudioStream);
                            }
                        } else if (doorStates[targetX][targetY] == 2) {
                            doorStates[targetX][targetY] = 3; // Začne zavírat
                            if (doorAudioStream && doorAudioData) {
                                SDL_PutAudioStreamData(doorAudioStream, doorAudioData, doorAudioSamples * sizeof(short));
                                SDL_FlushAudioStream(doorAudioStream);
                            }
                        }
                    }
                }
            }
            
            // ESCAPE PRO NÁVRAT DO MENU BĚHEM HRY
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                currentState = GameState::MENU;
                SDL_SetWindowRelativeMouseMode(window, false);
            }
        } // Konec if (PLAYING)

        // OVLÁDÁNÍ MENU POMOCÍ KLÁVESNICE
        if (currentState == GameState::MENU && event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
                if (event.key.key == SDLK_W || event.key.key == SDLK_UP) menuSelection--;
                if (event.key.key == SDLK_S || event.key.key == SDLK_DOWN) menuSelection++;
                
                // Rotace výběru (0 = Hrát, 1 = Fullscreen, 2 = Konec)
                if (menuSelection < 0) menuSelection = 2;
                if (menuSelection > 2) menuSelection = 0;

                // Potvrzení klávesou ENTER
                if (event.key.key == SDLK_RETURN) {
                    if (menuSelection == 0) { 
                        currentState = GameState::PLAYING;
                        SDL_SetWindowRelativeMouseMode(window, true);
                    }
                    else if (menuSelection == 1) { 
                        currentState = GameState::SETTINGS;
                        settingsSelection = 0;
                    }
                    else if (menuSelection == 2) {
                        isRunning = false; 
                    }
                }
            }

        // --- OVLÁDÁNÍ MYŠÍ V MENU ---
        if (currentState == GameState::MENU) {
            // Rozměry tlačítek (musí odpovídat těm v metodě drawMenu)
            int btnW = 200; 
            int btnH = 40; 
            int spacing = 50;
            int startX = screenWidth / 2 - btnW / 2;
            int startY = screenHeight / 2 - 60;

            // Highlight (přejetí myší)
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                int mx = event.motion.x;
                int my = event.motion.y;

                for (int i = 0; i < 3; i++) {
                    int boxY = startY + (i * spacing);
                    if (mx >= startX && mx <= startX + btnW && my >= boxY && my <= boxY + btnH) {
                        menuSelection = i; 
                    }
                }
            }

            // Kliknutí (potvrzení)
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;

                for (int i = 0; i < 3; i++) {
                    int boxY = startY + (i * spacing);
                    if (mx >= startX && mx <= startX + btnW && my >= boxY && my <= boxY + btnH) {
                        if (i == 0) { 
                            currentState = GameState::PLAYING;
                            SDL_SetWindowRelativeMouseMode(window, true);
                        }
                        else if (i == 1) { 
                            currentState = GameState::SETTINGS;
                            settingsSelection = 0;
                        }
                        else if (i == 2) {
                            isRunning = false; 
                        }
                    }
                }
            }
        }

        // --- OVLÁDÁNÍ MYŠÍ VE HŘE (Rotace kamery) ---
        if (currentState == GameState::PLAYING && event.type == SDL_EVENT_MOUSE_MOTION) {
            // motion.xrel je hodnota o kolik se myš pohla do strany
            double rotSpeed = event.motion.xrel * -mouseSensitivity * deltaTime;
            
            // Rotační matice pro směr pohledu a rovinu kamery
            double oldDirX = player.dirX;
            player.dirX = player.dirX * std::cos(rotSpeed) - player.dirY * std::sin(rotSpeed);
            player.dirY = oldDirX * std::sin(rotSpeed) + player.dirY * std::cos(rotSpeed);
            
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * std::cos(rotSpeed) - player.planeY * std::sin(rotSpeed);
            player.planeY = oldPlaneX * std::sin(rotSpeed) + player.planeY * std::cos(rotSpeed);
        }

        // --- PŘEPÍNÁNÍ ZBRANÍ ---
        if (currentState == GameState::PLAYING && event.type == SDL_EVENT_KEY_DOWN) {
            auto swp = [&](int id) {
                if (id < weapons.size() && player.hasWeapon[id]) {
                    player.currentWeapon = id;
                }
            };
            if (event.key.scancode == SDL_SCANCODE_1) swp(0);
            if (event.key.scancode == SDL_SCANCODE_2) swp(1);
            if (event.key.scancode == SDL_SCANCODE_3) swp(2);
            if (event.key.scancode == SDL_SCANCODE_4) swp(3);
        }

        // --- STŘELBA ---
        if (currentState == GameState::PLAYING && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (player.currentWeapon != 1) { // Chaingun se řeší v updatePlayer
                fireWeapon();
            }
        }

        
        if (currentState == GameState::GAME_OVER) {
            if (gameOverTimer <= 0 && (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_KEY_DOWN)) {
                resetGame();
                currentState = GameState::MENU;
                SDL_SetWindowRelativeMouseMode(window, false);
            }
        }

        if (currentState == GameState::SETTINGS) {
            handleSettingsInput(event);
        }
    }
}

void Engine::updatePlayer(double deltaTime) {
        int wpn = player.currentWeapon;
        if (wpn == 1 && !isShooting) { // Kontinuální střelba pro Chaingun
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) {
                fireWeapon();
            }
        }

        if (isShooting) {
            shootTimer -= deltaTime;
            if (shootTimer <= 0) {
                isShooting = false;
                shootTimer = 0.0;
                weaponFrameIndex = 0;
            } else {
                weaponAnimTimer += deltaTime;
                while (weaponAnimTimer >= WEAPON_ANIM_SPEED) {
                    weaponAnimTimer -= WEAPON_ANIM_SPEED;
                    if (wpn >= 0 && wpn < weapons.size() && !weapons[wpn].shootFrames.empty()) {
                        weaponFrameIndex = (weaponFrameIndex + 1) % weapons[wpn].shootFrames.size();
                    }
                }
            }
        } else {
            weaponAnimTimer += deltaTime;
            while (weaponAnimTimer >= WEAPON_ANIM_SPEED * 2.0) { // Idle může být pomalejší
                weaponAnimTimer -= WEAPON_ANIM_SPEED * 2.0;
                if (wpn >= 0 && wpn < weapons.size() && !weapons[wpn].idleFrames.empty()) {
                    weaponFrameIndex = (weaponFrameIndex + 1) % weapons[wpn].idleFrames.size();
                }
            }
        }

        const bool* keystate = SDL_GetKeyboardState(nullptr);
        double moveSpeed = 5.0 * deltaTime; // 5 bloků za sekundu
        
        // Zvětšíme buffer pro kolize (Wall sliding)
        double bufferX = (player.dirX > 0 ? 0.2 : -0.2);
        double bufferY = (player.dirY > 0 ? 0.2 : -0.2);
        
        isMoving = false; // Reset detekce pohybu pro zbraň

        // Krok vpřed (W)
        if (keystate[SDL_SCANCODE_W]) {
            isMoving = true;
            if (isWalkable(int(player.x + player.dirX * moveSpeed + bufferX), int(player.y))) player.x += player.dirX * moveSpeed;
            if (isWalkable(int(player.x), int(player.y + player.dirY * moveSpeed + bufferY))) player.y += player.dirY * moveSpeed;
        }
        // Krok vzad (S)
        if (keystate[SDL_SCANCODE_S]) {
            isMoving = true;
            if (isWalkable(int(player.x - player.dirX * moveSpeed - bufferX), int(player.y))) player.x -= player.dirX * moveSpeed;
            if (isWalkable(int(player.x), int(player.y - player.dirY * moveSpeed - bufferY))) player.y -= player.dirY * moveSpeed;
        }
        // Úkrok doleva (A)
        if (keystate[SDL_SCANCODE_A]) {
            isMoving = true;
            double strafeX = -player.dirY;
            double strafeY = player.dirX;
            double sBufferX = (strafeX > 0) ? 0.2 : -0.2;
            double sBufferY = (strafeY > 0) ? 0.2 : -0.2;
            if (isWalkable(int(player.x + strafeX * moveSpeed + sBufferX), int(player.y))) player.x += strafeX * moveSpeed;
            if (isWalkable(int(player.x), int(player.y + strafeY * moveSpeed + sBufferY))) player.y += strafeY * moveSpeed;
        }
        // Úkrok doprava (D)
        if (keystate[SDL_SCANCODE_D]) {
            isMoving = true;
            double strafeX = player.dirY;
            double strafeY = -player.dirX;
            double sBufferX = (strafeX > 0) ? 0.2 : -0.2;
            double sBufferY = (strafeY > 0) ? 0.2 : -0.2;
            if (isWalkable(int(player.x + strafeX * moveSpeed + sBufferX), int(player.y))) player.x += strafeX * moveSpeed;
            if (isWalkable(int(player.x), int(player.y + strafeY * moveSpeed + sBufferY))) player.y += strafeY * moveSpeed;
        }

        // Skákání
        if (keystate[SDL_SCANCODE_SPACE] && player.z == 0) {
            player.vz = 300.0;
        }

        // Gravitace a vertikální pohyb
        player.z += player.vz * deltaTime;
        if (player.z > 0) {
            player.vz -= 900.0 * deltaTime;
        } else {
            player.z = 0;
            player.vz = 0;
        }

}

void Engine::updateEntities(double deltaTime) {
        std::vector<Sprite> newProjectiles;
        for (auto& sprite : sprites) {
            // Animace sprite
            sprite.animTimer += deltaTime;
            if (sprite.animTimer >= 0.15) {
                sprite.animTimer -= 0.15;
                int maxFrames = 1;
                if (sprite.isProjectile) {
                    if (sprite.type >= 0 && sprite.type < projectileTypes.size()) maxFrames = projectileTypes[sprite.type].size();
                } else if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                    if (sprite.state == 0) maxFrames = enemyTypes[sprite.type].deathFrames.size();
                    else if (sprite.state == 1) maxFrames = enemyTypes[sprite.type].idleFrames.size();
                    else if (sprite.state == 2) maxFrames = enemyTypes[sprite.type].painFrames.size();
                }
                if (maxFrames <= 0) maxFrames = 1;
                
                sprite.frameIndex++;
                if (sprite.state == 0) {
                    if (sprite.frameIndex >= maxFrames) sprite.frameIndex = maxFrames - 1;
                } else if (sprite.state == 2) {
                    if (sprite.frameIndex >= maxFrames) {
                        sprite.state = 1;
                        sprite.frameIndex = 0;
                    }
                } else {
                    sprite.frameIndex %= maxFrames;
                }
            }
            
            if (sprite.isItem) {
                if (sprite.state == 1) { // 1 = visible
                    double pDist = std::sqrt((player.x - sprite.x)*(player.x - sprite.x) + (player.y - sprite.y)*(player.y - sprite.y));
                    if (pDist < 0.6) {
                        if (sprite.type >= 0 && sprite.type < itemTypes.size()) {
                            auto& itm = itemTypes[sprite.type];
                            bool pickedUp = false;
                            if (itm.type == ItemType::MEDKIT) {
                                if (player.hp < player.maxHp) {
                                    player.hp += itm.amount;
                                    if (player.hp > player.maxHp) player.hp = player.maxHp;
                                    pickedUp = true;
                                }
                            } else if (itm.type == ItemType::AMMO) {
                                int wpn = player.currentWeapon;
                                if (player.ammo[wpn] != -1) {
                                    player.ammo[wpn] += itm.amount;
                                    pickedUp = true;
                                }
                            } else if (itm.type == ItemType::WEAPON) {
                                if (itm.weaponId >= 0 && itm.weaponId < 5) {
                                    player.hasWeapon[itm.weaponId] = true;
                                    player.ammo[itm.weaponId] += itm.amount;
                                    pickedUp = true;
                                }
                            }
                            if (pickedUp) {
                                sprite.state = -1; // hide it immediately
                                sprite.deadTimer = 10.0;
                            }
                        }
                    }
                } else if (sprite.state == -1) {
                    sprite.deadTimer -= deltaTime;
                    if (sprite.deadTimer <= 0) {
                        sprite.state = 1; // Respawn
                        sprite.deadTimer = 0;
                    }
                }
                continue;
            }

            if (sprite.isProjectile) {
                if (sprite.state == 1) {
                    sprite.lifeTime -= deltaTime;
                    if (sprite.lifeTime <= 0) {
                        sprite.state = 0;
                    } else {
                        double moveSpeed = 8.0 * deltaTime;
                        if (!isWalkable(int(sprite.x + sprite.dx * moveSpeed), int(sprite.y + sprite.dy * moveSpeed))) {
                            sprite.state = 0;
                            continue;
                        }
                        sprite.x += sprite.dx * moveSpeed;
                        sprite.y += sprite.dy * moveSpeed;
                        
                        double pDist = std::sqrt((player.x - sprite.x)*(player.x - sprite.x) + (player.y - sprite.y)*(player.y - sprite.y));
                        if (pDist < 0.5) {
                            sprite.state = 0;
                            player.hp -= 15;
                            playerDamageTimer = 0.2;
                            playSound(playerPainData, playerPainSamples, playerPainRate, playerPainChannels);
                        }
                    }
                }
                continue;
            }

            if (sprite.state == 0) {
                sprite.deadTimer -= deltaTime;
                if (sprite.deadTimer <= 0) {
                    sprite.state = -1; // Completly disappear
                }
                continue;
            }
            if (sprite.state == -1) continue;

            if (sprite.damageTimer > 0) sprite.damageTimer -= deltaTime;
            if (sprite.attackCooldown > 0) sprite.attackCooldown -= deltaTime;

            double dx = player.x - sprite.x;
            double dy = player.y - sprite.y;
            double dist = std::sqrt(dx*dx + dy*dy);

            // Line of sight check
            bool canSeePlayer = true;
            int steps = std::max(1, (int)(dist * 10));
            for(int i=0; i<=steps; i++) {
                double cx = sprite.x + dx * ((double)i / steps);
                double cy = sprite.y + dy * ((double)i / steps);
                if (!isWalkable((int)cx, (int)cy)) {
                    canSeePlayer = false;
                    break;
                }
            }

            if (canSeePlayer && dist < 8.0 && dist > 0.6) {
                // Nepřítel pronásleduje hráče
                double speed = 1.5 * deltaTime;
                double moveX = (dx / dist) * speed;
                double moveY = (dy / dist) * speed;
                
                double sBufferX = (moveX > 0) ? 0.2 : -0.2;
                double sBufferY = (moveY > 0) ? 0.2 : -0.2;

                // Kolize pro nepřítele, aby nelezl přes zdi
                if (isWalkable(int(sprite.x + moveX + sBufferX), int(sprite.y))) {
                    sprite.x += moveX;
                }
                if (isWalkable(int(sprite.x), int(sprite.y + moveY + sBufferY))) {
                    sprite.y += moveY;
                }
                
                // Střelba projektilů
                if (sprite.attackCooldown <= 0 && (rand() % 100) < 2) {
                    sprite.attackCooldown = 2.0; // 2 sec cooldown
                    if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                        playSound(enemyTypes[sprite.type].soundAttack, enemyTypes[sprite.type].soundAttackSamples, enemyTypes[sprite.type].soundAttackRate, enemyTypes[sprite.type].soundAttackChannels);
                    }
                    Sprite proj;
                    proj.x = sprite.x;
                    proj.y = sprite.y;
                    proj.isProjectile = true;
                    proj.type = sprite.type;
                    proj.hp = 1;
                    proj.state = 1;
                    proj.dx = dx / dist;
                    proj.dy = dy / dist;
                    proj.lifeTime = 5.0;
                    newProjectiles.push_back(proj);
                }
            } else if (canSeePlayer && dist <= 0.6) {
                // Nepřítel útočí na blízko
                if (sprite.attackCooldown <= 0) {
                    sprite.attackCooldown = 1.5;
                    player.hp -= 5;
                    playerDamageTimer = 0.2;
                    playSound(playerPainData, playerPainSamples, playerPainRate, playerPainChannels);
                    if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                        playSound(enemyTypes[sprite.type].soundAttack, enemyTypes[sprite.type].soundAttackSamples, enemyTypes[sprite.type].soundAttackRate, enemyTypes[sprite.type].soundAttackChannels);
                    }
                }
            }
        }
        for (const auto& np : newProjectiles) {
            sprites.push_back(np);
        }


        if (playerDamageTimer > 0) playerDamageTimer -= deltaTime;
        
        if (player.hp <= 0) {
            player.hp = 0;
            if (currentState != GameState::GAME_OVER) {
                currentState = GameState::GAME_OVER;
                if (playerScore > highScore) highScore = playerScore;
                saveHighScore(); 
                gameOverTimer = 1.5; // Ochrana před okamžitým přeskočením (1.5 sekundy)
            }
        }

}

void Engine::updateDoors(double deltaTime) {
        if (isMoving) {
            weaponBobTime += deltaTime;
            stepTimer -= deltaTime;
            if (stepTimer <= 0.0) {
                if (stepAudioStream && stepAudioData) {
                    SDL_PutAudioStreamData(stepAudioStream, stepAudioData, stepAudioSamples * sizeof(short));
                    SDL_FlushAudioStream(stepAudioStream);
                }
                stepTimer = 0.5; // Krok každých 0.5s
            }
        } else {
            weaponBobTime = 0; // Plynulý návrat do klidu
            stepTimer = 0.0;
            if (wasMoving && stepAudioStream) {
                SDL_ClearAudioStream(stepAudioStream);
            }
        }
        wasMoving = isMoving;

        // --- AKTUALIZACE DVEŘÍ ---
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                if (doorStates[x][y] == 1) { // Otevírá se
                    doorOffsets[x][y] += 1.5 * deltaTime;
                    if (doorOffsets[x][y] >= 1.0) {
                        doorOffsets[x][y] = 1.0;
                        doorStates[x][y] = 2; // Plně otevřeno (průchozí)
                    }
                } else if (doorStates[x][y] == 3) { // Zavírá se
                    // Zavíráme jen, pokud v nich nikdo nestojí (jednoduchá kontrola hráče)
                    if (int(player.x) != x || int(player.y) != y) {
                        doorOffsets[x][y] -= 1.5 * deltaTime;
                        if (doorOffsets[x][y] <= 0.0) {
                            doorOffsets[x][y] = 0.0;
                            doorStates[x][y] = 0; // Plně zavřeno
                            if (doorAudioStream && doorAudioData) {
                                SDL_PutAudioStreamData(doorAudioStream, doorAudioData, doorAudioSamples * sizeof(short));
                                SDL_FlushAudioStream(doorAudioStream);
                            }
                        }
                    }
                }
            }
        }
}

void Engine::spawnEnemies(double deltaTime) {
        // --- ARCADE RESPAWN LOGIC ---
        if (player.hp > 0 && currentState == GameState::PLAYING) {
            enemySpawnTimer -= deltaTime;
            if (enemySpawnTimer <= 0.0) {
                // Calculate spawn interval based on score
                double interval = std::max(1.0, 4.0 - (playerScore / 2000.0));
                enemySpawnTimer = interval;
                
                // Find a spawn position far enough from player
                int sX = 0, sY = 0;
                bool found = false;
                for (int tries = 0; tries < 50; tries++) {
                    sX = rand() % MAP_WIDTH;
                    sY = rand() % MAP_HEIGHT;
                    if (isWalkable(sX, sY)) {
                        double dx = sX - player.x;
                        double dy = sY - player.y;
                        if (dx*dx + dy*dy > 64.0) { // At least 8 blocks away
                            found = true;
                            break;
                        }
                    }
                }
                
                if (found) {
                    if (enemyTypes.empty()) {
                        enemySpawnTimer = 4.0; // Zkus to znovu později
                    } else {
                        // Find a dead sprite or add new one
                        int spawnType = rand() % enemyTypes.size();
                        bool reused = false;
                    for (auto& s : sprites) {
                        if (s.state == -1 && !s.isProjectile && !s.isItem) {
                            s.x = sX + 0.5;
                            s.y = sY + 0.5;
                            s.spawnX = sX + 0.5;
                            s.spawnY = sY + 0.5;
                            s.type = spawnType;
                            s.hp = enemyTypes[spawnType].maxHp;
                            s.state = 1;
                            s.animTimer = 0;
                            s.frameIndex = 0;
                            s.damageTimer = 0;
                            reused = true;
                            break;
                        }
                    }
                        if (!reused && sprites.size() < 100) {
                            // Create new
                            sprites.push_back({sX + 0.5, sY + 0.5, 0, spawnType, enemyTypes[spawnType].maxHp, 1, 0.0, sX + 0.5, sY + 0.5, 0.0, 0.0, 0, 0.0, false});
                        }
                    }
                }
            }
        }
}

void Engine::update(double deltaTime) {
    handleEvents(deltaTime);

    if (currentState == GameState::PLAYING) {
        updatePlayer(deltaTime);
        updateEntities(deltaTime);
        updateDoors(deltaTime);
        spawnEnemies(deltaTime);
    }


    // Časovač game over (musí běžet nezávisle na PLAYING stavu)
    if (gameOverTimer > 0) gameOverTimer -= deltaTime;
}

void Engine::handleSettingsInput(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
        if (event.key.key == SDLK_ESCAPE) {
            saveSettings();
            currentState = GameState::MENU;
        }
        else if (event.key.key == SDLK_UP || event.key.key == SDLK_W) {
            settingsSelection--;
            if (settingsSelection < 0) settingsSelection = 3;
        }
        else if (event.key.key == SDLK_DOWN || event.key.key == SDLK_S) {
            settingsSelection++;
            if (settingsSelection > 3) settingsSelection = 0;
        }
        else if (event.key.key == SDLK_LEFT || event.key.key == SDLK_A) {
            if (settingsSelection == 0) mouseSensitivity = std::max(0.05, mouseSensitivity - 0.05);
            else if (settingsSelection == 1) changeResolution(800, 600);
            else if (settingsSelection == 2) setGlobalVolume(globalVolume - 0.1f);
        }
        else if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_D) {
            if (settingsSelection == 0) mouseSensitivity = std::min(1.0, mouseSensitivity + 0.05);
            else if (settingsSelection == 1) changeResolution(1920, 1080);
            else if (settingsSelection == 2) setGlobalVolume(globalVolume + 0.1f);
        }
        else if (event.key.key == SDLK_RETURN) {
            if (settingsSelection == 3) {
                saveSettings();
                currentState = GameState::MENU;
            }
        }
    }
}

bool Engine::isWalkable(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    if (worldMap[x][y] == 0) return true;
    if (worldMap[x][y] == 4 && doorStates[x][y] == 2) return true; // Open door
    return false;
}


void Engine::fireWeapon() {
    int wpn = player.currentWeapon;
    if (!isShooting && player.hp > 0 && (player.ammo[wpn] > 0 || player.ammo[wpn] == -1)) {
        if (player.ammo[wpn] > 0) {
            player.ammo[wpn]--;
            if (wpn == 2) player.ammo[wpn]--; // Coachgun bere 2
            if (player.ammo[wpn] < 0) player.ammo[wpn] = 0;
        }
        isShooting = true;
        
        double audioDuration = 0.25;
        if (weaponAudioRate[wpn] > 0 && weaponAudioChannels[wpn] > 0) {
            audioDuration = (double)(weaponAudioSamples[wpn] / weaponAudioChannels[wpn]) / weaponAudioRate[wpn];
        }
        shootTimer = audioDuration;
        if (wpn == 1) shootTimer = 0.1; // Chaingun rapid fire
        if ((wpn == 0 || wpn == 2) && !weapons[wpn].shootFrames.empty()) {
            shootTimer = weapons[wpn].shootFrames.size() * WEAPON_ANIM_SPEED; // Delší cooldown pro reload
        }
        
        weaponFrameIndex = 0;
        weaponAnimTimer = 0.0;
        
        if (weaponAudioData[wpn]) {
            playSound(weaponAudioData[wpn], weaponAudioSamples[wpn], weaponAudioRate[wpn], weaponAudioChannels[wpn]);
        }
        
        // Hitscan
        for (auto& sprite : sprites) {
            if (sprite.state <= 0) continue;
            if (sprite.isProjectile) continue;
            if (sprite.isItem) continue;
            
            double spriteX = sprite.x - player.x;
            double spriteY = sprite.y - player.y;
            double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
            double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
            double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);
            
            if (transformY > 0) {
                int spriteScreenX = int((screenWidth / 2) * (1 + transformX / transformY));
                int spriteWidth = std::abs(int(screenHeight / transformY));
                if (screenWidth / 2 > spriteScreenX - spriteWidth / 2 && screenWidth / 2 < spriteScreenX + spriteWidth / 2) {
                    if (transformY < 15.0) {
                        bool hitWall = false;
                        double dxLine = sprite.x - player.x;
                        double dyLine = sprite.y - player.y;
                        double dist = std::sqrt(dxLine*dxLine + dyLine*dyLine);
                        int steps = std::max(1, (int)(dist * 10));
                        for (int i = 0; i <= steps; i++) {
                            double cx = player.x + dxLine * ((double)i / steps);
                            double cy = player.y + dyLine * ((double)i / steps);
                            if (!isWalkable((int)cx, (int)cy)) {
                                hitWall = true;
                                break;
                            }
                        }
                        
                        if (!hitWall) {
                            int damage = 35;
                            if (wpn == 0) damage = 15; // Pistole
                            else if (wpn == 1) damage = 10; // Chaingun
                            else if (wpn == 2) damage = std::max(5, 100 - (int)(dist * 15)); // Coachgun dynamický damage
                            else if (wpn == 3) damage = 50; // Skeleton
                            
                            sprite.hp -= damage;
                            sprite.damageTimer = 0.2;
                            if (sprite.hp <= 0 && sprite.state != 0) {
                                playerScore += enemyTypes[sprite.type].scoreValue;
                                sprite.state = 0;
                                sprite.animTimer = 0;
                                sprite.frameIndex = 0;
                                sprite.deadTimer = 5.0;
                                if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                                    playSound(enemyTypes[sprite.type].soundDeath, enemyTypes[sprite.type].soundDeathSamples, enemyTypes[sprite.type].soundDeathRate, enemyTypes[sprite.type].soundDeathChannels);
                                }
                            } else if (sprite.hp > 0 && sprite.state != 0) {
                                sprite.state = 2;
                                sprite.animTimer = 0;
                                sprite.frameIndex = 0;
                                if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                                    playSound(enemyTypes[sprite.type].soundPain, enemyTypes[sprite.type].soundPainSamples, enemyTypes[sprite.type].soundPainRate, enemyTypes[sprite.type].soundPainChannels);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


void Engine::resetGame() {
    player.hp = 100;
    player.x = 2.0;
    player.y = 2.0;
    player.z = 0;
    player.vz = 0;
    player.dirX = -1.0;
    player.dirY = 0.0;
    player.planeX = 0.0;
    player.planeY = 0.66;
    playerDamageTimer = 0.0;
    playerScore = 0;
    enemySpawnTimer = 4.0;
    gameOverTimer = 0.0;

    player.currentWeapon = 0;
    for (int i = 0; i < 5; i++) {
        player.hasWeapon[i] = false;
        player.ammo[i] = 0;
    }
    player.hasWeapon[0] = true;
    player.ammo[0] = -1;

    isShooting = false;
    shootTimer = 0.0;
    weaponFrameIndex = 0;
    weaponAnimTimer = 0.0;

    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            doorStates[x][y] = 0;
            doorOffsets[x][y] = 0.0;
        }
    }

    sprites.clear();
    sprites = {
        {8.5, 8.5, 0, 0, 100, 1, 0.0, 8.5, 8.5, 0.0, 0.0, 0, 0.0},
        {10.5, 9.5, 0, 0, 100, 1, 0.0, 10.5, 9.5, 0.0, 0.0, 0, 0.0},
        {13.5, 3.5, 0, 0, 100, 1, 0.0, 13.5, 3.5, 0.0, 0.0, 0, 0.0},
        {22.5, 15.5, 0, 1, 200, 1, 0.0, 22.5, 15.5, 0.0, 0.0, 0, 0.0},
        {20.5, 20.5, 0, 1, 200, 1, 0.0, 20.5, 20.5, 0.0, 0.0, 0, 0.0},
        {5.5, 25.5, 0, 0, 100, 1, 0.0, 5.5, 25.5, 0.0, 0.0, 0, 0.0},
        {15.5, 28.5, 0, 1, 200, 1, 0.0, 15.5, 28.5, 0.0, 0.0, 0, 0.0},
        {28.5, 4.5, 0, 2, 300, 1, 0.0, 28.5, 4.5, 0.0, 0.0, 0, 0.0},
    };

    struct ItemSpawn { double x, y; int type; };
    std::vector<ItemSpawn> spawns = {
        {2.5, 2.5, 0}, {15.5, 6.5, 0}, {15.5, 21.5, 0},
        {10.5, 2.5, 1}, {5.5, 15.5, 1}, {28.5, 28.5, 1},
        {16.5, 2.5, 1}, {27.5, 2.5, 1}, {20.5, 15.5, 1},
        {5.5, 21.5, 2},
        {12.5, 15.5, 3}
    };
    for (const auto& spawn : spawns) {
        sprites.push_back({spawn.x, spawn.y, 0, spawn.type, 100, 1, 0.0, spawn.x, spawn.y, 0.0, 0.0, 0, 0.0, true});
    }
}
