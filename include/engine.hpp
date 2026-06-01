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
    GAME_OVER,
    SETTINGS
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
    int settingsSelection = 0;
    double mouseSensitivity = 0.2;
    float globalVolume = 1.0f;
    bool isFullscreen = false;

    void drawMenu(); // Nová metoda pro vykreslení menu
    void handleMenuInput(SDL_Event& event); // Oddělená logika vstupů pro menu
    void drawSettings();
    void handleSettingsInput(SDL_Event& event);
    void changeResolution(int w, int h);
    void setGlobalVolume(float vol);
    
    Raycaster raycaster;
    
    /**
     * @brief Zpracovává všechny herní vstupy a updatuje herní logiku.
     * Dříve jedna velká funkce, nyní rozdělená na menší pro lepší čitelnost.
     * @param deltaTime Čas uplynulý od posledního snímku v sekundách.
     */
    void update(double deltaTime);

    /**
     * @brief Zpracovává události z klávesnice a systému (např. zavření okna).
     */
    void handleEvents(double deltaTime);

    /**
     * @brief Řeší pohyb hráče, fyziku, kolize a střelbu.
     */
    void updatePlayer(double deltaTime);

    /**
     * @brief Řeší umělou inteligenci nepřátel, projektily a jejich interakce.
     */
    void updateEntities(double deltaTime);

    /**
     * @brief Zpracovává otevírání a zavírání dveří v závislosti na čase a pozici hráče.
     */
    void updateDoors(double deltaTime);

    /**
     * @brief Obsluhuje automatické nasazování nepřátel v čase (Arcade mód).
     */
    void spawnEnemies(double deltaTime);

    /**
     * @brief Inicializuje SDL knihovnu, okno, renderer a audio zařízení.
     */
    void initSDL();

    /**
     * @brief Načítá všechny statické a dynamické textury do paměti.
     */
    void loadTextures();

    /**
     * @brief Načítá všechny zvukové efekty (STB Vorbis) do paměti.
     */
    void loadAudio();

    void render();

    // Nové metody pro kreslení 2D prvků
    void drawRect(int startX, int startY, int width, int height, uint32_t color);
    void drawRectClipped(int startX, int startY, int width, int height, uint32_t color, int clipX, int clipY, int clipW, int clipH);
    void drawText(const std::string& text, int x, int y, uint32_t color, int scale = 2);
    void drawTextClipped(const std::string& text, int x, int y, int clipX, int clipY, int clipW, int clipH, uint32_t color, int scale = 2);
    void drawMinimap();

    int screenWidth;
    int screenHeight;
    bool isRunning;
    uint64_t lastTime; // <--- Přidána proměnná pro čas

    int playerScore = 0;
    int highScore = 0;
    void loadHighScore();
    void saveHighScore();
    void loadSettings();
    void saveSettings();

    double enemySpawnTimer = 0.0;

    // Proměnné pro zbraň
    bool isMoving;
    bool wasMoving = false;
    double weaponBobTime;
    bool isShooting = false;
    double shootTimer = 0.0;
    double playerDamageTimer = 0.0;
    double gameOverTimer = 0.0;
// UI textury (dynamické velikosti)
    std::vector<SpriteFrame> weaponFrames;
    std::vector<std::vector<SpriteFrame>> allWeaponFrames;
    std::vector<EnemyDef> enemyTypes;
    std::vector<ItemDef> itemTypes;
    void loadEnemyDef(const std::string& directoryPath, EnemyDef& def);
    void loadWeaponDef(const std::string& directoryPath, std::vector<SpriteFrame>& frames);
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

    short* stepAudioData = nullptr;
    int stepAudioSamples = 0;
    int stepAudioRate = 44100;
    int stepAudioChannels = 1;
    SDL_AudioStream* stepAudioStream = nullptr;
    double stepTimer = 0.0;

    short* doorAudioData = nullptr;
    int doorAudioSamples = 0;
    int doorAudioRate = 44100;
    int doorAudioChannels = 1;
    SDL_AudioStream* doorAudioStream = nullptr;

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