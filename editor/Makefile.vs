
CC = cl
CFLAGS = -I$(GLFW_HOME)/include -nologo -O2 -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -Drestrict= -I..\include
LDFLAGS = 

EDITOR_OBJS = main.obj editor.obj room.obj save.obj load.obj render.obj gfx.obj debug.obj glad.obj gl_error.obj matrix.obj grid.obj font.obj \
              model.obj gltf.obj shader.obj camera.obj json.obj base64.obj text.obj image.obj
EDITOR_LIBS = $(GLFW_HOME)\lib-vc2015\glfw3dll.lib

BUILDER_OBJS = builder.obj room.obj load.obj save_bff.obj matrix.obj model.obj gltf.obj json.obj base64.obj text_stdio.obj image.obj
BUILDER_LIBS =

all: editor.exe builder.exe

clean:
	-del *.obj editor.exe builder.exe

editor.exe: $(EDITOR_OBJS)
	$(CC) -Fe$@ $(EDITOR_OBJS) $(EDITOR_LIBS) $(LDFLAGS)

builder.exe: $(BUILDER_OBJS)
	$(CC) -Fe$@ $(BUILDER_OBJS) $(BUILDER_LIBS) $(LDFLAGS)

.c.obj:
	$(CC) $(CFLAGS) -c $<
