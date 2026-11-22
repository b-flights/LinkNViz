# Link N' Viz

A simple to use C visualization library with real time updates.

# About

Link N' Viz is an SDL based C library for creating simple visualizations using struct based templates. It is designed to be easy to integrate into any project with as little set up as possible. Visualizations can be updated in real time by modifying struct attributes.

# Requirements

SDL3 `https://github.com/libsdl-org/SDL/`<br>
SDL3-ttf `https://github.com/libsdl-org/SDL_ttf`

# Building

These instructions use CMake: `https://cmake.org/`

From the src folder, run `cmake -S . -B build`, with the following options depending on your platform and build system:

To specify the location of the SDL library, you can add the CMAKE_PREFIX_PATH command line argument as follows: `-DCMAKE_PREFIX_PATH=/path/to/libs`<br>
To specify a Makefile generator, you can use the -G option. For instance, using the MinGW GCC build system: `-G "MinGW Makefiles"`<br>
To build the example program, you can add the argument `-DBUILD_EXAMPLE=ON`.
