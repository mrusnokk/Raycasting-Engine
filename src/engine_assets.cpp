#include "engine.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_vorbis.c"
#include <stdlib.h>

void Engine::initSDL() {
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

    // NA STARTU JE HRA V MENU, TAKŽE MYŠ NEZAMYKÁME
    SDL_SetWindowRelativeMouseMode(window, false);

    if (window && renderer && frameTexture)
    {
        isRunning = true;
    }

    zBuffer.resize(screenWidth);
    // Vytvoření hordy nepřátel na velkou mapu
    

}

void Engine::loadTextures() {
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
        {28.5, 4.5, 0, 2, 300, 1, 0.0, 28.5, 4.5, 0.0, 0.0, 0, 0.0},
    };

    // Pevně dané pozice pro předměty (typ: 0=Medkit, 1=Ammo, 2=Chaingun, 3=Coachgun)
    struct ItemSpawn { double x, y; int type; };
    std::vector<ItemSpawn> spawns = {
        // Medkit (3x)
        {2.5, 2.5, 0}, {15.5, 8.5, 0}, {15.5, 21.5, 0},
        // Ammo (6x)
        {10.5, 2.5, 1}, {5.5, 15.5, 1}, {28.5, 28.5, 1},
        {16.5, 2.5, 1}, {27.5, 2.5, 1}, {20.5, 15.5, 1},
        // Chaingun (1x)
        {5.5, 21.5, 2},
        // Coachgun (1x)
        {10.5, 15.5, 3}
    };
    
    for (const auto& spawn : spawns) {
        sprites.push_back({spawn.x, spawn.y, 0, spawn.type, 100, 1, 0.0, spawn.x, spawn.y, 0.0, 0.0, 0, 0.0, true});
    }
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
    
    // Načítání zbraní
    auto loadWpn = [&](const std::string& pathPrefix, std::vector<SpriteFrame>& frames, char startC, char endC) {
        for (char c = startC; c <= endC; c++) {
            std::string filename = pathPrefix + std::string(1, c) + "0.png";
            std::vector<uint32_t> frameTex;
            int frameW = 0, frameH = 0;
            loadDynTex(filename.c_str(), frameTex, frameW, frameH, true);
            if (!frameTex.empty()) {
                SpriteFrame sf; sf.pixels = frameTex; sf.w = frameW; sf.h = frameH;
                frames.push_back(sf);
            } else { break; }
        }
    };
    
    std::vector<SpriteFrame> w0, w1, w2, w3;
    loadWpn("assets/SPRITES/WEAPONS/AUT9", w0, 'A', 'B'); // Pistol
    loadWpn("assets/newGuns/chaingun/HCGF", w1, 'A', 'B'); // Chaingun
    loadWpn("assets/newGuns/coachgun/COCH", w2, 'A', 'B'); // Coachgun
    loadWpn("assets/newGuns/skeletonrevange/sprites/skeletalrevenge/BOGU", w3, 'A', 'B'); // Skeleton
    
    allWeaponFrames.push_back(w0);
    allWeaponFrames.push_back(w1);
    allWeaponFrames.push_back(w2);
    allWeaponFrames.push_back(w3);
    
    if (!allWeaponFrames[0].empty()) weaponFrames = allWeaponFrames[0];

    // Načítání předmětů
    auto loadItem = [&](const std::string& path, ItemType type, int amount, int wpnId = -1) {
        ItemDef idf; idf.type = type; idf.amount = amount; idf.weaponId = wpnId;
        std::vector<uint32_t> frameTex; int frameW = 0, frameH = 0;
        loadDynTex(path.c_str(), frameTex, frameW, frameH, true);
        if (!frameTex.empty()) {
            SpriteFrame sf; sf.pixels = frameTex; sf.w = frameW; sf.h = frameH;
            idf.frames.push_back(sf);
            itemTypes.push_back(idf);
        }
    };
    
    loadItem("assets/items/medkit.png", ItemType::MEDKIT, 25);
    loadItem("assets/items/ammo.png", ItemType::AMMO, 20);
    loadItem("assets/newGuns/chaingun/HCNGA0.png", ItemType::WEAPON, 20, 1); // Zbraň 1 + 20 nábojů
    loadItem("assets/newGuns/coachgun/COCPA0.png", ItemType::WEAPON, 8, 2); // Zbraň 2 + 8 nábojů
    loadDynTex("assets/menu_bg.png", menuBgTexture, menuBgTexWidth, menuBgTexHeight);
    loadDynTex("assets/death_screen.png", deathTexture, deathTexWidth, deathTexHeight);

    // --- AUDIO SYSTEM ---

}

void Engine::loadAudio() {
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

void Engine::loadSettings() {
    std::ifstream file("settings.cfg");
    if (file.is_open()) {
        file >> screenWidth >> screenHeight >> mouseSensitivity >> globalVolume;
        file.close();
    } else {
        // Výchozí hodnoty
        screenWidth = 800;
        screenHeight = 600;
        mouseSensitivity = 0.2;
        globalVolume = 1.0f;
    }
}

void Engine::saveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        file << screenWidth << " " << screenHeight << " " << mouseSensitivity << " " << globalVolume;
        file.close();
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
        SDL_SetAudioStreamGain(activeStreams[currentAudioStream], globalVolume);
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

