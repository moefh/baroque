/* editor.h */

#ifndef EDITOR_H_FILE
#define EDITOR_H_FILE

#include <stdint.h>
#include <stddef.h>

#include "debug.h"
#include "camera.h"
#include "room.h"

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
 
void init_editor(void);
void editor_handle_cursor_pos(double x, double y);
void editor_handle_mouse_button(int button, int press, int mods);
void editor_handle_mouse_scroll(double x, double y);
void editor_handle_key(int key, int press, int mods);
void editor_handle_char(unsigned int codepoint);
int process_editor_step(void);

void out_text(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define EDITOR_MAX_INPUT_LINE_LEN  128
#define EDITOR_SCREEN_LINES        50
#define EDITOR_SCREEN_COLS         100

struct EDITOR_INPUT_LINE {
  int active;
  size_t cursor_pos;
  size_t line_len;
  char line[EDITOR_MAX_INPUT_LINE_LEN];
};

struct EDITOR {
  int quit;
  const char *text_screen[EDITOR_SCREEN_LINES];
  struct EDITOR_INPUT_LINE input;

  struct EDITOR_ROOM *selected_room;
  struct EDITOR_ROOM_LIST rooms;
  float grid_pos[3];
  float grid_color[4];
  struct CAMERA camera;
};

extern struct EDITOR editor;

#endif /* EDITOR_H_FILE */
