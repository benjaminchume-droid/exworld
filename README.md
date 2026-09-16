# EXWORLD

**EXWORLD** is a full open-world GTA-style game built on top of the [EXGINE](https://github.com/benjaminchume-droid/exgine) engine.

This is **not** a tech demo. It is the game itself.

## Core Pillars

- Continuous 3D open world with streaming
- Player on-foot locomotion driven by **ExAnimation**
- Vehicle possession with enter / exit animations
- Enterable buildings
- Full procedural audio via **ExSound** (footsteps by material, engines, doors, ambient city, impacts)
- Day / night cycle, weather, free-roam gameplay foundation

## Requirements

- C++20
- CMake ≥ 3.20
- EXGINE (sibling directory or set `EXGINE_ROOT`)

## Build

```bash
# Clone both repos next to each other
git clone https://github.com/benjaminchume-droid/exgine.git
git clone https://github.com/benjaminchume-droid/exworld.git
cd exworld

cmake -S . -B build -DEXGINE_ROOT=../exgine
cmake --build build -j
```

Run:

```bash
./build/exworld
```

## Project Layout

```text
content/          Game project, scenes, materials, rules
src/              Game code (player, vehicles, world, animation, sound)
include/exworld/  Public game headers
```

## License

Apache 2.0 (same as EXGINE)
