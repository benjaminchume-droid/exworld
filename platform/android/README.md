# EXWORLD Android

NativeActivity packaging for EXWORLD.

## Layout

```text
platform/android/
  app/                  Gradle application module
  src/main/cpp/         NativeActivity entry (exworld_android.so)
  CMakeLists.txt        Links game sources + EXGINE
```

Content from `content/` is shipped as Android assets.

## Local build

Requirements:
- Android SDK 35, NDK 27.2.12479018, CMake 3.22.1, JDK 17
- EXGINE checked out as sibling: `../exgine`

```bash
cd platform/android
gradle :app:assembleDebug
```

APK output:

```text
app/build/outputs/apk/debug/app-debug.apk
```

## CI

GitHub Actions workflow `.github/workflows/android.yml` builds the debug APK and uploads it as an artifact named `exworld-android-debug`.
