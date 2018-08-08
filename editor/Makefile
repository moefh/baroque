
ifeq ($(OS),Windows_NT)
OS_CFLAGS = -I$(GLFW_HOME)/include
#OS_LDFLAGS = -w -Wl,-subsystem,windows
OS_LDFLAGS =
OS_LIBS = -L$(GLFW_HOME)/lib-mingw-w64 -lglfw3 -lgdi32 -lopengl32
else ifeq ($(shell uname),Darwin)
OS_CFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LIBS = -L. -L/usr/local/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lglfw3 -ldl
else
OS_CFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LIBS = -L. -lglfw -lGL -ldl
endif

CC = gcc
CFLAGS = $(OS_CFLAGS) -O2 -Wall -Wextra -Wno-unused-parameter -I../include
LDFLAGS = $(OS_LDFLAGS)

EDITOR_OBJS = main.o editor.o room.o save.o load.o render.o gfx.o debug.o glad.o gl_error.o matrix.o grid.o font.o \
              model.o gltf.o shader.o camera.o json.o base64.o text.o image.o
EDITOR_LIBS = $(OS_LIBS) -lm

BUILDER_OBJS = builder.o room.o load.o bff.o matrix.o model.o gltf.o json.o base64.o text_stdio.o image.o
BUILDER_LIBS = -lm

all: editor builder

clean:
	-rm -f *.o editor editor.exe builder.exe out.txt

editor: $(EDITOR_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(EDITOR_OBJS) $(EDITOR_LIBS)

builder: $(BUILDER_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(BUILDER_OBJS) $(BUILDER_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
