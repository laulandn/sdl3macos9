# SPDX-License-Identifier: MIT OR Unlicense
cmake_minimum_required(VERSION 3.20)

project(after_egypt_sdl3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)

add_executable(after-egypt-sdl3
    src/main.cpp
)

target_link_libraries(after-egypt-sdl3 PRIVATE
    PkgConfig::SDL3
    PkgConfig::SDL3_TTF
)

target_compile_options(after-egypt-sdl3 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
