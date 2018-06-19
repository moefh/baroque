# baroque

A small 3D game engine in early development. Nothing much to see here yet.

## Compilation

This engine is pretty self-contained, the only external dependency is [GLFW](http://www.glfw.org/).

### Linux

Just make sure you have all the usual development stuff installed (including development libaries
for X, OpenGL, glfw, etc) then go to this project's directory and:

```
    $ cd src
    $ make
```

### Windows with MinGW-64

1. Create a directory to put the GLFW libararies and includes somewhere. This section assumes
this directory is `%HOME%\glfw`. Then:

```
    > cd %HOME%\glfw
    > mkdir lib
    > mkdir include
```

2. Download the [GLFW](http://www.glfw.org/) binaries, and:

  - copy the contents of `lib-mingw-w64` directory to the lib directory from step 1, `%HOME%\glfw\lib`.
  - copy the contents of `include` directory to the include directory from step 1, `%HOME%\glfw\include`.

3. In a command prompt, go to this project's directory and:

````
    > set DEVROOT=%HOME%\glfw
    > cd src
    > make
````

### Windows with Visual Studio

1. Create a directory to put the GLFW libararies and includes somewhere. This section assumes
this directory is `%HOME%\glfw`.

```
    > cd %HOME%\glfw
    > mkdir msvc-lib
    > mkdir msvc-include
```

2. Download the [GLFW](http://www.glfw.org/) binaries, and:

  - copy the contents of `lib-vc2015` directory to the lib directory from step 1, `%HOME%\glfw\msvc-lib`.
  - copy the contents of `include` directory to the include directory from step 1, `%HOME%\glfw\msvc-include`.

3. Open a MSVC command prompt, go to this project's directory and:

````
    > set DEVROOT=%HOME%\glfw
    > cd src
    > nmake -f Makefile.vc
````

4. To run, you'll need to copy the `glfw3.dll` from the GLFW binaries package to the directory
that contains `game.exe`.

## Running the game

The engine currently loads [glFT](https://www.khronos.org/gltf/) 2.0 binary files (`.glb`).
No assets are included yet, so you'll have to use your own to test it.
