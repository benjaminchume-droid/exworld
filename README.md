# EXWORLD

GTA-style open-world game on [EXGINE](https://github.com/benjaminchume-droid/exgine).

**100% procedural.** No authored glTF characters, no high-res texture packs, no sampled audio banks required.  
ExAnimation + ExSound + EXGINE generate the world, characters, vehicles, motion and audio.

## Status (v0.3)

| System | State |
|--------|--------|
| Vehicle enter/exit (ExAnimation + ExSound) | Done |
| VehicleDynamicsController possession | Done |
| Character controller + 3rd person camera | Done |
| Building doors + interior room nav | Done |
| Wanted levels + police AI chase | Done |
| Real input (keyboard / gamepad / touch) | Done |
| Save / load (player + wanted) | Done |
| Dense city + streaming | Done |
| **Installable Android APK** | **Not yet** |

## APK reality check

The game is **not** at the installable APK stage yet.

What exists:
- Full native C++ game logic
- EXGINE Android EGL / NativeActivity support (in the engine)

What is still missing for a real APK:
- `exworld` Android Gradle module + NativeActivity entry
- Wiring Android touch / lifecycle into `InputSystem`
- CMake/Gradle packaging + signing

Expected size once packaged (still fully procedural):

| Build | Size |
|-------|------|
| Release stripped | **18\u201335 MB** |
| Release + symbols | 40\u201360 MB |
| Debug | 60\u201390 MB |

## Build (desktop validation)

```bash
git clone https://github.com/benjaminchume-droid/exgine.git
git clone https://github.com/benjaminchume-droid/exworld.git
cd exworld
cmake -S . -B build -DEXGINE_ROOT=../exgine
cmake --build build -j
./build/exworld
```

## Controls (when input is connected)

| Action | Keyboard | Gamepad | Touch |
|--------|----------|---------|-------|
| Move | WASD | Left stick | Left virtual stick |
| Look | (mouse later) | Right stick | Right virtual stick |
| Sprint | Shift | RB | \u2014 |
| Interact (enter car/door) | E | A | Top-right zone |
| Exit vehicle/building | F | B | Bottom-right zone |
| Crouch / brake | Ctrl | LB | \u2014 |

## License

Apache 2.0
