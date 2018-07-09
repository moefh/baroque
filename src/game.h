/* game.h */

#ifndef GAME_H_FILE
#define GAME_H_FILE

#include "camera.h"
#include "gamepad.h"

#define KEY_MOD_SHIFT   (1<<0)
#define KEY_MOD_CTRL    (1<<1)
#define KEY_MOD_ALT     (1<<2)

#define KEY_ESCAPE      256
#define KEY_ENTER       257
#define KEY_TAB         258
#define KEY_BACKSPACE   259
#define KEY_INSERT      260
#define KEY_DELETE      261

#define KEY_RIGHT       262
#define KEY_LEFT        263
#define KEY_DOWN        264
#define KEY_UP          265

#define KEY_PGUP        266
#define KEY_PGDOWN      267
#define KEY_HOME        268
#define KEY_END         269

#define KEY_KP_0        320
#define KEY_KP_1        321
#define KEY_KP_2        322
#define KEY_KP_3        323
#define KEY_KP_4        324
#define KEY_KP_5        325
#define KEY_KP_6        326
#define KEY_KP_7        327
#define KEY_KP_8        328
#define KEY_KP_9        329
#define KEY_KP_DOT      330
#define KEY_KP_DIVIDE   331
#define KEY_KP_MULTIPLY 332
#define KEY_KP_SUBTRACT 333
#define KEY_KP_ADD      334
#define KEY_KP_ENTER    335

#define MAX_CREATURES 16

struct FPS_COUNTER {
  double start_time;
  double fps;
  int n_frames;
};

struct CREATURE {
  float pos[3];
  float theta;
};

struct GAME {
  int quit;
  int show_camera_info;
  struct CAMERA camera;
  struct CREATURE creatures[MAX_CREATURES];
  struct ROOM *current_room;
};

int init_game(int width, int height);
void close_game(void);
void handle_game_key(int key, int press, int mods);
int process_game_step(void);
void get_light_pos(float *light_pos);

extern struct GAME game;
extern struct GAMEPAD gamepad;
extern struct FPS_COUNTER fps_counter;

#endif /* GAME_H_FILE */
