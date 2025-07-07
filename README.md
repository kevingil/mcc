

## MC.C

![Voxel World](screenshots/image.png "Voxel World")

### Description

An open-world voxel game built with raylib, featuring open world gameplay with infinite terrain generation, block building, and first-person exploration. Built using a modular chunk system for efficient rendering and world management.

### Features

 - **First-person 3D exploration** with smooth WASD movement and mouse look
 - **Infinite voxel world** with chunk-based loading and procedural terrain generation
 - **Block interaction system** - place and destroy blocks with left/right mouse clicks
 - **Multiple block types** - grass, dirt, stone, wood, leaves, water
 - **Hotbar inventory** with 9 slots for different block types
 - **Realistic physics** - gravity, collision detection, and jumping
 - **Optimized rendering** - face culling, frustum culling, and efficient mesh generation
 - **Procedural terrain** - hills, valleys, water bodies, and tree generation

### Controls

Keyboard:
 - **WASD** - Move player
 - **SPACE** - Jump
 - **SHIFT** - Run/Sprint
 - **1-9** - Select block type from hotbar
 - **ESC** - Toggle cursor lock/unlock
 - **ENTER** - Return to main menu

Mouse:
 - **Mouse Movement** - Look around (first-person camera)
 - **Left Click** - Break/destroy blocks
 - **Right Click** - Place selected block


## Getting Started

### CMake

- Extract the zip of this project
- Type the follow command:

```sh
cmake -S . -B build
```

> if you want with debug symbols put the flag `-DCMAKE_BUILD_TYPE=Debug`

- After CMake config your project build:

```sh
cmake --build build
```

- Inside the build folder are another folder (named the same as the project name on CMakeLists.txt) with the executable and resources folder.
- In order for resources to load properly, cd to `src` and run the executable (`../build/${PROJECT_NAME}/${PROJECT_NAME}`) from there.

- cmake will automatically download a current release of raylib but if you want to use your local version you can pass `-DFETCHCONTENT_SOURCE_DIR_RAYLIB=<dir_with_raylib>` 
