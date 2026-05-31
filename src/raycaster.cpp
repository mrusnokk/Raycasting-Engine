#include "Raycaster.hpp"
#include <cmath>
#include <thread>
#include <algorithm>
#include "Engine.hpp"

void Raycaster::render(std::vector<uint32_t>& framebuffer, int screenWidth, int screenHeight, const Player& player, const uint32_t textures[4][TEX_WIDTH * TEX_HEIGHT], std::vector<double>& zBuffer, const double doorOffsets[32][32]) {
    // Zjistíme, kolik vláken náš procesor podporuje (např. 8, 12, 16)
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Pojistka

    std::vector<std::jthread> threads;
    int chunk = screenWidth / numThreads;

    // Vytvoříme Lambda funkci pro vykreslení jen ČÁSTI obrazovky
    auto renderChunk = [&](int startX, int endX) {
        for (int x = startX; x < endX; x++) {
            double cameraX = 2 * x / (double)screenWidth - 1.0;
            double rayDirX = player.dirX + player.planeX * cameraX;
            double rayDirY = player.dirY + player.planeY * cameraX;

            int mapX = int(player.x);
            int mapY = int(player.y);

            double sideDistX, sideDistY;
            double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
            double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);
            double perpWallDist;

            int stepX, stepY;
            int hit = 0;
            int side;

            if (rayDirX < 0) {
                stepX = -1;
                sideDistX = (player.x - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0 - player.x) * deltaDistX;
            }
            if (rayDirY < 0) {
                stepY = -1;
                sideDistY = (player.y - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0 - player.y) * deltaDistY;
            }

            double hitDoorOffset = 0.0;
            while (hit == 0) {
                if (sideDistX < sideDistY) {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                } else {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }
                if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                    if (worldMap[mapX][mapY] > 0) {
                        if (worldMap[mapX][mapY] == 4) { // Dveře
                            double perpDist = (side == 0) ? (mapX - player.x + (1 - stepX) / 2.0) / rayDirX 
                                                          : (mapY - player.y + (1 - stepY) / 2.0) / rayDirY;
                            double wallX = (side == 0) ? player.y + perpDist * rayDirY : player.x + perpDist * rayDirX;
                            wallX -= floor(wallX);
                            
                            double offset = doorOffsets[mapX][mapY];
                            // Pokud paprsek narazí do "pevné" části dveří, je to hit. 
                            // Jinak paprsek jen proletí (smyčka while pokračuje dál).
                            if (wallX > offset) {
                                hit = 1;
                                hitDoorOffset = offset;
                            }
                        } else {
                            hit = 1;
                        }
                    }
                } else {
                    hit = 1;
                }
            }

            if (side == 0) perpWallDist = (mapX - player.x + (1 - stepX) / 2.0) / rayDirX;
            else           perpWallDist = (mapY - player.y + (1 - stepY) / 2.0) / rayDirY;

            int zMove = int(player.z / perpWallDist);

            if (perpWallDist <= 0) perpWallDist = 0.1;
            int lineHeight = (int)(screenHeight / perpWallDist);

            int drawStart = -lineHeight / 2 + screenHeight / 2 + zMove;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + screenHeight / 2 + zMove;
            if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

            // --- VÝPOČET TEXTURY ---
            int texNum = worldMap[mapX][mapY] - 1; // 1 odečteme pro index pole
            if (texNum < 0) texNum = 0;

            double wallX; 
            if (side == 0) wallX = player.y + perpWallDist * rayDirY;
            else           wallX = player.x + perpWallDist * rayDirX;
            wallX -= floor((wallX));
            
            wallX -= hitDoorOffset;
            if (wallX < 0) wallX = 0;

            int texX = int(wallX * double(TEX_WIDTH));
            if (side == 0 && rayDirX > 0) texX = TEX_WIDTH - texX - 1;
            if (side == 1 && rayDirY < 0) texX = TEX_WIDTH - texX - 1;

            // Kolik posunout texturu vertikálně na každý pixel obrazovky
            double step = 1.0 * TEX_HEIGHT / lineHeight;
            double texPos = (drawStart - zMove - screenHeight / 2 + lineHeight / 2) * step;

            zBuffer[x] = perpWallDist;

            // FLOOR AND CEILING CASTING
            double floorXWall, floorYWall;
            if (side == 0 && rayDirX > 0) {
                floorXWall = mapX;
                floorYWall = mapY + wallX;
            } else if (side == 0 && rayDirX < 0) {
                floorXWall = mapX + 1.0;
                floorYWall = mapY + wallX;
            } else if (side == 1 && rayDirY > 0) {
                floorXWall = mapX + wallX;
                floorYWall = mapY;
            } else {
                floorXWall = mapX + wallX;
                floorYWall = mapY + 1.0;
            }

            double distWall = perpWallDist;

            // --- KRESLENÍ SLOUPCE ---
            for (int y = 0; y < screenHeight; y++) {
                int pixelIndex = y * screenWidth + x;
                
                if (y < drawStart) {
                    // Strop
                    double p = screenHeight / 2.0 - y;
                    if (p > 0) {
                        double currentDistCeil = (0.5 * screenHeight - player.z) / p;
                        double weight = currentDistCeil / distWall;
                        double currentFloorX = weight * floorXWall + (1.0 - weight) * player.x;
                        double currentFloorY = weight * floorYWall + (1.0 - weight) * player.y;

                        int floorTexX = int(currentFloorX * TEX_WIDTH) % TEX_WIDTH;
                        int floorTexY = int(currentFloorY * TEX_HEIGHT) % TEX_HEIGHT;
                        if (floorTexX < 0) floorTexX += TEX_WIDTH;
                        if (floorTexY < 0) floorTexY += TEX_HEIGHT;

                        framebuffer[pixelIndex] = ceilTexture[TEX_HEIGHT * floorTexY + floorTexX];
                    }
                } else if (y >= drawStart && y <= drawEnd) {
                    // ZED
                    int texY = (int)texPos & (TEX_HEIGHT - 1);
                    texPos += step;
                    
                    int texNumMap = worldMap[mapX][mapY];
                    if (texNumMap < 1 || texNumMap > 4) texNumMap = 1;
                    uint32_t color = textures[texNumMap][TEX_HEIGHT * texY + texX];
                    
                    if (side == 1) color = (color >> 1) & 8355711;
                    
                    framebuffer[pixelIndex] = color;
                } else {
                    // Podlaha
                    double p = y - screenHeight / 2.0;
                    if (p > 0) {
                        double currentDistFloor = (0.5 * screenHeight + player.z) / p;
                        double weight = currentDistFloor / distWall;
                        double currentFloorX = weight * floorXWall + (1.0 - weight) * player.x;
                        double currentFloorY = weight * floorYWall + (1.0 - weight) * player.y;

                        int floorTexX = int(currentFloorX * TEX_WIDTH) % TEX_WIDTH;
                        int floorTexY = int(currentFloorY * TEX_HEIGHT) % TEX_HEIGHT;
                        if (floorTexX < 0) floorTexX += TEX_WIDTH;
                        if (floorTexY < 0) floorTexY += TEX_HEIGHT;

                        framebuffer[pixelIndex] = floorTexture[TEX_HEIGHT * floorTexY + floorTexX];
                    }
                }
            }
        }
    };

    // Rozdáme práci vláknům (Multithreading)
    for (unsigned int i = 0; i < numThreads; ++i) {
        int start = i * chunk;
        int end = (i == numThreads - 1) ? screenWidth : start + chunk; // Poslední vlákno dojede do konce
        
        // emplace_back automaticky spustí std::jthread
        threads.emplace_back(renderChunk, start, end);
    }
    
    // Zde nemusíme volat threads.join(), protože std::jthread 
    // se na konci své platnosti (zde na konci funkce render) bezpečně spojí sám!
}

void Raycaster::renderSprites(std::vector<uint32_t>& framebuffer, int screenWidth, int screenHeight, const Player& player, std::vector<Sprite>& sprites, const std::vector<double>& zBuffer, const std::vector<EnemyDef>& enemyTypes, const std::vector<std::vector<SpriteFrame>>& projectileTypes) {
    std::vector<std::pair<double, int>> spriteOrder(sprites.size());
    for(size_t i = 0; i < sprites.size(); i++) {
        spriteOrder[i].first = ((player.x - sprites[i].x) * (player.x - sprites[i].x) + (player.y - sprites[i].y) * (player.y - sprites[i].y));
        spriteOrder[i].second = i;
    }
    std::sort(spriteOrder.rbegin(), spriteOrder.rend());

    for(size_t i = 0; i < sprites.size(); i++) {
        int spriteIdx = spriteOrder[i].second;

        if (sprites[spriteIdx].state == -1) continue;
        if (sprites[spriteIdx].isProjectile && sprites[spriteIdx].state == 0) continue;

        double spriteX = sprites[spriteIdx].x - player.x;
        double spriteY = sprites[spriteIdx].y - player.y;

        double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
        double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
        double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);

        int spriteScreenX = int((screenWidth / 2) * (1 + transformX / transformY));
        int vMoveScreen = int(0.0 / transformY);

        int spriteHeight = abs(int(screenHeight / (transformY)));
        int drawStartY = -spriteHeight / 2 + screenHeight / 2 + vMoveScreen;
        if (drawStartY < 0) drawStartY = 0;
        int drawEndY = spriteHeight / 2 + screenHeight / 2 + vMoveScreen;
        if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;

        int spriteWidth = abs(int(screenHeight / (transformY)));
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        if (drawStartX < 0) drawStartX = 0;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawEndX >= screenWidth) drawEndX = screenWidth - 1;

        int texWidth = TEX_WIDTH;
        int texHeight = TEX_HEIGHT;
        const uint32_t* texturePtr = nullptr;
        
        int type = sprites[spriteIdx].type;
        const std::vector<SpriteFrame>* animFrames = nullptr;
        
        if (sprites[spriteIdx].isProjectile) {
            if (type >= 0 && type < projectileTypes.size()) {
                animFrames = &projectileTypes[type];
            }
        } else {
            if (type >= 0 && type < enemyTypes.size()) {
                if (sprites[spriteIdx].state == 1) animFrames = &enemyTypes[type].idleFrames;
                else if (sprites[spriteIdx].state == 2) animFrames = &enemyTypes[type].painFrames;
                else animFrames = &enemyTypes[type].deathFrames;
            }
        }
        
        if (animFrames && !animFrames->empty()) {
            int frameIdx = sprites[spriteIdx].frameIndex;
            if (frameIdx < 0 || frameIdx >= animFrames->size()) frameIdx = 0;
            const auto& sf = (*animFrames)[frameIdx];
            texWidth = sf.w;
            texHeight = sf.h;
            texturePtr = sf.pixels.data();
        }

        if (!texturePtr) continue;

        for(int stripe = drawStartX; stripe < drawEndX; stripe++) {
            int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * texWidth / spriteWidth) / 256;
            if (transformY > 0 && stripe > 0 && stripe < screenWidth && transformY < zBuffer[stripe]) {
                for(int y = drawStartY; y < drawEndY; y++) {
                    int d = (y - vMoveScreen) * 256 - screenHeight * 128 + spriteHeight * 128;
                    int texY = ((d * texHeight) / spriteHeight) / 256;
                    uint32_t color = texturePtr[texWidth * texY + texX];
                    if ((color & 0xFF000000) != 0) {
                        framebuffer[y * screenWidth + stripe] = color;
                    }
                }
            }
        }
    }
}