This is SDL3, an extremely popular library used for building multimedia apps and games.

For classic MacOS 7/8/9 m68k and ppc, using Retro68. CodeWarrior support is broken but could be fixed "relatively" easily. 

https://github.com/laulandn/sdl3macos9
(Obviously Mac Finder file creators and types and resource forks are lost there.)

I discussed what I worked on and kept a running commentary over at System7Today for a while:
SDL2 for PPC MacOS 9 efforts...

This now includes a lot of work done by doctashay, which enabled the xash3d Half-life port for MacOS 9.  
https://github.com/doctashay/sdl2macos9/tree/os9-fixes


## Remaining limitations:

Crashes.

## Building the Classic Mac OS PowerPC target

The PowerPC build requires a Retro68 toolchain in `PATH`. The AGL declarations
needed by the OpenGL backend are included in the source tree.

```sh
make -C SDL-main -f Makefile.r68ppc
```

`Makefile.r68ppc` derives the Retro68 installation prefix from the compiler
and includes its `CursorDevicesGlue.o` compatibility object automatically.
Both paths remain overridable with `RETRO68_ROOT` and `CURSOR_DEVICES_GLUE`.

## Implementation references

The Classic backend follows Apple's published interfaces and lifecycle:

* [Processes (Inside Macintosh)](https://developer.apple.com/library/archive/documentation/mac/pdf/Processes/Intro_to_Procs_Tasks.pdf)
* [Imaging With QuickDraw](https://developer.apple.com/library/archive/documentation/mac/pdf/Imaging_With_QuickDraw/Imaging_LOF.pdf)
* [PixMap](https://developer.apple.com/documentation/applicationservices/pixmap)
* [DrawSprocket Programming Guide](https://leopard-adc.pepas.com/documentation/mac/Sprockets/GameSprockets-87.html)
* [DrawSprocket legacy reference](https://leopard-adc.pepas.com/documentation/Carbon/Reference/Games_Sprockets_Legacy/gamesprock_legacy_ref.pdf)
