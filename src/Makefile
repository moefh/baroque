
ifeq ($(OS),Windows_NT)
OS_CFLAGS = -I$(GLFW_HOME)/include
#OS_LDFLAGS = -w -Wl,-subsystem,windows
OS_LDFLAGS =
OS_LIBS = -L$(GLFW_HOME)/lib-mingw-w64 -lglfw3 -lgdi32 -lopengl32
else ifeq ($(shell uname),Darwin)
OS_CFLAGS = -pthread -g -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -pthread -g -fsanitize=address -fsanitize=undefined
OS_LIBS = -L. -L/usr/local/lib -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lpthread -ldl
else
OS_CFLAGS = -pthread -g -fsanitize=address -fsanitize=undefined
OS_LDFLAGS = -pthread -g -fsanitize=address -fsanitize=undefined
OS_LIBS = -L. -lglfw -lGL -lpthread -ldl
endif

CC = gcc
CFLAGS = $(OS_CFLAGS) -O2 -Wall -Wextra -Wno-unused-parameter -I../include
LDFLAGS = $(OS_LDFLAGS)

OBJS = main.o render.o bff.o gfx.o game.o model.o skeleton.o font.o shader.o debug.o glad.o gl_error.o \
       image.o matrix.o gamepad.o camera.o room.o file.o thread.o queue.o asset_loader.o
LIBS = $(OS_LIBS) -lm

all: game

clean:
	-rm -f *.o game game.exe out.txt

game: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
