/* gamepad.h */

#ifndef GAMEPAD_H_FILE
#define GAMEPAD_H_FILE

#define GAMEPAD_MAX_BUTTONS 16
#define GAMEPAD_MAX_AXES 8

#define GAMEPAD_DEAD_ZONE 0.1

struct GAMEPAD {
  int id;
  int n_buttons;
  int n_axes;
  unsigned char btn_state[GAMEPAD_MAX_BUTTONS];
  unsigned char btn_old[GAMEPAD_MAX_BUTTONS];
  unsigned char btn_pressed[GAMEPAD_MAX_BUTTONS];
  unsigned char btn_released[GAMEPAD_MAX_BUTTONS];
  float axis[GAMEPAD_MAX_AXES];
  float axis_old[GAMEPAD_MAX_AXES];
};

void init_gamepad(struct GAMEPAD *pad, int id);
int detect_gamepad(struct GAMEPAD *pad);
int update_gamepad(struct GAMEPAD *pad);
void dump_gamepad_state(struct GAMEPAD *pad);

#endif /* GAMEPAD_H_FILE */
