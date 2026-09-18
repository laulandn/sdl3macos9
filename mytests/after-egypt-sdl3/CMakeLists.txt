# SPDX-License-Identifier: MIT OR Unlicense
cmake_minimum_required(VERSION 3.20)

project(after_egypt_sdl3 LANGUAGES CXX)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

set(CMAKE_C_COMPILER /Users/nick/Retro68-build/toolchain/bin/powerpc-apple-macos-gcc)
set(CMAKE_CXX_COMPILER /Users/nick/Retro68-build/toolchain/bin/powerpc-apple-macos-g++)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# WORKAROUND 1: Custom Manual Setup for SDL3
# ==============================================================================
set(MY_SDL3_DIR "/Users/nick/sdl3macos9/SDL-main")

if(NOT TARGET SDL3::Headers)
    add_library(SDL3::Headers INTERFACE IMPORTED GLOBAL)
    set_target_properties(SDL3::Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MY_SDL3_DIR}/include"
    )
endif()

if(NOT TARGET SDL3::SDL3)
    add_library(SDL3::SDL3 INTERFACE IMPORTED GLOBAL)
    set_target_properties(SDL3::SDL3 PROPERTIES
        INTERFACE_LINK_LIBRARIES "${MY_SDL3_DIR}/lib/libSDL3.a"
    )
    target_link_libraries(SDL3::SDL3 INTERFACE SDL3::Headers)
endif()

if(NOT TARGET SDL3::SDL3-shared)
    add_library(SDL3::SDL3-shared INTERFACE IMPORTED GLOBAL)
    target_link_libraries(SDL3::SDL3-shared INTERFACE SDL3::SDL3)
endif()


# ==============================================================================
# WORKAROUND 2: Custom Manual Setup for SDL3_ttf
# ==============================================================================
# Adjust this directory path if your built ttf files are stored in a different folder!
set(MY_SDL3_TTF_DIR "/Users/nick/Retro68-build/toolchain/powerpc-apple-macos")

if(NOT TARGET SDL3_ttf::SDL3_ttf)
    add_library(SDL3_ttf::SDL3_ttf INTERFACE IMPORTED GLOBAL)
    set_target_properties(SDL3_ttf::SDL3_ttf PROPERTIES
        # Point to the include headers folder for your specific SDL3_ttf
        INTERFACE_INCLUDE_DIRECTORIES "${MY_SDL3_TTF_DIR}/include"
        # Point to the specific library archive binary file (.a)
        INTERFACE_LINK_LIBRARIES "${MY_SDL3_TTF_DIR}/lib/libSDL3_ttf.a"
    )
endif()

# ==============================================================================
# EXECUTABLE & LINKING
# ==============================================================================
add_executable(after-egypt-sdl3
    src/main.cpp
)

# Link directly to your custom targets
target_link_libraries(after-egypt-sdl3 PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    "/Users/nick/sdl3macos9/dummy_dir/dir_dummy.o"
    "/Users/nick/sdl3macos9/dummy_libstdcpp/dummy_libstdcpp.o"
)

target_compile_options(after-egypt-sdl3 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
