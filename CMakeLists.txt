cmake_minimum_required(VERSION 3.20)
project(exworld VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED EXGINE_ROOT)
  set(EXGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../exgine" CACHE PATH "Path to EXGINE source root")
endif()

if(NOT EXISTS "${EXGINE_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "EXGINE not found at ${EXGINE_ROOT}. Set -DEXGINE_ROOT=...")
endif()

if(NOT TARGET exgine)
  add_subdirectory("${EXGINE_ROOT}" "${CMAKE_BINARY_DIR}/exgine" EXCLUDE_FROM_ALL)
endif()

add_executable(exworld
  src/main.cpp
  src/game.cpp
  src/player.cpp
  src/vehicle.cpp
  src/world.cpp
  src/animation_driver.cpp
  src/sound_director.cpp
  src/camera.cpp
  src/wanted.cpp
)

target_include_directories(exworld PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${EXGINE_ROOT}/include
)

target_link_libraries(exworld PRIVATE exgine)

add_custom_command(TARGET exworld POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/content
    $<TARGET_FILE_DIR:exworld>/content
)
