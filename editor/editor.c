/* editor.c */

#include <string.h>

#include "editor.h"
#include "debug.h"

struct EDITOR editor;

static int run_command(const char *command)
{
  if (strcmp(command, "quit") == 0)
    return 1;

  if (strcmp(command, "exit") == 0)
    return 1;
  
  return 0;
}

void editor_handle_cursor_pos(double x, double y)
{
}

void editor_handle_mouse_button(int button, int press, int mods)
{
  //console("mouse button %d %s, mods=%x\n", button, (press) ? "pressed" : "released", mods);
}

void editor_handle_char(unsigned int codepoint)
{
  if (! editor.active)
    return;

  if (codepoint < 32 || codepoint >= 127)
    return;

  if (editor.line_len+1 >= MAX_EDIT_LINE_LEN)
    return;

  memmove(editor.line + editor.cursor_pos + 1, editor.line + editor.cursor_pos, editor.line_len - editor.cursor_pos + 1);
  editor.line[editor.cursor_pos] = codepoint;
  editor.cursor_pos++;
  editor.line_len++;
}

int editor_handle_key(int key, int press, int mods)
{
  //console("key %d %s, mods=%x\n", key, (press) ? "pressed" : "released", mods);
  if (! press)
    return 0;

  switch (key) {
  case KEY_LEFT:
    if (mods & KEY_MOD_CTRL) {
      size_t pos = editor.cursor_pos;
      while (pos > 0 && editor.line[pos-1] == ' ') pos--;
      while (pos > 0 && editor.line[pos-1] != ' ') pos--;
      editor.cursor_pos = pos;
    } else {
      if (editor.cursor_pos > 0)
        editor.cursor_pos--;
    }
    return 0;

  case KEY_RIGHT:
    if (mods & KEY_MOD_CTRL) {
      size_t pos = editor.cursor_pos;
      while (pos < editor.line_len && editor.line[pos] == ' ') pos++;
      while (pos < editor.line_len && editor.line[pos] != ' ') pos++;
      editor.cursor_pos = pos;
    } else {
      if (editor.cursor_pos < editor.line_len)
        editor.cursor_pos++;
    }
    return 0;
    
  case KEY_HOME:
    editor.cursor_pos = 0;
    return 0;

  case KEY_END:
    editor.cursor_pos = editor.line_len;
    return 0;

  case KEY_DELETE:
    if (editor.cursor_pos < editor.line_len) {
      memmove(editor.line + editor.cursor_pos, editor.line + editor.cursor_pos + 1, editor.line_len - editor.cursor_pos);
      editor.line_len--;
    }
    return 0;

  case KEY_BACKSPACE:
    if (editor.cursor_pos > 0) {
      memmove(editor.line + editor.cursor_pos - 1, editor.line + editor.cursor_pos, editor.line_len - editor.cursor_pos + 1);
      editor.cursor_pos--;
      editor.line_len--;
    }
    return 0;

  case KEY_ESCAPE:
    if (editor.active)
      editor.active = 0;
    return 0;
    
  case KEY_ENTER:
    if (editor.active) {
      if (run_command(editor.line) != 0)
        return 1;
      editor.active = 0;
    } else {
      editor.line[0] = '\0';
      editor.line_len = 0;
      editor.cursor_pos = 0;
      editor.active = 1;
    }
    return 0;

  case 'X':
    if (mods & KEY_MOD_CTRL)
      return 1;
    return 0;

  default:
    return 0;
  }
}

void process_editor_step(void)
{
}
