#include "engine.hpp"
#include <iostream>
#include <time.h>

Engine::Engine(int width, int height)
    : screenWidth(width), screenHeight(height), isRunning(false),
      window(nullptr), renderer(nullptr), frameTexture(nullptr),
      framebuffer(width * height, 0)
{
    srand((unsigned)time(NULL));
    player = {2.0, 2.0, -1.0, 0.0, 0.0, 0.66};
    loadHighScore();
    loadSettings();
    framebuffer.resize(screenWidth * screenHeight);

    initSDL();
    loadTextures();
    loadAudio();
    
    setGlobalVolume(globalVolume);

    // Zbraně se nyní animují vždy, proto resetujeme časovač a snímek
    weaponAnimTimer = 0.0;
    weaponFrameIndex = 0;
}

Engine::~Engine()
{
    saveHighScore();
    saveSettings();
    if (stepAudioStream) SDL_DestroyAudioStream(stepAudioStream);
    if (doorAudioStream) SDL_DestroyAudioStream(doorAudioStream);

    if (audioDevice) {
        SDL_CloseAudioDevice(audioDevice);
    }
    
    for (int i = 0; i < 5; i++) {
        if (weaponAudioData[i]) free(weaponAudioData[i]);
    }
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

        update(deltaTime); // Předáme čas do vstupů
        render();
    }
}

void Engine::changeResolution(int w, int h) {
    if (w == screenWidth && h == screenHeight) return;

    screenWidth = w;
    screenHeight = h;

    SDL_SetWindowSize(window, screenWidth, screenHeight);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    framebuffer.resize(screenWidth * screenHeight);
    zBuffer.resize(screenWidth);

    if (frameTexture) {
        SDL_DestroyTexture(frameTexture);
    }
    frameTexture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
        screenWidth, screenHeight);
}

void Engine::setGlobalVolume(float vol) {
    globalVolume = std::max(0.0f, std::min(vol, 1.0f));
    
    if (weaponAudioStream) SDL_SetAudioStreamGain(weaponAudioStream, globalVolume);
    if (doorAudioStream) SDL_SetAudioStreamGain(doorAudioStream, globalVolume);
    if (stepAudioStream) SDL_SetAudioStreamGain(stepAudioStream, globalVolume);
    
    for (int i = 0; i < 8; ++i) {
        if (activeStreams[i]) {
            SDL_SetAudioStreamGain(activeStreams[i], globalVolume);
        }
    }
}
