/* editor.c */

#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "debug.h"
#include "camera.h"
#include "matrix.h"

#define MAX_COMMAND_ARGS  32
#define MAX_COMMAND_LEN   128

#define MOUSE_ACTION_NONE    0
#define MOUSE_ACTION_ROTATE  1
#define MOUSE_ACTION_MOVE    2

typedef void (*command_func)(int argc, char **argv);

struct EDITOR_COMMAND {
  const char *name;
  command_func func;
};

struct EDITOR_MOUSE_ACTION {
  int action;
  float mouse_x;
  float mouse_y;
  float start_x;
  float start_y;
  struct CAMERA camera;
};

struct EDITOR editor;

static struct EDITOR_COMMAND commands[];
static struct EDITOR_MOUSE_ACTION action;

static struct {
  int argc;
  char *argv[MAX_COMMAND_ARGS];
  char text[MAX_COMMAND_LEN];
} cmdline;

static int parse_command_line(const char *command)
{
  const char *in = command;
  char *out = cmdline.text;
  cmdline.argc = 0;
  
  while (*in != '\0') {
    while (*in == ' ')
      in++;
    if (*in == '\0')
      break;
    
    if (cmdline.argc >= MAX_COMMAND_ARGS - 1)
      return 1;
    cmdline.argv[cmdline.argc++] = out;
    while (*in != '\0' && *in != ' ')
      *out++ = *in++;
    *out++ = '\0';
  }

  cmdline.argv[cmdline.argc] = NULL;
  return 0;
}

static void run_command_line(const char *command)
{
  if (parse_command_line(command) != 0)
    return;

  for (int i = 0; commands[i].name != NULL; i++) {
    if (strcmp(commands[i].name, cmdline.argv[0]) == 0) {
      commands[i].func(cmdline.argc, cmdline.argv);
      return;
    }
  }
}

void init_editor(void)
{
  init_camera(&editor.camera);
  editor.input.active = 0;

  vec4_load(editor.grid_color, 0.4, 0.4, 0.4, 1);
  vec3_load(editor.grid_pos, 0, 0, 0);
}

void editor_handle_cursor_pos(double x, double y)
{
  action.mouse_x = x;
  action.mouse_y = y;

  if (action.action == MOUSE_ACTION_ROTATE) {
    float dx = x - action.start_x;
    float dy = y - action.start_y;
    editor.camera.theta = action.camera.theta + dx/100.0;
    editor.camera.phi = clamp(action.camera.phi + dy/100.0, -M_PI/2 + 0.1, M_PI/2 - 0.1);
    return;
  }

  if (action.action == MOUSE_ACTION_MOVE) {
    float dx = x - action.start_x;
    float dy = y - action.start_y;

    float front[3], left[3];
    get_camera_vectors(&editor.camera, front, left);
    front[1] = 0;
    left[1] = 0;
    vec3_normalize(front);
    vec3_normalize(left);
    vec3_scale(left,  dx);
    vec3_scale(front, dy);

    float move[3];
    vec3_add(move, front, left);
    vec3_scale(move, 0.05);
    vec3_add(editor.camera.center, action.camera.center, move);

    editor.camera.center[0] = floor(4.0 * editor.camera.center[0]) * 0.25;
    editor.camera.center[1] = floor(4.0 * editor.camera.center[1]) * 0.25;
    editor.camera.center[2] = floor(4.0 * editor.camera.center[2]) * 0.25;
    vec3_copy(editor.grid_pos, editor.camera.center);
  }
}

void editor_handle_mouse_button(int button, int press, int mods)
{
  //console("mouse button %d %s, mods=%x\n", button, (press) ? "pressed" : "released", mods);
  if (button == 2) {
    action.action = MOUSE_ACTION_NONE;
    if (! press)
      return;
    action.start_x = action.mouse_x;
    action.start_y = action.mouse_y;
    action.camera = editor.camera;
    if (mods & KEY_MOD_SHIFT)
      action.action = MOUSE_ACTION_MOVE;
    else
      action.action = MOUSE_ACTION_ROTATE;
  }
}

void editor_handle_char(unsigned int codepoint)
{
  struct EDITOR_INPUT_LINE *input = &editor.input;
  
  if (! input->active)
    return;

  if (codepoint < 32 || codepoint >= 127)
    return;

  if (input->line_len+1 >= MAX_EDIT_LINE_LEN)
    return;

  memmove(input->line + input->cursor_pos + 1, input->line + input->cursor_pos, input->line_len - input->cursor_pos + 1);
  input->line[input->cursor_pos] = codepoint;
  input->cursor_pos++;
  input->line_len++;
}

void editor_handle_key(int key, int press, int mods)
{
  struct EDITOR_INPUT_LINE *input = &editor.input;

  //console("key %d %s, mods=%x\n", key, (press) ? "pressed" : "released", mods);
  if (! press)
    return;

  switch (key) {
  case KEY_LEFT:
    if (mods & KEY_MOD_CTRL) {
      size_t pos = input->cursor_pos;
      while (pos > 0 && input->line[pos-1] == ' ') pos--;
      while (pos > 0 && input->line[pos-1] != ' ') pos--;
      input->cursor_pos = pos;
    } else {
      if (input->cursor_pos > 0)
        input->cursor_pos--;
    }
    break;

  case KEY_RIGHT:
    if (mods & KEY_MOD_CTRL) {
      size_t pos = input->cursor_pos;
      while (pos < input->line_len && input->line[pos] == ' ') pos++;
      while (pos < input->line_len && input->line[pos] != ' ') pos++;
      input->cursor_pos = pos;
    } else {
      if (input->cursor_pos < input->line_len)
        input->cursor_pos++;
    }
    break;
    
  case KEY_HOME:
    input->cursor_pos = 0;
    break;

  case KEY_END:
    input->cursor_pos = input->line_len;
    break;

  case KEY_DELETE:
    if (input->cursor_pos < input->line_len) {
      memmove(input->line + input->cursor_pos, input->line + input->cursor_pos + 1, input->line_len - input->cursor_pos);
      input->line_len--;
    }
    break;

  case KEY_BACKSPACE:
    if (input->cursor_pos > 0) {
      memmove(input->line + input->cursor_pos - 1, input->line + input->cursor_pos, input->line_len - input->cursor_pos + 1);
      input->cursor_pos--;
      input->line_len--;
    }
    break;

  case KEY_ESCAPE:
    if (input->active)
      input->active = 0;
    break;
    
  case KEY_ENTER:
    if (input->active) {
      run_command_line(input->line);
      input->active = 0;
    } else {
      input->line[0] = '\0';
      input->line_len = 0;
      input->cursor_pos = 0;
      input->active = 1;
    }
    break;

  case 'X':
    if (mods & KEY_MOD_CTRL)
      editor.quit = 1;
    break;

  default:
    break;
  }
}

int process_editor_step(void)
{
  return editor.quit;
}

static void cmd_quit(int argc, char **argv)
{
  editor.quit = 1;
}

static void cmd_sel(int argc, char **argv)
{
  if (! cmdline.argv[1])
    return;
  long num = strtol(cmdline.argv[1], NULL, 0);
  editor.selected_room = num;
}

static struct EDITOR_COMMAND commands[] = {
  { "quit", cmd_quit },
  { "exit", cmd_quit },
  { "sel", cmd_sel },
  { NULL }
};
