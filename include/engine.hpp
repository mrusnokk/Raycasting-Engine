#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include <string>
#include "Player.hpp"
#include "Raycaster.hpp"

enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER
};

#define TEX_WIDTH 64
#define TEX_HEIGHT 64

// Globální pole pro textury a font (musí být inicializováno v .cpp)
// Pole pro 5 různých textur (0 prázdná, 1-3 zdi, 4 dveře)
inline uint32_t textures[5][TEX_WIDTH * TEX_HEIGHT];
inline uint32_t floorTexture[TEX_WIDTH * TEX_HEIGHT];
inline uint32_t ceilTexture[TEX_WIDTH * TEX_HEIGHT];

#include "Sprite.hpp"

class Engine {
public:
    Engine(int width, int height);
    ~Engine(); // Destruktor se postará o úklid

    void run(); // Hlavní metoda, která spustí hru

    double doorOffsets[32][32] = {0.0};
    int doorStates[32][32] = {0}; // 0 = closed, 1 = opening, 2 = open, 3 = closing
    bool isWalkable(int x, int y);

private:
    GameState currentState = GameState::MENU;
    int menuSelection = 0;
    bool isFullscreen = false;

    void drawMenu(); // Nová metoda pro vykreslení menu
    void handleMenuInput(SDL_Event& event); // Oddělená logika vstupů pro menu
    
    Raycaster raycaster;
    void processInput(double deltaTime); // <--- Přidán parametr
    void render();

    // Nové metody pro kreslení 2D prvků
    void drawRect(int startX, int startY, int width, int height, uint32_t color);
    void drawText(const std::string& text, int x, int y, uint32_t color, int scale = 2);
    void drawMinimap();

    int screenWidth;
    int screenHeight;
    bool isRunning;
    uint64_t lastTime; // <--- Přidána proměnná pro čas

    int playerScore = 0;
    int highScore = 0;
    void loadHighScore();
    void saveHighScore();

    double enemySpawnTimer = 0.0;

    // Proměnné pro zbraň
    bool isMoving;
    double weaponBobTime;
    bool isShooting = false;
    double shootTimer = 0.0;
    double playerDamageTimer = 0.0;
    double gameOverTimer = 0.0;
// UI textury (dynamické velikosti)
    std::vector<SpriteFrame> weaponFrames;
    std::vector<EnemyDef> enemyTypes;
    void loadEnemyDef(const std::string& directoryPath, EnemyDef& def);
    std::vector<std::vector<SpriteFrame>> projectileTypes;
    int weaponFrameIndex = 0;
    double weaponAnimTimer = 0.0;
    double WEAPON_ANIM_SPEED = 0.05;

    std::vector<uint32_t> menuBgTexture;
    int menuBgTexWidth = 0, menuBgTexHeight = 0;
    
    std::vector<uint32_t> deathTexture;
    int deathTexWidth = 0, deathTexHeight = 0;

    // --- AUDIO SYSTEM ---
    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioStream* weaponAudioStream = nullptr;
    SDL_AudioStream* activeStreams[8] = {nullptr};
    int currentAudioStream = 0;
    void playSound(short* data, int samples, int sampleRate = 44100, int channels = 1);
    short* weaponAudioData = nullptr;
    int weaponAudioSamples = 0;
    int weaponAudioRate = 44100;
    int weaponAudioChannels = 1;

    short* playerPainData = nullptr;
    int playerPainSamples = 0;
    int playerPainRate = 44100;
    int playerPainChannels = 1;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* frameTexture;
    
    std::vector<uint32_t> framebuffer;
    std::vector<double> zBuffer;
    std::vector<Sprite> sprites;
    Player player;
};