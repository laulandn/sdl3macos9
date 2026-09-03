add_library(SDL3::SDL3-shared SHARED IMPORTED)
set_target_properties(SDL3::SDL3-shared PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "/Users/nick/sdl3macos9/SDL-main/include"
  IMPORTED_LOCATION "/Users/nick/sdl3macos9/SDL-main/libSDL3.dylib"
)
set(SDL3_FOUND TRUE)

