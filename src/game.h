/* game.h */

#ifndef GAME_H_FILE
#define GAME_H_FILE

struct FPS_COUNTER {
  double start_time;
  double fps;
  int n_frames;
};

struct GAMEPAD;

void handle_input(struct GAMEPAD *pad);

extern struct CAMERA camera;
struct FPS_COUNTER fps_counter;

#endif /* GAME_H_FILE */
