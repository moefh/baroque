/* editor.h */

#ifndef EDITOR_H_FILE
#define EDITOR_H_FILE

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

void editor_handle_cursor_pos(double x, double y);
void editor_handle_mouse_button(int button, int press, int mods);
int editor_handle_key(int key, int press, int mods);
void editor_handle_char(unsigned int codepoint);
void process_editor_step(void);

#define MAX_EDIT_LINE_LEN 128

struct EDITOR {
  int active;
  size_t cursor_pos;
  size_t line_len;
  char line[MAX_EDIT_LINE_LEN];
};

extern struct EDITOR editor;

#endif /* EDITOR_H_FILE */
