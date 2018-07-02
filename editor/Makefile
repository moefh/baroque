
ifeq ($(OS),Windows_NT)
OS_CFLAGS = -I$(DEVROOT)/include
#OS_LDFLAGS = -w -Wl,-subsystem,windows
OS_LDFLAGS =
OS_LIBS = -L$(DEVROOT)/lib -lglfw3 -lgdi32 -lopengl32
else
OS_CFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -g -fsanitize=address -fsanitize=undefined
OS_LIBS = -L. -lglfw -lGL -ldl
endif

CC = gcc
CFLAGS = $(OS_CFLAGS) -O2 -Wall -I../include
LDFLAGS = $(OS_LDFLAGS)

OBJS = main.o editor.o room.o save.o load.o render.o gfx.o debug.o glad.o gl_error.o image.o matrix.o grid.o font.o model.o gltf.o shader.o camera.o json.o
LIBS = $(OS_LIBS) -lm

all: editor

clean:
	-rm -f *.o editor editor.exe out.txt

editor: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
