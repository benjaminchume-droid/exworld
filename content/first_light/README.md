# EXWORLD: First Light

First Light is the first 3D game world packaged as an EXWORLD project on top of EXGINE.

The migrated project contains its `.exg` project manifest, scene data, and referenced mesh/material assets. The game systems live in EXWORLD (`src/` and `include/exworld/`) while EXGINE remains the engine/runtime dependency.

## World

- procedural terrain
- player and NPCs
- multi-floor outpost
- vehicle entity
- physics crate
- procedural rain/weather
- lighting
- procedural character animation
- spatial/procedural audio
- HUD entities
- save/runtime support through EXGINE

## Android

The Android shell uses EXGINE's native EGL/OpenGL ES presenter and mobile input bridge. The APK is built from this repository and checks out EXGINE as a separate engine dependency.

The CI artifact is a **signed debug APK**, so it is directly installable on an Android device. The previous EXGINE artifact was an unsigned release APK; that package was not installable.
