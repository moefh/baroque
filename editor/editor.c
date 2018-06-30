/* editor.c */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "editor.h"
#include "debug.h"
#include "camera.h"
#include "matrix.h"
#include "model.h"
#include "gfx.h"
#include "render.h"

#define MAX_COMMAND_ARGS  32
#define MAX_COMMAND_LEN   128

#define MOUSE_BUTTON_LEFT   0
#define MOUSE_BUTTON_RIGHT  1
#define MOUSE_BUTTON_MIDDLE 2

#define MOUSE_ACTION_NONE            0
#define MOUSE_ACTION_ROTATE_CAM      1
#define MOUSE_ACTION_MOVE_CAM        2
#define MOUSE_ACTION_ZOOM_CAM        3
#define MOUSE_ACTION_MOVE_ROOM       4
#define MOUSE_ACTION_SEL_NEIGHBOR    5

#define MOUSE_ACTION_ALLOW_AXIS_X  (1<<0)
#define MOUSE_ACTION_ALLOW_AXIS_Y  (1<<1)
#define MOUSE_ACTION_ALLOW_AXIS_Z  (1<<2)

typedef void (*command_func)(const char *line, int argc, char **argv);

struct EDITOR_COMMAND {
  const char *name;
  command_func func;
  const char *description;
};

struct TEXT_SCREEN {
  char text[EDITOR_SCREEN_LINES*EDITOR_SCREEN_COLS];
  int cursor_col;
};

struct MOUSE_ACTION {
  int action_id;
  int allow_axis_flags;
  float mouse_x;
  float mouse_y;
  float start_x;
  float start_y;
  struct CAMERA camera;
  float room_pos[3];
  struct EDITOR_ROOM *hover_room;
  float hover_room_save_color[4];
};

struct EDITOR editor;

static const float room_normal_color[4] =   { 1.0, 1.0, 1.0, 0.25 };
static const float room_selected_color[4] = { 1.0, 1.0, 1.0, 0.80 };
static const float room_neighbor_color[4] = { 0.5, 0.5, 1.0, 0.75 };
static const float room_hover_color[4] =    { 1.0, 0.0, 0.0, 0.75 };

static const struct EDITOR_COMMAND commands[];
static struct MOUSE_ACTION action;
static struct TEXT_SCREEN screen;
static struct {
  int argc;
  char *argv[MAX_COMMAND_ARGS];
  char text[MAX_COMMAND_LEN];
} cmdline;

static struct EDITOR_ROOM *new_room(const char *name)
{
  struct EDITOR_ROOM *room = malloc(sizeof(*room));
  if (! room)
    return NULL;
  room->next = editor.room_list;
  editor.room_list = room;

  room->id = editor.next_room_id++;
  strncpy(room->name, name, sizeof(room->name));
  room->name[sizeof(room->name)-1] = '\0';

  room->n_neighbors = 0;
  room->n_tiles_x = 0;
  room->n_tiles_y = 0;
  vec3_load(room->pos, 0, 0, 0);
  vec4_load(room->display_color, 1,1,1,1);

  return room;
}

static int free_room(struct EDITOR_ROOM *room)
{
  struct EDITOR_ROOM **p = &editor.room_list;
  while (*p && *p != room)
    p = &(*p)->next;
  if (! *p)
    return 1;

  *p = room->next;
  free(room);
  return 0;
}

static void set_room_neighbor_color(struct EDITOR_ROOM *room, const float *color)
{
  for (int i = 0; i < room->n_neighbors; i++)
    vec4_copy(room->neighbors[i]->display_color, color);
}

static void select_room(struct EDITOR_ROOM *room)
{
  if (editor.selected_room && editor.selected_room != room) {
    vec4_copy(editor.selected_room->display_color, room_normal_color);
    set_room_neighbor_color(editor.selected_room, room_normal_color);
  }
  
  editor.selected_room = room;
  if (room) {
    vec4_copy(room->display_color, room_selected_color);
    set_room_neighbor_color(room, room_neighbor_color);
  }
}

static void toggle_room_neighbor(struct EDITOR_ROOM *room, struct EDITOR_ROOM *neighbor)
{
  if (room == NULL || neighbor == NULL || room == neighbor)
    return;
  for (int i = 0; i < room->n_neighbors; i++) {
    if (room->neighbors[i] == neighbor) {
      if (action.action_id == MOUSE_ACTION_SEL_NEIGHBOR && room->neighbors[i] == action.hover_room)
        vec4_copy(action.hover_room_save_color, room_normal_color);
      room->n_neighbors--;
      memmove(room->neighbors + i, room->neighbors + i + 1, (room->n_neighbors-i) * sizeof(*room->neighbors));
      if (room == editor.selected_room)
        select_room(room);
      return;
    }
  }
  if (room->n_neighbors+1 < EDITOR_ROOM_MAX_NEIGHBORS) {
    room->neighbors[room->n_neighbors++] = neighbor;
    if (room == editor.selected_room)
      select_room(room);
  }
}

static struct EDITOR_ROOM *get_room_by_name(const char *name)
{
  for (struct EDITOR_ROOM *room = editor.room_list; room != NULL; room = room->next) {
    if (strcmp(room->name, name) == 0)
      return room;
  }
  return NULL;
}

static struct EDITOR_ROOM *get_room_at_screen_pos(int screen_x, int screen_y)
{
  float pos[3], vec[3];
  get_screen_ray(&editor.camera, pos, vec, projection_fovy, screen_x, screen_y, viewport_width, viewport_height);
  
  // calculate click = intersection of line (vec+pos) with plane y=0
  float alpha = -pos[1] / vec[1];
  float click[3] = {
    alpha*vec[0] + pos[0],
    alpha*vec[1] + pos[1],
    alpha*vec[2] + pos[2],
  };

  // select room with center closest to click
  float sel_dist = 0;
  struct EDITOR_ROOM *sel = NULL;
  for (struct EDITOR_ROOM *room = editor.room_list; room != NULL; room = room->next) {
    float delta[3];
    vec3_sub(delta, room->pos, click);
    float dist = vec3_dot(delta, delta);
    if (sel == NULL || sel_dist > dist) {
      sel = room;
      sel_dist = dist;
    }
  }
  return sel;
}

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
  if (cmdline.argc == 0)
    return;

  out_text("> %s\n", command);
  for (int i = 0; commands[i].name != NULL; i++) {
    if (strcmp(commands[i].name, cmdline.argv[0]) == 0) {
      commands[i].func(command, cmdline.argc, cmdline.argv);
      return;
    }
  }
  out_text("** ERROR: unknown command\n");
}

static int autocomplete(char *line, size_t line_size, size_t *p_cursor_pos, const struct EDITOR_COMMAND **comp, size_t comp_size)
{
  size_t cursor_pos = *p_cursor_pos;
  size_t frag_pos = cursor_pos;
  while (frag_pos > 0 && line[frag_pos-1] != ' ')
    frag_pos--;
  size_t frag_len = cursor_pos - frag_pos;
  if (frag_len == 0)
    return 0;

  // find all possible completions
  size_t num_comp = 0;
  for (int i = 0; commands[i].name != NULL; i++) {
    if (memcmp(line + frag_pos, commands[i].name, frag_len) == 0) {
      if (num_comp >= comp_size)
        return -1;
      comp[num_comp++] = &commands[i];
    }
  }
  if (num_comp == 0)
    return 0;


  // find size of common text for all completions
  size_t common_size = strlen(comp[0]->name);
  for (int c = 1; c < num_comp; c++) {
    for (size_t p = frag_len; p < common_size; p++) {
      if (comp[c]->name[p] != comp[c-1]->name[p]) {
        common_size = p;
        break;
      }
    }
  }

  // add common text to line
  size_t fill_len = common_size - frag_len;
  if (fill_len > 0) {
    size_t total_len = strlen(line);
    if (fill_len + total_len >= line_size)
      return -1;
    memmove(line + cursor_pos + fill_len, line + cursor_pos, total_len - cursor_pos + 1);
    memcpy(line + cursor_pos, comp[0]->name + frag_len, fill_len);
    *p_cursor_pos = cursor_pos + fill_len;
  }

  return num_comp;
}

static void scroll_screen(int lines)
{
  if (lines == 0)
    return;
  if (lines < 0 || lines >= EDITOR_SCREEN_LINES) {
    memset(screen.text, 0, sizeof(screen.text));
    return;
  }
  memmove(screen.text, screen.text + EDITOR_SCREEN_COLS*lines, EDITOR_SCREEN_COLS*(EDITOR_SCREEN_LINES-lines));
  memset(screen.text + EDITOR_SCREEN_COLS*(EDITOR_SCREEN_LINES-lines), 0, EDITOR_SCREEN_COLS*lines);
}

static void clear_text_screen(void)
{
  memset(screen.text, 0, sizeof(screen.text));
}

void out_text(const char *fmt, ...)
{
  static char buf[1024];
  
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  const char *p = buf;
  while (*p) {
    if (*p == '\n' || screen.cursor_col >= EDITOR_SCREEN_COLS) {
      scroll_screen(1);
      screen.cursor_col = 0;
      if (*p == '\n') {
        p++;
        continue;
      }
    }
    screen.text[(EDITOR_SCREEN_LINES-1)*EDITOR_SCREEN_COLS + screen.cursor_col++] = *p++;
  }
}

void init_editor(void)
{
  editor.room_list = NULL;
  editor.selected_room = NULL;
  editor.next_room_id = 0;
  
  init_camera(&editor.camera);
  editor.camera.distance = 25;
  editor.input.active = 0;

  vec4_load(editor.grid_color, 0.4, 0.4, 0.4, 1);
  vec3_load(editor.grid_pos, 0, 0, 0);

  memset(screen.text, 0, sizeof(screen.text));
  for (int i = 0; i < EDITOR_SCREEN_LINES; i++)
    editor.text_screen[i] = &screen.text[i*EDITOR_SCREEN_COLS];

  out_text("Type ENTER help ENTER to get help\n");
}

void editor_handle_mouse_scroll(double x_off, double y_off)
{
  float zoom = 1.0 - y_off/20.0;
  editor.camera.distance *= zoom;
  editor.camera.distance = clamp(editor.camera.distance, 3.0, 100.0);
}

void editor_handle_cursor_pos(double x, double y)
{
  action.mouse_x = x;
  action.mouse_y = y;

  if (action.action_id == MOUSE_ACTION_ROTATE_CAM) {
    float dx = x - action.start_x;
    float dy = y - action.start_y;
    editor.camera.theta = action.camera.theta + dx/100.0;
    editor.camera.phi = clamp(action.camera.phi + dy/100.0, -M_PI/2 + 0.1, M_PI/2 - 0.1);
    return;
  }

  if (action.action_id == MOUSE_ACTION_MOVE_CAM) {
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

    editor.camera.center[0] = snap_to_grid(editor.camera.center[0], 0.25);
    editor.camera.center[1] = snap_to_grid(editor.camera.center[1], 0.25);
    editor.camera.center[2] = snap_to_grid(editor.camera.center[2], 0.25);
    vec3_copy(editor.grid_pos, editor.camera.center);
    return;
  }

  if (action.action_id == MOUSE_ACTION_MOVE_ROOM) {
    if (! editor.selected_room)
      return;
    float dx = x - action.start_x;
    float dy = y - action.start_y;

    float front[3], left[3], up[3];
    get_camera_vectors(&editor.camera, front, left);
    vec3_cross(up, front, left);
    if (! (action.allow_axis_flags & MOUSE_ACTION_ALLOW_AXIS_X)) {
      up[0] = 0;
      left[0] = 0;
    }
    if (! (action.allow_axis_flags & MOUSE_ACTION_ALLOW_AXIS_Y)) {
      up[1] = 0;
      left[1] = 0;
    }
    if (! (action.allow_axis_flags & MOUSE_ACTION_ALLOW_AXIS_Z)) {
      up[2] = 0;
      left[2] = 0;
    }
    vec3_scale(left,  -dx);
    vec3_scale(up, -dy);

    float move[3];
    vec3_add(move, up, left);
    vec3_scale(move, 0.04);
    vec3_add(editor.selected_room->pos, action.room_pos, move);

    editor.selected_room->pos[0] = snap_to_grid(editor.selected_room->pos[0], 0.25);
    editor.selected_room->pos[1] = snap_to_grid(editor.selected_room->pos[1], 0.25);
    editor.selected_room->pos[2] = snap_to_grid(editor.selected_room->pos[2], 0.25);
    return;
  }

  if (action.action_id == MOUSE_ACTION_SEL_NEIGHBOR) {
    struct EDITOR_ROOM *sel = get_room_at_screen_pos(action.mouse_x, action.mouse_y);
    if (sel != action.hover_room) {
      if (action.hover_room)
        vec4_copy(action.hover_room->display_color, action.hover_room_save_color);
      if (sel) {
        vec4_copy(action.hover_room_save_color, sel->display_color);
        vec4_copy(sel->display_color, room_hover_color);
        action.hover_room = sel;
      }
    }
  }
}

static void start_mouse_action(int action_id)
{
  action.action_id = action_id;
  action.start_x = action.mouse_x;
  action.start_y = action.mouse_y;
  action.camera = editor.camera;
  action.hover_room = NULL;
  if (editor.selected_room)
    vec3_copy(action.room_pos, editor.selected_room->pos);
  else
    vec3_load(action.room_pos, 0, 0, 0);
}

static void end_mouse_action(void)
{
  if (action.action_id == MOUSE_ACTION_SEL_NEIGHBOR && action.hover_room) {
    vec4_copy(action.hover_room->display_color, action.hover_room_save_color);
    select_room(editor.selected_room);
  }

  action.action_id = MOUSE_ACTION_NONE;
}

void editor_handle_mouse_button(int button, int press, int mods)
{
  //console("mouse button %d %s, mods=%x\n", button, (press) ? "pressed" : "released", mods);

  if (button == MOUSE_BUTTON_LEFT) {
    if (! press)
      return;

    switch (action.action_id) {
    case MOUSE_ACTION_MOVE_ROOM:
      end_mouse_action();
      break;

    case MOUSE_ACTION_SEL_NEIGHBOR:
      {
        struct EDITOR_ROOM *sel = get_room_at_screen_pos(action.mouse_x, action.mouse_y);
        if (sel)
          toggle_room_neighbor(editor.selected_room, sel);
        end_mouse_action();
      }
      break;
    }
    
    return;
  }

  if (button == MOUSE_BUTTON_RIGHT) {
    if (! press)
      return;
    
    switch (action.action_id) {
    case MOUSE_ACTION_MOVE_ROOM:
      vec3_copy(editor.selected_room->pos, action.room_pos);
      end_mouse_action();
      break;

    case MOUSE_ACTION_NONE:
      {
        struct EDITOR_ROOM *sel = get_room_at_screen_pos(action.mouse_x, action.mouse_y);
        select_room(sel);
      }
      break;

    default:
      end_mouse_action();
    }
    return;
  }
  
  if (button == MOUSE_BUTTON_MIDDLE) {
    if (! press) {
      if (action.action_id == MOUSE_ACTION_MOVE_CAM || action.action_id == MOUSE_ACTION_ROTATE_CAM)
        end_mouse_action();
      return;
    }

    switch (action.action_id) {
    case MOUSE_ACTION_NONE:
      if (mods & KEY_MOD_SHIFT)
        start_mouse_action(MOUSE_ACTION_MOVE_CAM);
      else
        start_mouse_action(MOUSE_ACTION_ROTATE_CAM);
      break;
    }
    return;
  }
}

void editor_handle_char(unsigned int codepoint)
{
  struct EDITOR_INPUT_LINE *input = &editor.input;
  
  if (! input->active)
    return;

  if (codepoint < 32 || codepoint >= 127)
    return;

  if (input->line_len+1 >= EDITOR_MAX_INPUT_LINE_LEN)
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
  case KEY_KP_DOT:
    if (! editor.input.active && editor.selected_room) {
      vec3_copy(editor.camera.center, editor.selected_room->pos);
      vec3_copy(editor.grid_pos, editor.camera.center);
    }
    break;
    
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

  case KEY_TAB:
    if (input->active) {
      const struct EDITOR_COMMAND *completions[16];
      autocomplete(editor.input.line, sizeof(editor.input.line), &editor.input.cursor_pos,
                   completions, sizeof(completions)/sizeof(*completions));
      editor.input.line_len = strlen(editor.input.line);
    } else {
      struct EDITOR_ROOM *sel = editor.selected_room;
      if (sel)
        sel = editor.selected_room->next;
      if (! sel)
        sel = editor.room_list;
      select_room(sel);
    }
    break;
    
  case KEY_ENTER:
    if (input->active) {
      run_command_line(input->line);
      input->active = 0;
    } else {
      end_mouse_action();
      input->line[0] = '\0';
      input->line_len = 0;
      input->cursor_pos = 0;
      input->active = 1;
    }
    break;

  case 'G':
    if (! editor.input.active && mods == 0 && editor.selected_room) {
      start_mouse_action(MOUSE_ACTION_MOVE_ROOM);
      action.allow_axis_flags = MOUSE_ACTION_ALLOW_AXIS_X | MOUSE_ACTION_ALLOW_AXIS_Z;
    }
    break;

  case 'N':
    if (! editor.input.active && mods == 0 && editor.selected_room)
      start_mouse_action(MOUSE_ACTION_SEL_NEIGHBOR);
    break;

  case 'X':
    if (action.action_id == MOUSE_ACTION_MOVE_ROOM && ! editor.input.active)
      action.allow_axis_flags = (mods & KEY_MOD_SHIFT) ? ~MOUSE_ACTION_ALLOW_AXIS_X : MOUSE_ACTION_ALLOW_AXIS_X;
    break;
    
  case 'Y':
    if (action.action_id == MOUSE_ACTION_MOVE_ROOM && ! editor.input.active)
      action.allow_axis_flags = (mods & KEY_MOD_SHIFT) ? ~MOUSE_ACTION_ALLOW_AXIS_Y : MOUSE_ACTION_ALLOW_AXIS_Y;
    break;
    
  case 'Z':
    if (action.action_id == MOUSE_ACTION_MOVE_ROOM && ! editor.input.active)
      action.allow_axis_flags = (mods & KEY_MOD_SHIFT) ? ~MOUSE_ACTION_ALLOW_AXIS_Z : MOUSE_ACTION_ALLOW_AXIS_Z;
    break;
    
  case 'Q':
    if (! editor.input.active && (mods & KEY_MOD_CTRL))
      editor.quit = 1;
    break;

  case 'L':
    if (mods & KEY_MOD_CTRL)
      clear_text_screen();
    break;

  default:
    //if (! editor.input.active)
    //  out_text("key: %d (%c)\n", key, (key >= 32 && key < 127) ? key : '-');
    break;
  }
}

int process_editor_step(void)
{
  return editor.quit;
}

static struct EDITOR_ROOM *add_room(const char *name)
{
  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename) != 0) {
    out_text("** ERROR: can't read model from '%s'\n", filename);
    return NULL;
  }

  struct EDITOR_ROOM *room = new_room(name);
  if (! room) {
    out_text("*** ERROR: out of memory for room\n");
    free_model(&model);
    return NULL;
  }
  
  if (gfx_upload_model(&model, GFX_MESH_TYPE_ROOM, room->id, room) != 0) {
    out_text("** ERROR: can't create model gfx\n");
    free_model(&model);
    return NULL;
  }

  free_model(&model);
  return room;
}

static void delete_room(struct EDITOR_ROOM *room)
{
  if (editor.selected_room == room)
    select_room(NULL);
  
  // remove room from everyone's neighbor list
  for (struct EDITOR_ROOM *p = editor.room_list; p != NULL; p = p->next) {
    for (int i = 0; i < p->n_neighbors;) {
      if (p->neighbors[i] == room)
        memcpy(&p->neighbors[i], &p->neighbors[i+1], sizeof(p->neighbors[0]) * (p->n_neighbors-1-i));
      else
        i++;
    }
  }

  gfx_free_meshes(GFX_MESH_TYPE_ROOM, room->id);
  free_room(room);
}

static void cmd_quit(const char *line, int argc, char **argv)
{
  editor.quit = 1;
}

static void cmd_cls(const char *line, int argc, char **argv)
{
  clear_text_screen();
}

static void cmd_sel(const char *line, int argc, char **argv)
{
  if (argc != 2) {
    out_text("USAGE: sel name\n");
    return;
  }
  const char *name = argv[1];
  
  struct EDITOR_ROOM *room = get_room_by_name(name);
  if (room) {
    select_room(room);
    out_text("- selected room '%s'\n", room->name);
    return;
  }

  out_text("** ERROR: room '%s' not found\n", name);
}

static void cmd_add(const char *line, int argc, char **argv)
{
  if (argc != 2) {
    out_text("USAGE: add name\n");
    return;
  }
  const char *name = argv[1];

  for (struct EDITOR_ROOM *room = editor.room_list; room != NULL; room = room->next) {
    if (strcmp(room->name, name) == 0) {
      out_text("** ERROR: room '%s' already exists\n", name);
      return;
    }
  }
  
  struct EDITOR_ROOM *room = add_room(name);
  if (room) {
    vec3_copy(room->pos, editor.camera.center);
    select_room(room);
    out_text("- added room '%s'\n", name);
  }
}

static void cmd_remove(const char *line, int argc, char **argv)
{
  if (argc != 2) {
    out_text("USAGE: remove name\n");
    out_text("   OR: remove .\n");
    return;
  }
  char name[EDITOR_ROOM_NAME_LEN];
  strncpy(name, argv[1], sizeof(name));
  name[sizeof(name)-1] = '\0';

  if (strcmp(name, ".") == 0) {
    if (! editor.selected_room) {
      out_text("** ERROR: no room selected\n");
      return;
    }
    strncpy(name, editor.selected_room->name, sizeof(name));
    name[sizeof(name)-1] = '\0';
  }
  
  struct EDITOR_ROOM *room = get_room_by_name(name);
  if (! room) {
    out_text("** ERROR: room '%s' not found\n", name);
    return;
  }

  delete_room(room);
  out_text("- removed room '%s'\n", name);
}

static void cmd_ls(const char *line, int argc, char **argv)
{
  int n_rooms = 0;
  for (struct EDITOR_ROOM *room = editor.room_list; room != NULL; room = room->next) {
    out_text("- [%d] %s\n", room->id, room->name);
    n_rooms++;
  }
  out_text("%d rooms\n", n_rooms);
}

static void cmd_info(const char *line, int argc, char **argv)
{
  out_text("camera:\n");
  out_text("  theta=%f, phi=%f, dist=%f\n", editor.camera.theta, editor.camera.phi, editor.camera.distance);
  out_text("  center=(%+f,%+f,%+f)\n", editor.camera.center[0], editor.camera.center[1], editor.camera.center[2]);
}

static void cmd_help(const char *line, int argc, char **argv)
{
  for (int i = 0; commands[i].name != NULL; i++)
    out_text("%-10s %s\n", commands[i].name, commands[i].description);
}

static void cmd_keys(const char *line, int argc, char **argv)
{
  out_text("\n");
  out_text("keys:\n");
  out_text("G       Grab (move) selected room\n");
  out_text("N       Add/remove neighbors (press N, left click on a room)\n");
  out_text("X,Y,Z   While moving room, limit movement to axis\n");
  out_text("CTRL+L  Clear text\n");

  out_text("\n");
  out_text("mouse:\n");
  out_text("LEFT    Confirm command\n");
  out_text("MIDDLE  Move (with SHIFT) or rotate camera\n");
  out_text("RIGHT   Cancel command / select room\n");
  out_text("SCROLL  Zoom\n");
}

static const struct EDITOR_COMMAND commands[] = {
  { "keys",     cmd_keys,     "Show key/mouse shortcuts" },
  { "quit",     cmd_quit,     "Quit" },
  { "cls",      cmd_cls,      "Clear text" },
  { "sel",      cmd_sel,      "Select room" },
  { "add",      cmd_add,      "Add room from model file" },
  { "remove",   cmd_remove,   "Remove room" },
  { "ls",       cmd_ls,       "List rooms" },
  { "info",     cmd_info,     "Get camera info" },
  { "help",     cmd_help,     "Show command list" },
  { NULL }
};
