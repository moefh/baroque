/* game.c */

#include <math.h>

#include "game.h"
#include "debug.h"
#include "gamepad.h"
#include "camera.h"
#include "matrix.h"

struct GAMEPAD gamepad;
struct CAMERA camera;
struct FPS_COUNTER fps_counter;
struct CREATURE creatures[MAX_CREATURES];

#define MOVE_SPEED (1.0 / 20.0)

// these should be configurable:
#define PAD_BTN_A      0
#define PAD_BTN_B      1
#define PAD_BTN_X      2
#define PAD_BTN_Y      3
#define PAD_BTN_LB     4
#define PAD_BTN_RB     5
#define PAD_BTN_SELECT 6
#define PAD_BTN_START  7
#define PAD_BTN_UP     10
#define PAD_BTN_RIGHT  11
#define PAD_BTN_DOWN   12
#define PAD_BTN_LEFT   13

#define PAD_AXIS_MOVE_X 0
#define PAD_AXIS_MOVE_Y 1
#define PAD_AXIS_CAM_X  2
#define PAD_AXIS_CAM_Y  3
#define PAD_AXIS_LT     4
#define PAD_AXIS_RT     5

#define CAM_SENSITIVITY_X (1.0 / 40.0)
#define CAM_SENSITIVITY_Y (1.0 / 40.0)

static void handle_input(void)
{
  if (update_gamepad(&gamepad) < 0)
    return;
  //dump_gamepad_state(&gamepad);
  
  if (gamepad.btn_pressed[PAD_BTN_A])
    console("%4.1f fps\n", fps_counter.fps);

  float cam_x = gamepad.axis[PAD_AXIS_CAM_X];
  float cam_y = gamepad.axis[PAD_AXIS_CAM_Y];
  if (fabsf(cam_x) > GAMEPAD_DEAD_ZONE)
    camera.theta += cam_x * CAM_SENSITIVITY_X;
  if (fabsf(cam_y) > GAMEPAD_DEAD_ZONE)
    camera.phi -= cam_y * CAM_SENSITIVITY_Y;
  camera.phi = clamp(camera.phi, 0.2, M_PI/2 - 0.2);

  float move_x = gamepad.axis[PAD_AXIS_MOVE_X];
  if (fabs(move_x) < GAMEPAD_DEAD_ZONE)
    move_x = 0.0;
  float move_y = gamepad.axis[PAD_AXIS_MOVE_Y];
  if (fabs(move_y) < GAMEPAD_DEAD_ZONE)
    move_y = 0.0;
  if (move_x != 0.0 || move_y != 0.0) {
    float front[3], left[3];
    get_camera_vectors(&camera, front, left);
    front[1] = 0;
    left[1] = 0;
    vec3_normalize(front);
    vec3_normalize(left);
    vec3_scale(left, -move_x);
    vec3_scale(front, move_y);

    float move[3];
    vec3_add(move, front, left);
    vec3_scale(move, MOVE_SPEED);
    vec3_add_to(creatures[0].pos, move);
    creatures[0].theta = atan2(move[2], move[0]) - M_PI/2;
  }
}

void process_game_step(void)
{
  handle_input();

  vec3_copy(camera.center, creatures[0].pos);
}
