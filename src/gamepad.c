/* gamepad.c */

#include <string.h>
#include <GLFW/glfw3.h>

#include "gamepad.h"
#include "debug.h"

void init_gamepad(struct GAMEPAD *pad, int id)
{
  memset(pad, 0, sizeof(struct GAMEPAD));
  pad->id = id;
}

int detect_gamepad(struct GAMEPAD *pad)
{
  pad->id = -1;
  for (int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
    if (glfwJoystickPresent(i)) {
      debug("- Detected joystick %d: %s\n", i, glfwGetJoystickName(i));
      init_gamepad(pad, i);
      return 0;
    }
  }
  return -1;
}

int update_gamepad(struct GAMEPAD *pad)
{
  if (pad->id < 0)
    return -1;
  
  const unsigned char *buttons = glfwGetJoystickButtons(pad->id, &pad->n_buttons);
  if (buttons) {
    if (pad->n_buttons > GAMEPAD_MAX_BUTTONS)
      pad->n_buttons = GAMEPAD_MAX_BUTTONS;
    memcpy(pad->btn_state, buttons, pad->n_buttons);
  }
  
  const float *axes = glfwGetJoystickAxes(pad->id, &pad->n_axes);
  if (axes) {
    if (pad->n_axes > GAMEPAD_MAX_AXES)
      pad->n_axes = GAMEPAD_MAX_AXES;
    memcpy(pad->axis, axes, pad->n_axes*sizeof(float));
  }

  if (pad->n_buttons == 0 || pad->n_axes == 0) {
    debug("* Error reading joystick %d\n", pad->id);
    pad->id = -1;
    return -1;
  }

  for (int b = 0; b < pad->n_buttons; b++) {
    pad->btn_pressed[b]  = (pad->btn_state[b] && ! pad->btn_old[b]);
    pad->btn_released[b] = (! pad->btn_state[b] && pad->btn_old[b]);
  }

  memcpy(pad->btn_old, pad->btn_state, sizeof(pad->btn_old));
  memcpy(pad->axis_old, pad->axis, sizeof(pad->axis_old));
  return 0;
}

void dump_gamepad_state(struct GAMEPAD *pad)
{
  for (int b = 0; b < pad->n_buttons; b++) {
    if (pad->btn_pressed[b])
      console("button %d pressed\n", b);
  }
  for (int i = 0; i < pad->n_axes; i++)
    console("axis %d: %+f\n", i, pad->axis[i]);
}

