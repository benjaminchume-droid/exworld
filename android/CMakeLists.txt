cmake_minimum_required(VERSION 3.20)
project(exworld_android LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "EXWORLD Android build requires the Android NDK CMake toolchain")
endif()

if(NOT DEFINED EXGINE_ROOT)
  message(FATAL_ERROR "EXGINE_ROOT must point to the checked-out EXGINE repository")
endif()

set(EXGINE_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(EXGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${EXGINE_ROOT} ${CMAKE_BINARY_DIR}/exgine-core EXCLUDE_FROM_ALL)

set(ANDROID_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
set(ANDROID_GLUE_SOURCE "${ANDROID_GLUE_DIR}/android_native_app_glue.c")
if(NOT EXISTS "${ANDROID_GLUE_SOURCE}")
  message(FATAL_ERROR "Android NDK native_app_glue not found: ${ANDROID_GLUE_SOURCE}")
endif()

add_library(exworld_game STATIC
  ${CMAKE_CURRENT_LIST_DIR}/../src/game.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/player.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/vehicle.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/world.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/animation_driver.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/sound_director.cpp
)

target_include_directories(exworld_game PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/../include
  ${EXGINE_ROOT}/include
)
target_link_libraries(exworld_game PUBLIC exgine_core)
target_compile_features(exworld_game PUBLIC cxx_std_20)

add_library(exworld_android SHARED
  app/src/main/cpp/native_activity.cpp
  ${ANDROID_GLUE_SOURCE}
)

target_include_directories(exworld_android PRIVATE
  ${ANDROID_GLUE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../include
  ${EXGINE_ROOT}/include
)
target_compile_features(exworld_android PRIVATE cxx_std_20)
target_link_libraries(exworld_android PRIVATE
  exworld_game
  exgine_core
  android
  log
  EGL
  GLESv3
)
