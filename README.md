# EXWORLD

**EXWORLD** is a GTA-style open-world game built on [EXGINE](https://github.com/benjaminchume-droid/exgine).

This is the actual game, not a demo.

## Current vertical slice (v0.2)

- **Vehicle enter/exit**  
  Walk up to a car \u2192 interact \u2192 ExAnimation enter clip + ExSound metal door + control switches to the vehicle.  
  Exit plays exit clip + door sound and returns control to on-foot.

- **Character controller + camera**  
  Uses EXGINE `CharacterControllerInput` when available, with kinematic fallback.  
  Third-person follow camera with separate on-foot / in-vehicle distances.

- **Building interiors + doors**  
  Approach a building door \u2192 interact \u2192 enter clip + wood door sound + interior mode.

- **Dense city block**  
  7 buildings, 5 vehicles, 6 pedestrians, continuous terrain, open-world streaming focus.

- **Wanted / free-roam foundation**  
  Stealing a car adds heat. Wanted levels 0\u20135 with decay. Alert sound on first heat.

- **Audio**  
  All driven by **ExSound**: footsteps (material + speed), engines, doors, city ambient, wanted UI.

- **Animation**  
  All driven by **ExAnimation**: idle / walk / run / sprint + one-shot enter/exit vehicle & building.

## Build

```bash
git clone https://github.com/benjaminchume-droid/exgine.git
git clone https://github.com/benjaminchume-droid/exworld.git
cd exworld
cmake -S . -B build -DEXGINE_ROOT=../exgine
cmake --build build -j
./build/exworld
```

## APK size (Android)

Because EXWORLD + EXGINE are pure native C++ with **procedural** content (no large texture/mesh packs):

| Build type              | Expected APK size      |
|-------------------------|------------------------|
| Release, stripped       | **18 \u2013 35 MB**         |
| Release + symbols       | 40 \u2013 60 MB             |
| Debug                   | 60 \u2013 90 MB             |

Once you add authored glTF characters, high-res textures, or audio samples the size will grow. The current procedural path stays very lean.

## License

Apache 2.0
