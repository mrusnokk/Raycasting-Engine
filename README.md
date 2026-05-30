# Retro Raycasting Engine in C++ 🎮

Welcome to this custom-built 3D Raycasting Engine written in raw C++! 
This project is deeply inspired by classic 90s shooters like *DOOM* and *Wolfenstein 3D*, built from scratch to render a pseudo-3D world using software raycasting and SDL2.

## 🌟 Key Features

* **Software Raycaster:** Fully custom raycasting engine written in C++ running on the CPU. It features rendering of textured walls, floors, ceilings, and dynamic depth-sorting.
* **8-Directional Sprites:** Full support for retro 8-angle sprite rendering. Enemies dynamically face the player just like in the original DOOM.
* **Advanced AI & Line of Sight:** Enemies use smart distance checking and rigorous line-of-sight algorithms to hunt down the player and engage in combat without "wallhacking".
* **Animated Weapons & Projectiles:** Complete state-machines for weapon bobbing, firing animations, and flying energetic projectiles.
* **Integrated Audio System:** Features a built-in sound mixer using `SDL_Audio` and `stb_vorbis` for playing multiple overlapping sounds (footsteps, enemy alerts, dynamic pain and death sounds, weapon firing).
* **Interactive Environment:** Functional minimap, sliding doors, and collision detection.

## 🛠️ Technology Stack

* **Language:** C++17
* **Graphics/Windowing:** SDL2 (Simple DirectMedia Layer)
* **Audio:** SDL_Audio + stb_vorbis
* **Build System:** CMake

## 🚀 How to Build

Make sure you have a modern C++ compiler (e.g., GCC, MSVC, Clang) and CMake installed. 
You will also need the SDL2 development libraries.

1. Clone this repository.
2. Ensure SDL2 is installed and correctly configured in your PATH or CMake environment.
3. Generate the build files:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
4. Compile the project:
   ```bash
   cmake --build .
   ```

## 🎮 How to Play

Run the compiled executable `voxel_project.exe`.

**Controls:**
* `W` `A` `S` `D` - Move and strafe
* `Mouse` - Look around (turn left / right)
* `Space` - Open doors
* `Left Click` - Fire weapon

## 📜 Credits
Developed as an advanced C++ study project exploring the magic of retro 3D rendering algorithms.
