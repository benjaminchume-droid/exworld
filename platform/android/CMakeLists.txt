cmake_minimum_required(VERSION 3.20)
project(exworld_android LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "platform/android requires an Android NDK CMake toolchain")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Resolve EXGINE_ROOT: Gradle -D, env, then sibling probes
if(DEFINED EXGINE_ROOT AND EXISTS "${EXGINE_ROOT}/CMakeLists.txt")
  # already set via -DEXGINE_ROOT=...
elseif(DEFINED ENV{EXGINE_ROOT} AND EXISTS "$ENV{EXGINE_ROOT}/CMakeLists.txt")
  set(EXGINE_ROOT "$ENV{EXGINE_ROOT}")
else()
  # platform/android -> ../../../exgine  (CI: workspace/exgine next to workspace/exworld)
  set(_candidates
    "${CMAKE_CURRENT_LIST_DIR}/../../../exgine"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../exgine"
    "${CMAKE_CURRENT_LIST_DIR}/../../exgine"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../exgine"
  )
  set(EXGINE_ROOT "")
  foreach(_c IN LISTS _candidates)
    get_filename_component(_abs "${_c}" ABSOLUTE)
    if(EXISTS "${_abs}/CMakeLists.txt")
      set(EXGINE_ROOT "${_abs}")
      break()
    endif()
  endforeach()
endif()

if(NOT EXGINE_ROOT OR NOT EXISTS "${EXGINE_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR
    "EXGINE not found.\n"
    "  CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}\n"
    "  EXGINE_ROOT=${EXGINE_ROOT}\n"
    "  ENV EXGINE_ROOT=$ENV{EXGINE_ROOT}\n"
    "Pass -DEXGINE_ROOT=/path/to/exgine or place exgine as a sibling of exworld.")
endif()

message(STATUS "EXWORLD Android using EXGINE_ROOT=${EXGINE_ROOT}")

set(EXGINE_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(EXGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${EXGINE_ROOT}" "${CMAKE_BINARY_DIR}/exgine-core")

set(EXWORLD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
get_filename_component(EXWORLD_ROOT "${EXWORLD_ROOT}" ABSOLUTE)

set(ANDROID_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
set(ANDROID_GLUE_SOURCE "${ANDROID_GLUE_DIR}/android_native_app_glue.c")
if(NOT EXISTS "${ANDROID_GLUE_SOURCE}")
  message(FATAL_ERROR "Android NDK native_app_glue not found: ${ANDROID_GLUE_SOURCE}")
endif()

add_library(exworld_android SHARED
  src/main/cpp/native_activity.cpp
  ${ANDROID_GLUE_SOURCE}
  ${EXWORLD_ROOT}/src/game.cpp
  ${EXWORLD_ROOT}/src/player.cpp
  ${EXWORLD_ROOT}/src/vehicle.cpp
  ${EXWORLD_ROOT}/src/world.cpp
  ${EXWORLD_ROOT}/src/animation_driver.cpp
  ${EXWORLD_ROOT}/src/sound_director.cpp
  ${EXWORLD_ROOT}/src/camera.cpp
  ${EXWORLD_ROOT}/src/wanted.cpp
  ${EXWORLD_ROOT}/src/input.cpp
  ${EXWORLD_ROOT}/src/police.cpp
  ${EXWORLD_ROOT}/src/interior.cpp
  ${EXWORLD_ROOT}/src/save.cpp
)

target_include_directories(exworld_android PRIVATE
  ${ANDROID_GLUE_DIR}
  ${EXWORLD_ROOT}/include
  ${EXGINE_ROOT}/include
)

target_compile_features(exworld_android PRIVATE cxx_std_20)
target_compile_definitions(exworld_android PRIVATE EXWORLD_ANDROID=1)

target_link_libraries(exworld_android PRIVATE
  exgine_core
  android
  log
  EGL
  GLESv3
  aaudio
)
