
# DEVROOT = $(HOME)\src\env

CC = cl
CFLAGS = -I$(DEVROOT)/msvc-include -nologo -O2 -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -Drestrict= -I..\include
LDFLAGS = 

OBJS = main.obj render.obj game.obj model.obj font.obj gltf.obj shader.obj debug.obj glad.obj \
       gl_error.obj image.obj matrix.obj gamepad.obj camera.obj
LIBS = $(DEVROOT)\msvc-lib\glfw3dll.lib

all: game.exe

clean:
	-del *.obj game.exe

game.exe: $(OBJS)
	$(CC) -Fe$@ $(OBJS) $(LIBS) $(LDFLAGS)

.c.obj:
	$(CC) $(CFLAGS) -c $<
