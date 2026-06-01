# Nastavení minimální verze CMake
cmake_minimum_required(VERSION 3.16)

# Název a jazyk projektu
project(voxel_project LANGUAGES CXX)

# Nekompromisně vyžadujeme C++20 (kvůli std::jthread atd.)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Chceme čistý standard bez vendor-specific rozšíření

# Hledání SDL3 knihovny (CMake se podívá do tvého MSYS2 prostředí)
find_package(SDL3 REQUIRED)

# Podpora pro vlákna (nezbytné primárně na Linuxu pro std::jthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_BUILD_TYPE Release)
# Přidání spustitelného souboru a zdrojových kódů
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/engine_core.cpp 
    src/engine_update.cpp 
    src/engine_render.cpp 
    src/engine_assets.cpp 
    src/raycaster.cpp
)

# Řekneme kompilátoru, kde má hledat hlavičkové soubory (.hpp a .h)
target_include_directories(${PROJECT_NAME} PRIVATE include lib)

# Přilinkování SDL3 a knihovny pro vlákna k našemu programu
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Threads::Threads)

# Nastavení přísných varování kompilátoru, aby nám neutekly chyby
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Zkopírování celé složky assets do složky build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)