/* game.c */

#include <math.h>

#include "game.h"
#include "debug.h"
#include "gamepad.h"
#include "camera.h"
#include "matrix.h"

float projection_aspect;
float projection_fov = 75*M_PI/180;
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
#define PAD_BTN_BACK   6
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

static float apply_dead_zone(float val)
{
  if (fabs(val) < GAMEPAD_DEAD_ZONE)
    return 0.0;
  return val;
}

static float apply_dead_zone_at(float val, float dead_zone_val)
{
  if (fabs(val - dead_zone_val) < GAMEPAD_DEAD_ZONE)
    return dead_zone_val;
  return val;
}

static void handle_input(void)
{
  if (update_gamepad(&gamepad) < 0)
    return;
  //dump_gamepad_state(&gamepad);
  
  if (gamepad.btn_pressed[PAD_BTN_BACK])
    console("%4.1f fps\n", fps_counter.fps);

  if (gamepad.btn_pressed[PAD_BTN_START])
    console("cam.dist=+%f, cam.theta=%+f, cam.phi=%+f, fov=%+f\n",
            camera.distance, camera.theta, camera.phi, projection_fov/M_PI*180);

  if (gamepad.btn_state[PAD_BTN_LB]) projection_fov -= 0.01;
  if (gamepad.btn_state[PAD_BTN_RB]) projection_fov += 0.01;
  projection_fov = clamp(projection_fov, M_PI/4, M_PI*0.9);
  
  float cam_x = apply_dead_zone(gamepad.axis[PAD_AXIS_CAM_X]);
  float cam_y = apply_dead_zone(gamepad.axis[PAD_AXIS_CAM_Y]);
  if (cam_x != 0.0 || cam_y != 0.0) {
    camera.theta += cam_x * CAM_SENSITIVITY_X;
    camera.phi -= cam_y * CAM_SENSITIVITY_Y;
    //camera.phi = clamp(camera.phi, 0.2, M_PI/2 - 0.2);
    float phi_min = -asin(camera.center[1] / camera.distance);
    camera.phi = clamp(camera.phi, phi_min + 0.05, M_PI/2 - 0.2);
  }

  float zoom_in = apply_dead_zone_at(gamepad.axis[PAD_AXIS_RT], -1.0);
  float zoom_out = apply_dead_zone_at(gamepad.axis[PAD_AXIS_LT], -1.0);
  if (zoom_in != -1.0 || zoom_out != -1.0) {
    zoom_in =  1.0 - (1.0 + zoom_in) * 0.01;
    zoom_out = 1.0 + (1.0 + zoom_out) * 0.01;
    camera.distance *= zoom_in * zoom_out;
    camera.distance = clamp(camera.distance, 1.0, 20.0);
    float phi_min = -asin(camera.center[1] / camera.distance);
    camera.phi = clamp(camera.phi, phi_min + 0.05, M_PI/2 - 0.2);
  }
  
  float move_x = apply_dead_zone(gamepad.axis[PAD_AXIS_MOVE_X]);
  float move_y = apply_dead_zone(gamepad.axis[PAD_AXIS_MOVE_Y]);
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
    vec3_scale(move, MOVE_SPEED * (gamepad.btn_state[PAD_BTN_A] ? 2.0 : 1.0));
    vec3_add_to(creatures[0].pos, move);
    creatures[0].theta = atan2(move[2], move[0]) - M_PI/2;
  }
}

void process_game_step(void)
{
  handle_input();

  vec3_load(camera.center,
            creatures[0].pos[0],
            creatures[0].pos[1] + 0.8,
            creatures[0].pos[2]);
}
