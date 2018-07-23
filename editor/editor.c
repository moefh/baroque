/* editor.c */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include "editor.h"
#include "save.h"
#include "load.h"
#include "debug.h"
#include "camera.h"
#include "matrix.h"
#include "model.h"
#include "gfx.h"
#include "render.h"

#define DEFAULT_MAP_FILE "data/world.json"

#define MAX_COMMAND_ARGS   32

#define MOUSE_BUTTON_LEFT   0
#define MOUSE_BUTTON_RIGHT  1
#define MOUSE_BUTTON_MIDDLE 2

#define MOUSE_ACTION_NONE            0
#define MOUSE_ACTION_ROTATE_CAM      1
#define MOUSE_ACTION_MOVE_CAM        2
#define MOUSE_ACTION_ZOOM_CAM        3
#define MOUSE_ACTION_MOVE_ROOM       4
#define MOUSE_ACTION_SEL_NEIGHBOR    5
#define MOUSE_ACTION_FILL_TILES      6

#define MOUSE_ACTION_ALLOW_AXIS_X  (1<<0)
#define MOUSE_ACTION_ALLOW_AXIS_Y  (1<<1)
#define MOUSE_ACTION_ALLOW_AXIS_Z  (1<<2)

typedef void (*command_func)(const char *line, int argc, char **argv);

struct EDITOR_COMMAND {
  const char *name;
  command_func func;
  const char *description;
};

static const struct EDITOR_COMMAND *get_editor_commands(void);

static struct {
  int loaded;
  struct MODEL_SKELETON skel;
  struct MODEL model;
} anim;

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
  uint16_t fill_tile_value;
  int start_tile_x;
  int start_tile_y;
  int end_tile_x;
  int end_tile_y;
  uint16_t save_tiles[256][256];
};

struct EDITOR editor;

static const float room_normal_color[4] =   { 1.0, 1.0, 1.0, 0.25 };
static const float room_selected_color[4] = { 1.0, 1.0, 1.0, 0.95 };
static const float room_neighbor_color[4] = { 0.5, 0.5, 1.0, 0.75 };
static const float room_hover_color[4] =    { 1.0, 0.0, 0.0, 0.75 };

static struct MOUSE_ACTION action;
static struct {
  int argc;
  char *argv[MAX_COMMAND_ARGS];
  char text[EDITOR_MAX_INPUT_LINE_LEN];
} cmdline;

static void set_room_neighbor_color(struct EDITOR_ROOM *room, const float *color)
{
  for (int i = 0; i < room->n_neighbors; i++)
    vec4_copy(room->neighbors[i]->display.color, color);
}

static void select_room(struct EDITOR_ROOM *room)
{
  if (editor.selected_room && editor.selected_room != room) {
    vec4_copy(editor.selected_room->display.color, room_normal_color);
    set_room_neighbor_color(editor.selected_room, room_normal_color);
  }
  
  editor.selected_room = room;
  if (room) {
    vec4_copy(room->display.color, room_selected_color);
    set_room_neighbor_color(room, room_neighbor_color);
  }
}

struct EDITOR_ROOM *get_room_at_screen_pos(int screen_x, int screen_y)
{
  float pos[3], vec[3];
  get_screen_ray(&editor.camera, pos, vec, screen_x, screen_y);
  
  // calculate click = intersection of line (vec+pos) with plane y=0
  if (fabs(vec[1]) < 0.00001)
    return NULL;
  float alpha = -pos[1] / vec[1];
  float click[3] = {
    alpha*vec[0] + pos[0],
    alpha*vec[1] + pos[1],
    alpha*vec[2] + pos[2],
  };

  // select room with center closest to click
  float sel_dist = 0;
  struct EDITOR_ROOM *sel = NULL;
  for (struct EDITOR_ROOM *room = editor.rooms.list; room != NULL; room = room->next) {
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


static int load_room_gfx(struct EDITOR_ROOM *room)
{
  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, 0) != 0) {
    out_text("** ERROR: can't read model from '%s'\n", filename);
    return 1;
  }

  if (gfx_upload_model(&model, GFX_MESH_TYPE_ROOM, room->display.gfx_id, room) != 0) {
    out_text("** ERROR: can't create model gfx\n");
    free_model(&model);
    return 1;
  }

  free_model(&model);
  return 0;
}

static void delete_room(struct EDITOR_ROOM *room)
{
  if (editor.selected_room == room)
    select_room(NULL);
  
  gfx_free_meshes(GFX_MESH_TYPE_ROOM, room->display.gfx_id);
  list_remove_room(&editor.rooms, room);
}

static struct EDITOR_ROOM *add_room(const char *name)
{
  struct EDITOR_ROOM *room = list_add_room(&editor.rooms, name);
  if (! room) {
    out_text("*** ERROR: out of memory for room\n");
    return NULL;
  }
  if (load_room_gfx(room) != 0) {
    list_remove_room(&editor.rooms, room);
    return NULL;
  }
  return room;
}

static int load_map(const char *filename)
{
  struct EDITOR_ROOM_LIST new_rooms;
  if (load_map_rooms(filename, &new_rooms) != 0)
    return 1;

  select_room(NULL);
  while (editor.rooms.list)
    delete_room(editor.rooms.list);

  int has_error = 0;
  struct EDITOR_ROOM *room = new_rooms.list;
  while (room != NULL) {
    struct EDITOR_ROOM *next = room->next;
    if (load_room_gfx(room) != 0) {
      list_remove_room(&editor.rooms, room);
      has_error = 1;
    } else {
      vec4_copy(room->display.color, room_normal_color);
    }
    room = next;
  }
  editor.rooms = new_rooms;

  for (struct EDITOR_ROOM *room = new_rooms.list; room != NULL; room = room->next)
    if (room->next == NULL)
      select_room(room);
  return has_error;
}

static void start_text_input(void)
{
  struct EDITOR_INPUT_LINE *input = &editor.input;
  input->line[0] = '\0';
  input->line_len = 0;
  input->cursor_pos = 0;
  input->active = 1;
}

static void start_mouse_action(int action_id)
{
  action.action_id = action_id;
  action.start_x = action.mouse_x;
  action.start_y = action.mouse_y;
  action.camera = editor.camera;

  switch (action_id) {
  case MOUSE_ACTION_MOVE_ROOM:
    action.allow_axis_flags = MOUSE_ACTION_ALLOW_AXIS_X | MOUSE_ACTION_ALLOW_AXIS_Z;
    if (editor.selected_room)
      vec3_copy(action.room_pos, editor.selected_room->pos);
    else
      vec3_load(action.room_pos, 0, 0, 0);
    break;

  case MOUSE_ACTION_SEL_NEIGHBOR:
    action.hover_room = NULL;
    break;

  case MOUSE_ACTION_FILL_TILES:
    action.fill_tile_value = 1;
    if (editor.selected_room)
      memcpy(action.save_tiles, editor.selected_room->tiles, sizeof(action.save_tiles));
    break;
  }
}

static void end_mouse_action(void)
{
  if (action.action_id == MOUSE_ACTION_SEL_NEIGHBOR && action.hover_room) {
    vec4_copy(action.hover_room->display.color, action.hover_room_save_color);
    select_room(editor.selected_room);
  }

  action.action_id = MOUSE_ACTION_NONE;
}

static int get_room_tile_at_screen_pos(struct EDITOR_ROOM *room, int screen_x, int screen_y, int *p_tile_x, int *p_tile_y)
{
  float pos[3], vec[3];
  get_screen_ray(&editor.camera, pos, vec, screen_x, screen_y);
  
  // calculate click = intersection of line (t*vec+pos) with plane y=room->pos[1]
  float room_normal[3] = { 0, 1, 0 };
  float denom = vec3_dot(room_normal, vec);
  if (fabs(denom) < 0.00001)
    return 1;  // screen ray (t*vec+pos) is parallel to plane
  float floor_vec[3];
  vec3_sub(floor_vec, room->pos, pos);
  float t = vec3_dot(room_normal, floor_vec) / denom;

  float click[3] = {
    t*vec[0] + pos[0],
    t*vec[1] + pos[1],
    t*vec[2] + pos[2],
  };

  *p_tile_x = floor(4 * (click[0] - room->pos[0]));
  *p_tile_y = floor(4 * (click[2] - room->pos[2]));
  return 0;
}

static int parse_int(const char *str, int *p_num)
{
  char *end;
  errno = 0;
  long n = strtol(str, &end, 0);
  if (errno != 0)
    return 1;
  if (n > INT_MAX || n < INT_MIN)
    return 1;
  *p_num = (int) n;
  return 0;
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

  const struct EDITOR_COMMAND *commands = get_editor_commands();
  
  add_input_history(command);
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
  const struct EDITOR_COMMAND *commands = get_editor_commands();
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
  for (size_t c = 1; c < num_comp; c++) {
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

void init_editor(int width, int height)
{
  editor.selected_room = NULL;
  init_room_list(&editor.rooms);
  
  init_camera(&editor.camera, width, height);
  editor.camera.distance = 25;
  editor.input.active = 0;

  vec4_load(editor.grid_color, 0.4, 0.4, 0.4, 1);
  vec3_load(editor.grid_pos, 0, 0, 0);

  clear_text_screen();
  for (int i = 0; i < TEXT_SCREEN_LINES; i++)
    editor.text_screen[i] = &text_screen.text[i*TEXT_SCREEN_COLS];

  init_input_history();
  
  out_text("Type /help to get help\n");
}

void editor_handle_mouse_scroll(double x_off, double y_off)
{
  float zoom = 1.0 - y_off/20.0;
  editor.camera.distance *= zoom;
  editor.camera.distance = clamp(editor.camera.distance, 3.0, 1000.0);
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
    get_camera_vectors(&editor.camera, front, left, NULL, NULL);
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
    get_camera_vectors(&editor.camera, front, left, up, NULL);
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
    if (sel && sel != action.hover_room) {
      if (action.hover_room)
        vec4_copy(action.hover_room->display.color, action.hover_room_save_color);
      if (sel) {
        vec4_copy(action.hover_room_save_color, sel->display.color);
        vec4_copy(sel->display.color, room_hover_color);
        action.hover_room = sel;
      }
    }
    return;
  }

  if (action.action_id == MOUSE_ACTION_FILL_TILES) {
    int tile_x, tile_y;
    struct EDITOR_ROOM *room = editor.selected_room;
    if (room && get_room_tile_at_screen_pos(room, action.mouse_x, action.mouse_y, &tile_x, &tile_y) == 0) {
      if (tile_x != action.end_tile_x || tile_y != action.end_tile_y) {
        memcpy(room->tiles, action.save_tiles, sizeof(room->tiles));
        action.end_tile_x = tile_x;
        action.end_tile_y = tile_y;
        fill_room_tiles(room, action.fill_tile_value, action.start_tile_x, action.start_tile_y, action.end_tile_x, action.end_tile_y);
        room->display.tiles_changed = 1;
      }
    }
    return;
  }
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

    case MOUSE_ACTION_FILL_TILES:
      end_mouse_action();
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

    case MOUSE_ACTION_FILL_TILES:
      if (editor.selected_room) {
        memcpy(editor.selected_room->tiles, action.save_tiles, sizeof(editor.selected_room->tiles));
        editor.selected_room->display.tiles_changed = 1;
      }
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
  
  if (! input->active) {
    if (codepoint == '/')
      start_text_input();
    return;
  }

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

  case KEY_UP:
    if (editor.input.active && input_history.size > 0) {
      strcpy(input->line, input_history.lines[input_history.last_view]);
      input->line_len = strlen(input->line);
      input->cursor_pos = input->line_len;
      input_history.last_view = (input_history.last_view - 1 + input_history.size) % input_history.size;
    }
    break;
    
  case KEY_DOWN:
    if (editor.input.active && input_history.size > 0) {
      strcpy(input->line, input_history.lines[input_history.last_view]);
      input->line_len = strlen(input->line);
      input->cursor_pos = input->line_len;
      input_history.last_view = (input_history.last_view + 1 + input_history.size) % input_history.size;
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
        sel = editor.rooms.list;
      select_room(sel);
      if (sel) {
        vec3_copy(editor.camera.center, sel->pos);
        vec3_copy(editor.grid_pos, editor.camera.center);
      }
    }
    break;
    
  case KEY_ENTER:
    if (input->active) {
      run_command_line(input->line);
      input->active = 0;
    } else {
      end_mouse_action();
      start_text_input();
    }
    break;

  case 'G':
    if (! editor.input.active && mods == 0 && editor.selected_room)
      start_mouse_action(MOUSE_ACTION_MOVE_ROOM);
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

  case 'F':
    if (action.action_id == MOUSE_ACTION_NONE && ! editor.input.active && editor.selected_room) {
      int tile_x, tile_y;
      if (get_room_tile_at_screen_pos(editor.selected_room, action.mouse_x, action.mouse_y, &tile_x, &tile_y) == 0) {
        start_mouse_action(MOUSE_ACTION_FILL_TILES);
        action.start_tile_x = action.end_tile_x = tile_x;
        action.start_tile_y = action.end_tile_y = tile_y;
        fill_room_tiles(editor.selected_room, action.fill_tile_value, tile_x, tile_y, tile_x, tile_y);
        editor.selected_room->display.tiles_changed = 1;
      }
      return;
    }
    if (action.action_id == MOUSE_ACTION_FILL_TILES && ! editor.input.active && editor.selected_room) {
      action.fill_tile_value = 1 - action.fill_tile_value;
      fill_room_tiles(editor.selected_room, action.fill_tile_value, action.start_tile_x, action.start_tile_y, action.end_tile_x, action.end_tile_y);
      editor.selected_room->display.tiles_changed = 1;
    }
    break;
    
  case 'L':
    if (mods & KEY_MOD_CTRL)
      clear_text_screen();
    break;

  case 'S':
    if ((mods & KEY_MOD_CTRL) && action.action_id == MOUSE_ACTION_NONE && ! editor.input.active)
      save_map_rooms(DEFAULT_MAP_FILE, &editor.rooms);
    break;

  case 'R':
    if ((mods & KEY_MOD_CTRL) && action.action_id == MOUSE_ACTION_NONE)
      load_map(DEFAULT_MAP_FILE);
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
  
  struct EDITOR_ROOM *room = get_room_by_name(&editor.rooms, name);
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

  for (struct EDITOR_ROOM *room = editor.rooms.list; room != NULL; room = room->next) {
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

static void cmd_anim(const char *line, int argc, char **argv)
{
  if (argc != 2) {
    out_text("USAGE: anim name\n");
    return;
  }
  const char *name = argv[1];

  if (anim.loaded) {
    gfx_free_meshes(GFX_MESH_TYPE_TEST, 0);
    free_model(&anim.model);
    free_model_skeleton(&anim.skel);
    anim.loaded = 0;
  }

  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", name);
  if (read_glb_animated_model(&anim.model, &anim.skel, filename, 0) != 0) {
    out_text("** ERROR: can't load '%s'\n", filename);
    return;
  }
  gfx_upload_model(&anim.model, GFX_MESH_TYPE_TEST, 0, &anim.skel);

  anim.loaded = 1;
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
  
  struct EDITOR_ROOM *room = get_room_by_name(&editor.rooms, name);
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
  for (struct EDITOR_ROOM *room = editor.rooms.list; room != NULL; room = room->next) {
    out_text("- %s\n", room->name);
    n_rooms++;
  }
  out_text("%d rooms\n", n_rooms);
}

static void cmd_wipe(const char *line, int argc, char **argv)
{
  struct EDITOR_ROOM *room = editor.selected_room;
  if (! room) {
    out_text("** ERROR: no room selected\n");
    return;
  }

  memset(room->tiles, 0, sizeof(room->tiles));
  room->display.tiles_changed = 1;
}

static void cmd_fill(const char *line, int argc, char **argv)
{
  if (argc != 6) {
    out_text("USAGE: fill VALUE X_FIRST Y_FIRST X_LAST Y_LAST\n");
    return;
  }

  struct EDITOR_ROOM *room = editor.selected_room;
  if (! room) {
    out_text("** ERROR: no room selected\n");
    return;
  }
  
  int value, x_first, y_first, x_last, y_last;
  if (parse_int(argv[1], &value) != 0 ||
      parse_int(argv[2], &x_first) != 0 ||
      parse_int(argv[3], &y_first) != 0 || 
      parse_int(argv[4], &x_last) != 0 || 
      parse_int(argv[5], &y_last) != 0) {
    out_text("** ERROR: bad numbers\n");
    return;
  }

  if (x_first > x_last || y_first > y_last) {
    out_text("** ERROR: invalid range\n");
    return;
  }
  
  fill_room_tiles(room, value, x_first, y_first, x_last, y_last);
  room->display.tiles_changed = 1;
}

static void cmd_history(const char *line, int argc, char **argv)
{
  if (input_history.last >= input_history.size)
    return;
  
  int pos = input_history.last;
  do {
    pos = (pos + 1) % input_history.size;
    out_text("%s\n", input_history.lines[pos]);
  } while (pos != input_history.last);
}

static void cmd_save(const char *line, int argc, char **argv)
{
  const char *filename;
  if (argc < 2)
    filename = DEFAULT_MAP_FILE;
  else
    filename = argv[1];

  save_map_rooms(filename, &editor.rooms);
}

static void cmd_load(const char *line, int argc, char **argv)
{
  const char *filename;
  if (argc < 2)
    filename = DEFAULT_MAP_FILE;
  else
    filename = argv[1];

  load_map(filename);
}

static void cmd_info(const char *line, int argc, char **argv)
{
  out_text("camera:\n");
  out_text("  theta=%f, phi=%f, dist=%f\n", editor.camera.theta, editor.camera.phi, editor.camera.distance);
  out_text("  center=(%+f,%+f,%+f)\n", editor.camera.center[0], editor.camera.center[1], editor.camera.center[2]);
}

static void cmd_gfxinfo(const char *line, int argc, char **argv)
{
  int n_meshes = 0;
  for (int i = 0; i < NUM_GFX_MESHES; i++) {
    if (gfx_meshes[i].use_count) {
      out_text("mesh[%d]: %d users\n", i, gfx_meshes[i].use_count);
      n_meshes++;
    }
  }

  int n_tex = 0;
  for (int i = 0; i < NUM_GFX_TEXTURES; i++) {
    if (gfx_textures[i].use_count) {
      out_text("tex[%d]: %d users\n", i, gfx_textures[i].use_count);
      n_tex++;
    }
  }

  out_text("%d meshes, %d textures used\n", n_meshes, n_tex);
}

static void cmd_help(const char *line, int argc, char **argv)
{
  const struct EDITOR_COMMAND *commands = get_editor_commands();
  for (int i = 0; commands[i].name != NULL; i++) {
    if (commands[i].description)
      out_text("%-10s %s\n", commands[i].name, commands[i].description);
  }
}

static void cmd_pleh(const char *line, int argc, char **argv)
{
  const struct EDITOR_COMMAND *commands = get_editor_commands();
  int n_commands = 0;
  while (commands[n_commands].name != NULL)
    n_commands++;
  
  for (int i = n_commands-1; i >= 0; i--) {
    if (commands[i].description) {
      const char *p = commands[i].name + strlen(commands[i].name);
      while (p-- > commands[i].name)
        out_text("%c", *p);
      for (int j = strlen(commands[i].name); j < 11; j++)
        out_text(" ");
      p = commands[i].description + strlen(commands[i].description);
      while (p-- > commands[i].description)
        out_text("%c", *p);
      out_text("\n");
    }
  }
}

static void cmd_keys(const char *line, int argc, char **argv)
{
  out_text("\n");
  out_text("keys:\n");
  out_text("TAB       Select next room\n");
  out_text("G         Grab (move) selected room\n");
  out_text("X, Y, Z   While moving room, limit movement to axis\n");
  out_text("F         Fill room tiles\n");
  out_text("N         Add/remove neighbor (press N, left click on a room)\n");
  out_text("/, ENTER  Open text command input\n");
  out_text("ESC       Close text command input\n");
  out_text("CTRL+L    Clear text\n");
  out_text("CTRL+S    Save map\n");

  out_text("\n");
  out_text("mouse:\n");
  out_text("LEFT      Confirm command\n");
  out_text("MIDDLE    Move (with SHIFT) or rotate camera\n");
  out_text("RIGHT     Cancel command / select room\n");
  out_text("SCROLL    Zoom\n");
}

static void cmd_sysinfo(const char *line, int argc, char **argv)
{
  out_text("Built with ");
  out_text(COMPILER_VERSION_FORMAT);
  out_text(" on %s %s\n", __DATE__, __TIME__);
}

static const struct EDITOR_COMMAND commands[] = {
  { "anim",     cmd_anim,     "Load animated model (for testing)" },

  { "save",     cmd_save,     "Save map" },
  { "load",     cmd_load,     "Load map" },
  { "keys",     cmd_keys,     "Show key/mouse shortcuts" },
  { "quit",     cmd_quit,     "Quit" },
  { "cls",      cmd_cls,      "Clear text" },
  { "sel",      cmd_sel,      "Select room" },
  { "add",      cmd_add,      "Add room from model file" },
  { "remove",   cmd_remove,   "Remove room" },
  { "ls",       cmd_ls,       "List rooms" },
  { "fill",     cmd_fill,     "Fill tiles of selected room" }, 
  { "wipe",     cmd_wipe,     "Wipe tiles of selected room" }, 
  { "history",  cmd_history,  "Show command history" }, 
  { "info",     cmd_info,     "Get camera info" },
  { "gfxinfo",  cmd_gfxinfo,  "Show gfx info" },
  { "help",     cmd_help,     "Show command list" },
  { "sysinfo",  cmd_sysinfo,  "Show system information" },
  { "pleh",     cmd_pleh,     "pleh sdrawkcab wohS" },
  { NULL }
};

static const struct EDITOR_COMMAND *get_editor_commands(void)
{
  return commands;
}
