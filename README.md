# Lights Please (WIP)

`Lights Please` is a work-in-progress C++20 game/graphics engine project using SDL2 + Vulkan.

## Current status

This repository is actively under development. The current app:

- initializes the engine and renderer
- loads OBJ assets (example: `models/Minion.obj`)
- includes a UUID-backed asset pipeline with mesh/texture importers, binary cache
  formats, async loading, dependency tracking, and timestamp-based hot reload
- creates Vulkan pipelines/materials/textures
- renders sample geometry (including axis meshes)
- exercises an ECS-style entity/component path in `main.cpp`

Roadmap notes live in [`Roamap.md`](./Roamap.md).

## Requirements

- CMake 3.16+
- C++20 compiler (Clang/GCC)
- Ninja (recommended generator)
- SDL2 development libraries
- Vulkan SDK/runtime

## Build

From the repository root:

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Or use the helper script:

```bash
./rebuild.sh
```

## Run

Run from the repository root so relative asset paths resolve:

```bash
./build/"Lights Please"
```

## Project layout

- `engine/` core engine systems (platform, renderer, ECS, memory, jobs)
- `shaders/` GLSL shaders and precompiled SPIR-V
- `models/` sample OBJ assets
- `textures/` sample textures
- `main.cpp` current integration/demo entrypoint

## Notes

- This README is intentionally lightweight while the project is still WIP.
- Interfaces and structure may change frequently.
