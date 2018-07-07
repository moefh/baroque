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
    $ cd ../editor
    $ make
```

### Windows with MinGW-64

1. Download the GLFW binaries [GLFW](http://www.glfw.org/), and unpack
them somewhere. The next step assumes this directory is `%HOME%`
(usually `c:\users\USERNAME`), so the zip extraction created the
directory `%HOME%\glfw-X.Y.Z.bin.WIN64`.

2. In a command prompt, go to this project's directory and:

````
    > set GLFW_HOME=%HOME%\glfw-X.Y.Z.bin.WIN64
    > cd src
    > make
    > cd ..\editor
    > make
````

3. To run, you'll need to copy the `glfw3.dll` from the GLFW binaries package to the directory
that contains the generated `.exe` (i.e., `src` or `editor`).

### Windows with Visual Studio

1. Download the GLFW binaries [GLFW](http://www.glfw.org/), and unpack
them somewhere. The next step assumes this directory is `%HOME%`
(usually `c:\users\USERNAME`), so the zip extraction created the
directory `%HOME%\glfw-X.Y.Z.bin.WIN64`.

2. Open a MSVC command prompt, go to this project's directory and:

````
    > set GLFW_HOME=%HOME%\glfw-X.Y.Z.bin.WIN64
    > cd src
    > nmake -f Makefile.vc
    > cd ..\editor
    > nmake -f Makefile.vc
````

3. To run, you'll need to copy the `glfw3.dll` from the GLFW binaries package to the directory
that contains the generated `.exe` (i.e., `src` or `editor`).

## Running the game

The engine currently loads [glFT](https://www.khronos.org/gltf/) 2.0 binary files (`.glb`).
No assets are included yet, so you'll have to use your own to test it.

## Credits

Although the only external dependency is GLFW, this project includes code from other projects:

- [STB image](https://github.com/nothings/stb) for image loading
- [Glad](http://glad.dav1d.de/) to load OpenGL

