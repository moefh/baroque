
ifeq ($(OS),Windows_NT)
OS_CFLAGS = -I$(DEVROOT)/include
#OS_LDFLAGS = -w -Wl,-subsystem,windows
OS_LDFLAGS =
OS_LIBS = -L$(DEVROOT)/lib -lglfw3 -lgdi32 -lopengl32
else
OS_CFLAGS = -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -fsanitize=address -fsanitize=undefined
OS_LIBS = -lglfw -lGL -ldl
endif

CC = gcc
CFLAGS = $(OS_CFLAGS) -O2 -Wall -I../include
LDFLAGS = $(OS_LDFLAGS)

OBJS = main.o render.o game.o model.o font.o gltf.o shader.o debug.o glad.o gl_error.o image.o matrix.o gamepad.o camera.o
LIBS = $(OS_LIBS) -lm

all: game

clean:
	-rm -f *.o game game.exe out.txt

game: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
