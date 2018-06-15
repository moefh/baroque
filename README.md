# baroque

A small 3D game engine in early development. Nothing much to see here yet.

## Compilation

This engine is pretty self-contained, the only external dependency is [GLFW](http://www.glfw.org/).

The Makefile should work on Windows (using mingw) and Linux. On Windows, set the
environment variable `DEVROOT` to a directory containing an installation of GLFW
includes and libraries in `%DEVROOT%\include` and `%DEVROOT%\lib`.
