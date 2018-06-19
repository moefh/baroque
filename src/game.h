/* game.h */

#ifndef GAME_H_FILE
#define GAME_H_FILE

#include "camera.h"
#include "gamepad.h"

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

void process_game_step(void);

extern struct GAMEPAD gamepad;
extern struct CAMERA camera;
extern struct FPS_COUNTER fps_counter;
extern struct CREATURE creatures[MAX_CREATURES];
extern float projection_fov;
extern float projection_aspect;

#endif /* GAME_H_FILE */
