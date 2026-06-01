#pragma once

#include <vector>
#include <cstdint>
#include "Player.hpp"
#include "Map.hpp"
#include "Sprite.hpp"

constexpr int TEX_WIDTH = 64;
constexpr int TEX_HEIGHT = 64;

class Raycaster {
public:
    Raycaster() = default;
    void render(std::vector<uint32_t>& framebuffer, int screenWidth, int screenHeight, const Player& player, const uint32_t textures[5][TEX_WIDTH * TEX_HEIGHT], std::vector<double>& zBuffer, const double doorOffsets[32][32]);
    void renderSprites(std::vector<uint32_t>& framebuffer, int screenWidth, int screenHeight, const Player& player, std::vector<Sprite>& sprites, const std::vector<double>& zBuffer, const std::vector<EnemyDef>& enemyTypes, const std::vector<std::vector<SpriteFrame>>& projectileTypes, const std::vector<ItemDef>& itemTypes);
};