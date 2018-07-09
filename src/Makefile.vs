
CC = cl
CFLAGS = -I$(GLFW_HOME)/include -nologo -O2 -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -Drestrict= -I..\include
LDFLAGS = 

OBJS = main.obj render.obj gfx.obj bff.obj game.obj model.obj font.obj shader.obj debug.obj glad.obj \
       gl_error.obj image.obj matrix.obj gamepad.obj camera.obj room.obj
LIBS = $(GLFW_HOME)\lib-vc2015\glfw3dll.lib

all: game.exe

clean:
	-del *.obj game.exe

game.exe: $(OBJS)
	$(CC) -Fe$@ $(OBJS) $(LIBS) $(LDFLAGS)

.c.obj:
	$(CC) $(CFLAGS) -c $<
