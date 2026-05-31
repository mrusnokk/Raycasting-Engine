#include "Engine.hpp"
#include <iostream>
#include <cmath>
#include <filesystem>
#include <algorithm>   // Pro sin() a cos()
#include "Map.hpp" // Potřebujeme vidět mapu kvůli kolizím
#include "Font.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vložíme implementaci stb_vorbis.c přímo sem, abychom nemuseli řešit C/C++ kompilátory v CMake
#include "stb_vorbis.c"
#include <stdlib.h>
#include <time.h>

Engine::Engine(int width, int height)
    : screenWidth(width), screenHeight(height), isRunning(false),
      window(nullptr), renderer(nullptr), frameTexture(nullptr),
      framebuffer(width * height, 0)
{
    srand((unsigned)time(NULL));
    player = {2.0, 2.0, -1.0, 0.0, 0.0, 0.66};
    loadHighScore();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Chyba inicializace SDL3: " << SDL_GetError() << "\n";
        return;
    }

    window = SDL_CreateWindow("2.5D Retro Engine", screenWidth, screenHeight, 0);
    renderer = SDL_CreateRenderer(window, nullptr);
    frameTexture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
        screenWidth, screenHeight);

    // ZAMKNUTÍ MYŠI DO OKNA (Skryje kurzor a snímá jen pohyb)
    SDL_SetWindowRelativeMouseMode(window, true);

    if (window && renderer && frameTexture)
    {
        isRunning = true;
    }

    zBuffer.resize(screenWidth);
    // Vytvoření hordy nepřátel na velkou mapu
    
    auto loadProj = [&](const std::string& prefix, std::vector<SpriteFrame>& frames) {
        for (int i = 0; i < 26; ++i) {
            char suffix = 'A' + i;
            std::string path1 = "assets/SPRITES/PROJECTILES/" + prefix + suffix + "0.png";
            std::string path2 = "assets/SPRITES/PROJECTILES/" + prefix + suffix + "0.PNG";
            std::string path = std::filesystem::exists(path1) ? path1 : (std::filesystem::exists(path2) ? path2 : "");
            if (!path.empty()) {
                SpriteFrame frame;
                int channels;
                unsigned char* data = stbi_load(path.c_str(), &frame.w, &frame.h, &channels, 4);
                if (data) {
                    frame.pixels.assign((uint32_t*)data, (uint32_t*)data + (frame.w * frame.h));
                    stbi_image_free(data);
                    frames.push_back(frame);
                }
            } else {
                break; // Stop loading if frame doesn't exist
            }
        }
    };
    std::vector<SpriteFrame> p0, p1, p2;
    loadProj("AGAS", p0); // Agaures projectile
    loadProj("BCAB", p1); // Cacobite projectile
    loadProj("BLTR", p2); // Arachnobaron projectile
    projectileTypes.push_back(p0);
    projectileTypes.push_back(p1);
    projectileTypes.push_back(p2);

    EnemyDef type0; loadEnemyDef("assets/SPRITES/ENEMIES/Agaures", type0); type0.maxHp = 100; type0.scoreValue = 100; if (!type0.idleFrames.empty()) enemyTypes.push_back(type0);
    EnemyDef type1; loadEnemyDef("assets/SPRITES/ENEMIES/Cacobite", type1); type1.maxHp = 200; type1.scoreValue = 300; if (!type1.idleFrames.empty()) enemyTypes.push_back(type1);
    EnemyDef type2; loadEnemyDef("assets/SPRITES/ENEMIES/Arachnobaron", type2); type2.maxHp = 300; type2.scoreValue = 500; if (!type2.idleFrames.empty()) enemyTypes.push_back(type2);
    sprites = {
        {8.5, 8.5, 0, 0, 100, 1, 0.0, 8.5, 8.5, 0.0, 0.0, 0, 0.0},
        {10.5, 9.5, 0, 0, 100, 1, 0.0, 10.5, 9.5, 0.0, 0.0, 0, 0.0},
        {13.5, 3.5, 0, 0, 100, 1, 0.0, 13.5, 3.5, 0.0, 0.0, 0, 0.0},
        {22.5, 15.5, 0, 1, 200, 1, 0.0, 22.5, 15.5, 0.0, 0.0, 0, 0.0},
        {20.5, 20.5, 0, 1, 200, 1, 0.0, 20.5, 20.5, 0.0, 0.0, 0, 0.0},
        {5.5, 25.5, 0, 0, 100, 1, 0.0, 5.5, 25.5, 0.0, 0.0, 0, 0.0},
        {15.5, 28.5, 0, 1, 200, 1, 0.0, 15.5, 28.5, 0.0, 0.0, 0, 0.0},
        {28.5, 4.5, 0, 2, 300, 1, 0.0, 28.5, 4.5, 0.0, 0.0, 0, 0.0}
    };
// Pomocná funkce pro načtení textury pomocí stb_image
    auto loadTex = [](const char* path, uint32_t* texArray) {
        int w, h, channels;
        unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
        if (data) {
            for (int y = 0; y < TEX_HEIGHT; y++) {
                for (int x = 0; x < TEX_WIDTH; x++) {
                    int srcX = (x * w) / TEX_WIDTH;
                    int srcY = (y * h) / TEX_HEIGHT;
                    int idx = (srcY * w + srcX) * 4;
                    uint8_t r = data[idx];
                    uint8_t g = data[idx+1];
                    uint8_t b = data[idx+2];
                    uint8_t a = data[idx+3];
                    texArray[TEX_WIDTH * y + x] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
            stbi_image_free(data);
            return true;
        }
        std::cerr << "Nepodarilo se nacist texturu: " << path << "\n";
        return false;
    };

    // Načtení externích textur
    if (!loadTex("assets/wall.png", textures[1])) {
        // Fallback, pokud textura chybí
        for (int x = 0; x < TEX_WIDTH; x++) {
            for (int y = 0; y < TEX_HEIGHT; y++) {
                int xorcolor = (x * 256 / TEX_WIDTH) ^ (y * 256 / TEX_HEIGHT);
                textures[1][TEX_WIDTH * y + x] = 0xFF000000 | (xorcolor << 16) | (0 << 8) | 0;
            }
        }
    }
    loadTex("assets/wall.png", textures[2]);
    loadTex("assets/wall.png", textures[3]);
    
    if (!loadTex("assets/door.png", textures[4])) {
        // Fallback pro dveře (žlutý čtverec, aby byly nápadné)
        for (int x = 0; x < TEX_WIDTH; x++) {
            for (int y = 0; y < TEX_HEIGHT; y++) {
                textures[4][TEX_HEIGHT * y + x] = 0xFFFFFF00;
            }
        }
    }
    
    if (!loadTex("assets/floor.png", floorTexture)) {
        for(int i=0;i<TEX_WIDTH*TEX_HEIGHT;i++) floorTexture[i]=0xFF444444;
    }
    if (!loadTex("assets/ceil.png", ceilTexture)) {
        for(int i=0;i<TEX_WIDTH*TEX_HEIGHT;i++) ceilTexture[i]=0xFF222222;
    }


    // Načtení UI textur a Death Screen s dynamickou velikostí
    auto loadDynTex = [](const char* path, std::vector<uint32_t>& tex, int& w, int& h, bool autoTransparent = false) {
        int channels;
        unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
        if (data) {
            tex.resize(w * h);
            uint32_t bgColor = 0;
            // Pokud je zapnutý autoTransparent, ale obrázek už MÁ průhlednost v levém horním rohu, vypneme ho.
            if (autoTransparent && data[3] == 0) {
                autoTransparent = false;
            }
            if (autoTransparent) {
                bgColor = (data[0] << 16) | (data[1] << 8) | data[2];
            }

            for (int i = 0; i < w * h; i++) {
                uint8_t r = data[i*4];
                uint8_t g = data[i*4+1];
                uint8_t b = data[i*4+2];
                uint8_t a = data[i*4+3];
                
                if (autoTransparent) {
                    int dr = std::abs((int)r - (int)((bgColor >> 16) & 0xFF));
                    int dg = std::abs((int)g - (int)((bgColor >> 8) & 0xFF));
                    int db = std::abs((int)b - (int)(bgColor & 0xFF));
                    // Pokud je barva hodně podobná barvě levého horního pixelu, zprůhledníme ji
                    if (dr + dg + db < 60) {
                        a = 0;
                    }
                }
                
                tex[i] = (a << 24) | (r << 16) | (g << 8) | b;
            }
            stbi_image_free(data);
        }
    };
    
    // Zbraň - načítání jednotlivých framů z AUT9 (A a B pro střelbu)
    for (char c = 'A'; c <= 'B'; c++) {
        std::string filename = "assets/SPRITES/WEAPONS/AUT9" + std::string(1, c) + "0.png";
        std::vector<uint32_t> frameTex;
        int frameW = 0, frameH = 0;
        loadDynTex(filename.c_str(), frameTex, frameW, frameH, true);
        if (!frameTex.empty()) {
            SpriteFrame sf;
            sf.pixels = frameTex;
            sf.w = frameW;
            sf.h = frameH;
            weaponFrames.push_back(sf);
        } else {
            break; // Konec sekvence
        }
    }
    loadDynTex("assets/menu_bg.png", menuBgTexture, menuBgTexWidth, menuBgTexHeight);
    loadDynTex("assets/death_screen.png", deathTexture, deathTexWidth, deathTexHeight);

    // --- AUDIO SYSTEM ---
    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audioDevice) {
        SDL_ResumeAudioDevice(audioDevice);
        int channels = 0, sample_rate = 0;
        short* decoded_data = nullptr;
                int samples = stb_vorbis_decode_filename("assets/SOUNDS/AUT9FIRC.ogg", &channels, &sample_rate, &decoded_data);
        if (samples <= 0) samples = stb_vorbis_decode_filename("SOUNDS/AUT9FIRC.ogg", &channels, &sample_rate, &decoded_data);
        if (samples > 0 && decoded_data) {
            weaponAudioData = decoded_data;
            weaponAudioSamples = samples * channels;
            weaponAudioRate = sample_rate;
            weaponAudioChannels = channels;
            SDL_AudioSpec spec;
            spec.format = SDL_AUDIO_S16LE;
            spec.channels = channels;
            spec.freq = sample_rate;
            weaponAudioStream = SDL_CreateAudioStream(&spec, nullptr);
            if (weaponAudioStream) SDL_BindAudioStream(audioDevice, weaponAudioStream);
        }
        samples = stb_vorbis_decode_filename("assets/SOUNDS/AGURPAIN.ogg", &channels, &sample_rate, &decoded_data);
        if (samples <= 0) samples = stb_vorbis_decode_filename("SOUNDS/AGURPAIN.ogg", &channels, &sample_rate, &decoded_data);
        if (samples > 0 && decoded_data) {
            playerPainData = decoded_data;
            playerPainSamples = samples * channels;
            playerPainRate = sample_rate;
            playerPainChannels = channels;
        }

        samples = stb_vorbis_decode_filename("assets/SOUNDS/step.ogg", &channels, &sample_rate, &decoded_data);
        if (samples > 0 && decoded_data) {
            stepAudioData = decoded_data;
            stepAudioSamples = samples * channels;
            stepAudioRate = sample_rate;
            stepAudioChannels = channels;
            SDL_AudioSpec spec = { SDL_AUDIO_S16LE, channels, sample_rate };
            stepAudioStream = SDL_CreateAudioStream(&spec, nullptr);
            if (stepAudioStream) SDL_BindAudioStream(audioDevice, stepAudioStream);
        }

        samples = stb_vorbis_decode_filename("assets/SOUNDS/door.ogg", &channels, &sample_rate, &decoded_data);
        if (samples > 0 && decoded_data) {
            doorAudioData = decoded_data;
            doorAudioSamples = samples * channels;
            doorAudioRate = sample_rate;
            doorAudioChannels = channels;
            SDL_AudioSpec spec = { SDL_AUDIO_S16LE, channels, sample_rate };
            doorAudioStream = SDL_CreateAudioStream(&spec, nullptr);
            if (doorAudioStream) SDL_BindAudioStream(audioDevice, doorAudioStream);
        }

    }
}

Engine::~Engine()
{
    saveHighScore();
    if (weaponAudioStream) SDL_DestroyAudioStream(weaponAudioStream);
    if (stepAudioStream) SDL_DestroyAudioStream(stepAudioStream);
    if (doorAudioStream) SDL_DestroyAudioStream(doorAudioStream);

    if (audioDevice) {
        SDL_CloseAudioDevice(audioDevice);
    }
    
    if (weaponAudioData) free(weaponAudioData);
    if (playerPainData) free(playerPainData);
    if (stepAudioData) free(stepAudioData);
    if (doorAudioData) free(doorAudioData);

    if (frameTexture)
        SDL_DestroyTexture(frameTexture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Engine::isWalkable(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    if (worldMap[x][y] == 0) return true;
    if (worldMap[x][y] == 4 && doorStates[x][y] == 2) return true;
    return false;
}

void Engine::processInput(double deltaTime) {
    SDL_Event event;
    
    // ==========================================
    // 1. ZPRACOVÁNÍ JEDNORÁZOVÝCH UDÁLOSTÍ
    // ==========================================
    while (SDL_PollEvent(&event)) {
        // Křížek okna
        if (event.type == SDL_EVENT_QUIT) isRunning = false;
        
        // --- STISK KLÁVES ---
        if (event.type == SDL_EVENT_KEY_DOWN) {
            
            // ESCAPE - PŘEPÍNÁNÍ MENU A HRY
            if (event.key.key == SDLK_ESCAPE) {
                currentState = (currentState == GameState::PLAYING) ? GameState::MENU : GameState::PLAYING;
                
                // Zamknutí myši pro rotaci (když hrajeme) nebo uvolnění kurzoru (když jsme v menu)
                SDL_SetWindowRelativeMouseMode(window, currentState == GameState::PLAYING);
            }
            
            // INTERAKCE S DVEŘMI (E)
            if (currentState == GameState::PLAYING && event.key.key == SDLK_E) {
                int targetX = int(player.x + player.dirX * 1.5);
                int targetY = int(player.y + player.dirY * 1.5);
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
            
            // OVLÁDÁNÍ MENU POMOCÍ KLÁVESNICE
            if (currentState == GameState::MENU) {
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
                        isFullscreen = !isFullscreen;
                        SDL_SetWindowFullscreen(window, isFullscreen);
                    }
                    else if (menuSelection == 2) {
                        isRunning = false; 
                    }
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
                            isFullscreen = !isFullscreen;
                            SDL_SetWindowFullscreen(window, isFullscreen);
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
            double rotSpeed = event.motion.xrel * -0.2 * deltaTime;
            
            // Rotační matice pro směr pohledu a rovinu kamery
            double oldDirX = player.dirX;
            player.dirX = player.dirX * std::cos(rotSpeed) - player.dirY * std::sin(rotSpeed);
            player.dirY = oldDirX * std::sin(rotSpeed) + player.dirY * std::cos(rotSpeed);
            
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * std::cos(rotSpeed) - player.planeY * std::sin(rotSpeed);
            player.planeY = oldPlaneX * std::sin(rotSpeed) + player.planeY * std::cos(rotSpeed);
        }

        // --- STŘELBA ---
        if (currentState == GameState::PLAYING && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (!isShooting && player.hp > 0) {
                isShooting = true;
                
                double audioDuration = 0.25; // Default fallback
                if (weaponAudioRate > 0 && weaponAudioChannels > 0) {
                    audioDuration = (double)(weaponAudioSamples / weaponAudioChannels) / weaponAudioRate;
                }
                shootTimer = (weaponFrames.size() > 1) ? audioDuration : 0.2; // Celkový čas (výstřel + cooldown)
                
                if (weaponFrames.size() > 1) {
                    weaponFrameIndex = 1; // Snímek záblesku
                    weaponAnimTimer = 0.0;
                }

                // Přehrajeme zvuk výstřelu
                if (weaponAudioStream && weaponAudioData) {
                    SDL_PutAudioStreamData(weaponAudioStream, weaponAudioData, weaponAudioSamples * sizeof(short));
                    SDL_FlushAudioStream(weaponAudioStream);
                    SDL_FlushAudioStream(weaponAudioStream);
                }

                // Hitscan
                for (auto& sprite : sprites) {
                    if (sprite.state <= 0) continue;
                    if (sprite.isProjectile) continue;
                    
                    double spriteX = sprite.x - player.x;
                    double spriteY = sprite.y - player.y;

                    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
                    double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
                    double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);

                    if (transformY > 0) {
                        int spriteScreenX = int((screenWidth / 2) * (1 + transformX / transformY));
                        int spriteWidth = std::abs(int(screenHeight / transformY));
                        
                        // Zkontrolujeme, jestli je střed obrazovky uvnitř šířky spritu
                        if (screenWidth / 2 > spriteScreenX - spriteWidth / 2 && screenWidth / 2 < spriteScreenX + spriteWidth / 2) {
                            if (transformY < 15.0) { // Dostřel
                                sprite.hp -= 35;
                                sprite.damageTimer = 0.2; 
                                if (sprite.hp <= 0 && sprite.state != 0) {
                                    playerScore += enemyTypes[sprite.type].scoreValue;
                                    sprite.state = 0; 
                                    sprite.animTimer = 0;
                                    sprite.frameIndex = 0;
                                    sprite.deadTimer = 5.0; // 5 sekund do respawnu
                                    if (sprite.type >= 0 && sprite.type < enemyTypes.size()) {
                                        playSound(enemyTypes[sprite.type].soundDeath, enemyTypes[sprite.type].soundDeathSamples, enemyTypes[sprite.type].soundDeathRate, enemyTypes[sprite.type].soundDeathChannels);
                                    }
                                } else if (sprite.hp > 0 && sprite.state != 0) {
                                    sprite.state = 2; // Pain state
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
        
        if (currentState == GameState::GAME_OVER) {
            if (gameOverTimer <= 0 && (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_KEY_DOWN)) {
                // Resetování hry a návrat do MENU
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
                
                for (auto& sprite : sprites) {
                    sprite.state = 1;
                    sprite.hp = 100;
                    sprite.x = sprite.spawnX;
                    sprite.y = sprite.spawnY;
                    sprite.deadTimer = 0;
                    sprite.damageTimer = 0;
                }
                currentState = GameState::MENU;
            }
        }
    }

    // ==========================================
    // 2. PLYNULÝ POHYB HRÁČE (Pouze během hraní)
    // ==========================================
    if (currentState == GameState::PLAYING) {
        if (isShooting) {
            shootTimer -= deltaTime;
            if (shootTimer <= 0) {
                isShooting = false;
                shootTimer = 0.0;
                weaponFrameIndex = 0;
            } else {
                if (weaponFrames.size() > 1) {
                    weaponAnimTimer += deltaTime;
                    // Snímek výstřelu zobrazíme jen na krátkou dobu (např. 0.08s) a pak se vrátíme do idle, ale střílet dál nejde dokud nedoběhne shootTimer
                    if (weaponAnimTimer >= 0.08 && weaponFrameIndex != 0) {
                        weaponFrameIndex = 0;
                    }
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

                // --- AI NEPŘÁTEL A PROJEKTILY ---
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

        // --- AKTUALIZACE HOUPÁNÍ ZBRANĚ A CHŮZE ---
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
        }

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
                        if (s.state == -1 && !s.isProjectile) {
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
    } // END of if (currentState == GameState::PLAYING)

    // Časovač game over (musí běžet nezávisle na PLAYING stavu)
    if (gameOverTimer > 0) gameOverTimer -= deltaTime;
}

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
    std::string buttonTexts[3] = {"PLAY GAME", "FULLSCREEN", "QUIT"};

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
    raycaster.renderSprites(framebuffer, screenWidth, screenHeight, player, sprites, zBuffer, enemyTypes, projectileTypes);

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

#include <fstream>

void Engine::loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        highScore = 0;
    }
}

void Engine::saveHighScore() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}

void Engine::run()
{
    lastTime = SDL_GetTicks(); // Zaznamenáme čas před startem

    while (isRunning)
    {
        // Výpočet Delta Time v sekundách
        uint64_t currentTime = SDL_GetTicks();
        double deltaTime = (currentTime - lastTime) / 1000.0;
        lastTime = currentTime;

        // Ochrana proti příliš velkým skokům (např. při lagování okna)
        if (deltaTime > 0.1)
            deltaTime = 0.1;

        processInput(deltaTime); // Předáme čas do vstupů
        render();
    }
}

void Engine::playSound(short* data, int samples, int sampleRate, int channels) {
    if (!audioDevice || !data || samples <= 0) return;
    if (activeStreams[currentAudioStream]) {
        SDL_DestroyAudioStream(activeStreams[currentAudioStream]);
    }
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = channels;
    spec.freq = sampleRate;
    activeStreams[currentAudioStream] = SDL_CreateAudioStream(&spec, nullptr);
    if (activeStreams[currentAudioStream]) {
        SDL_BindAudioStream(audioDevice, activeStreams[currentAudioStream]);
        SDL_PutAudioStreamData(activeStreams[currentAudioStream], data, samples * sizeof(short));
        SDL_FlushAudioStream(activeStreams[currentAudioStream]);
    }
    currentAudioStream = (currentAudioStream + 1) % 8;
}


void Engine::loadEnemyDef(const std::string& directoryPath, EnemyDef& def) {
    if (!std::filesystem::exists(directoryPath)) return;
    std::string folderName = std::filesystem::path(directoryPath).filename().string();
    auto loadDynTexLocal = [](const std::string& path, std::vector<uint32_t>& tex, int& w, int& h) {
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (data) {
            tex.resize(w * h);
            for (int i = 0; i < w * h; i++) {
                uint8_t r = data[i * 4 + 0], g = data[i * 4 + 1], b = data[i * 4 + 2], a = data[i * 4 + 3];
                if (r == 0 && g == 255 && b == 255) a = 0;
                tex[i] = (a << 24) | (r << 16) | (g << 8) | b;
            }
            stbi_image_free(data);
        }
    };
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".PNG") files.push_back(entry.path().string());
    }
    std::sort(files.begin(), files.end());
    for (const auto& file : files) {
        std::string filename = std::filesystem::path(file).filename().string();
        if (filename.find('1') != std::string::npos || filename.find('0') != std::string::npos) {
            SpriteFrame sf;
            loadDynTexLocal(file, sf.pixels, sf.w, sf.h);
            if (!sf.pixels.empty()) {
                char frameLetter = '\0';
                for(size_t i=0; i<filename.length(); i++) {
                    if(filename[i]=='1'||filename[i]=='0') { if(i>0) frameLetter=filename[i-1]; break; }
                }
                
                if (frameLetter != '\0') {
                    if (folderName == "Agaures") {
                        if (frameLetter >= 'A' && frameLetter <= 'D') def.idleFrames.push_back(sf);
                        else if (frameLetter >= 'H' && frameLetter <= 'H') def.painFrames.push_back(sf);
                        else if (frameLetter >= 'I' && frameLetter <= 'M') def.deathFrames.push_back(sf);
                    } else if (folderName == "Cacobite") {
                        if (frameLetter >= 'A' && frameLetter <= 'E') def.idleFrames.push_back(sf);
                        else if (frameLetter == 'F') def.painFrames.push_back(sf);
                        else if (frameLetter >= 'G' && frameLetter <= 'L') def.deathFrames.push_back(sf);
                    } else if (folderName == "Arachnobaron") {
                        if (frameLetter >= 'A' && frameLetter <= 'F') def.idleFrames.push_back(sf);
                        else if (frameLetter >= 'G' && frameLetter <= 'H') def.painFrames.push_back(sf);
                        else if (frameLetter >= 'J' && frameLetter <= 'O') def.deathFrames.push_back(sf);
                    } else {
                        if (frameLetter >= 'A' && frameLetter <= 'E') def.idleFrames.push_back(sf);
                        else if (frameLetter == 'H') def.painFrames.push_back(sf);
                        else if (frameLetter >= 'I' && frameLetter <= 'O') def.deathFrames.push_back(sf);
                    }
                }

            }
        }
    }
    if (def.idleFrames.empty()) return;
    if (def.painFrames.empty()) def.painFrames.push_back(def.idleFrames[0]);
    if (def.deathFrames.empty()) def.deathFrames.push_back(def.idleFrames[0]);
    
    std::string painP, deathP, attackP;
    if (folderName == "Agaures") { painP="assets/SOUNDS/AGURPAIN.ogg"; deathP="assets/SOUNDS/AGURDTH1.ogg"; attackP="assets/SOUNDS/AGURHITS.ogg"; }
    else if (folderName == "Cacobite") { painP="assets/SOUNDS/CACOBPAI.ogg"; deathP="assets/SOUNDS/CACOBDTH.ogg"; attackP="assets/SOUNDS/BABYBITE.ogg"; }
    else if (folderName == "Arachnobaron") { painP="assets/SOUNDS/DSABRDTH.ogg"; deathP="assets/SOUNDS/DSABRDTH.ogg"; attackP="assets/SOUNDS/AGURSWNG.ogg"; }

    auto lSnd = [](const std::string& p, short*& d, int& s, int& r, int& c) {
        if (!p.empty() && std::filesystem::exists(p)) {
            short* dec = nullptr;
            int smp = stb_vorbis_decode_filename(p.c_str(), &c, &r, &dec);
            if (smp > 0) { d = dec; s = smp * c; }
        }
    };
    lSnd(painP, def.soundPain, def.soundPainSamples, def.soundPainRate, def.soundPainChannels);
    lSnd(deathP, def.soundDeath, def.soundDeathSamples, def.soundDeathRate, def.soundDeathChannels);
    lSnd(attackP, def.soundAttack, def.soundAttackSamples, def.soundAttackRate, def.soundAttackChannels);
}
