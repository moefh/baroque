/* game.c */

#include <math.h>

#include "game.h"
#include "debug.h"
#include "gamepad.h"
#include "camera.h"
#include "matrix.h"

struct CAMERA camera;
struct FPS_COUNTER fps_counter;

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

#define MOVE_SPEED (1.0 / 20.0)

static float clamp(float val, float min, float max)
{
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}

#if 0
static void dump_gamepad(struct GAMEPAD *pad)
{
  int buttons_pressed = 0;
  for (int b = 0; b < pad->n_buttons; b++) {
    if (pad->btn_pressed[b]) {
      console("button %d pressed\n", b);
      buttons_pressed = 1;
    }
  }
  if (buttons_pressed) {
    for (int i = 0; i < pad->n_axes; i++)
      console("axis %d: %+f\n", i, pad->axis[i]);
  }
}
#endif

void handle_input(struct GAMEPAD *pad)
{
  if (update_gamepad(pad) < 0)
    return;

  //dump_gamepad(pad);
  
  if (pad->btn_pressed[PAD_BTN_A])
    console("%4.1f fps\n", fps_counter.fps);

  float cam_x = pad->axis[PAD_AXIS_CAM_X];
  float cam_y = pad->axis[PAD_AXIS_CAM_Y];
  if (fabsf(cam_x) > GAMEPAD_DEAD_ZONE)
    camera.theta += cam_x * CAM_SENSITIVITY_X;
  if (fabsf(cam_y) > GAMEPAD_DEAD_ZONE)
    camera.phi -= cam_y * CAM_SENSITIVITY_Y;
  camera.phi = clamp(camera.phi, 0.2, M_PI/2 - 0.2);

  float move_x = pad->axis[PAD_AXIS_MOVE_X];
  float move_y = pad->axis[PAD_AXIS_MOVE_Y];
  if (fabsf(move_x) > GAMEPAD_DEAD_ZONE || fabsf(move_y) > GAMEPAD_DEAD_ZONE) {
    float front[3], left[3];

    get_camera_vectors(&camera, front, left);
    front[1] = 0;
    vec3_normalize(front);
    left[1] = 0;
    vec3_normalize(left);
    
    if (fabsf(move_x) > GAMEPAD_DEAD_ZONE) {
      vec3_scale(left, -move_x * MOVE_SPEED);
      vec3_add_to(camera.center, left);
    }
    if (fabsf(move_y) > GAMEPAD_DEAD_ZONE) {
      vec3_scale(front, move_y * MOVE_SPEED);
      vec3_add_to(camera.center, front);
    }
  }

  
  //console("cam: %+f,%+f\n", pad->axis[PAD_AXIS_CAM_X], pad->axis[PAD_AXIS_CAM_Y]);
}
