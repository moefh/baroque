/* game.c */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "game.h"
#include "debug.h"
#include "gamepad.h"
#include "camera.h"
#include "matrix.h"
#include "bff.h"
#include "room.h"
#include "asset_loader.h"
#include "model.h"

struct GAME game;
struct GAMEPAD gamepad;
struct FPS_COUNTER fps_counter;

static struct BWF_READER bwf_reader;
static int bwf_reader_open;

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

void get_light_pos(float *restrict light_pos)
{
  float camera_pos[3];
  get_camera_pos(&game.camera, camera_pos);

  float camera_dir[3] = {
    camera_pos[0] - game.camera.center[0],
    camera_pos[1] - game.camera.center[1],
    camera_pos[2] - game.camera.center[2],
  };
  vec3_normalize(camera_dir);
  vec3_scale(camera_dir, 4.0);
  light_pos[0] = camera_pos[0] + camera_dir[0];
  light_pos[1] = camera_pos[1] + 4.0;
  light_pos[2] = camera_pos[2] + camera_dir[2];
}

void handle_game_key(int key, int press, int mods)
{
  if (key == 'Q' && (mods & KEY_MOD_CTRL))
    game.quit = 1;
}

static void unload_unused_rooms(bool unload_all)
{
  mark_all_rooms(0);

  if (game.current_room) {
    game.current_room->mark = 1;
    for (int i = 0; i < game.current_room->n_neighbors; i++)
      game.current_room->neighbor[i]->mark = 1;
  }

  while (1) {
    struct ROOM *unused = get_marked_room(0);
    if (! unused)
      break;
    gfx_free_meshes(GFX_MESH_TYPE_ROOM, unused->index);
    free_room(unused);
    if (! unload_all)
      break;
  }
}

static struct ROOM *load_room(int room_index)
{
  struct ROOM *room = alloc_room();
  if (! room) {
    debug("** ERROR: out of memory for room %d\n", room_index);
    return NULL;
  }
  room->index = room_index;
  if (load_bwf_room(&bwf_reader, room) != 0) {
    debug("** ERROR: can't load room\n");
    close_bwf(&bwf_reader);
    bwf_reader_open = 0;
    return NULL;
  }
  for (int i = 0; i < room->n_neighbors; i++)
    room->neighbor[i] = NULL;
  return room;
}

static int load_room_neighbors(struct ROOM *room)
{
  for (int i = 0; i < room->n_neighbors; i++) {
    room->neighbor[i] = get_room_by_index(room->neighbor_index[i]);
    if (room->neighbor[i])
      continue;

    room->neighbor[i] = load_room(room->neighbor_index[i]);
    if (room->neighbor[i])
      continue;

    debug("** ERROR: can't load room %d (neighbor of %d)\n", room->neighbor_index[i], room->index);
    return 1;
  }
  return 0;
}

static int set_current_room(int room_index)
{
  struct ROOM *room = get_room_by_index(room_index);
  if (! room) {
    if (! has_free_room())
      unload_unused_rooms(false);
    room = load_room(room_index);
    if (! room)
      return 1;
  }
  game.current_room = room;
  if (load_room_neighbors(room) != 0)
    return 1;
  unload_unused_rooms(true);
  return 0;
}

static void check_room_change(void)
{
  /*
   * HACK: just find the closest loaded room to the player character
   * and make it the current room. We really should use the tiles to
   * check for room transitions, but that's not yet implemented in the
   * editor).
   */
  float *player_pos = game.creatures[0].pos;
  
  struct ROOM *closest_room = NULL;
  float closest_dist = 0;
  for (struct ROOM *room = get_room_list(); room != NULL; room = room->next) {
    float dist = ((room->pos[0] - player_pos[0]) * (room->pos[0] - player_pos[0]) +
                  (room->pos[1] - player_pos[1]) * (room->pos[1] - player_pos[1]) +
                  (room->pos[2] - player_pos[2]) * (room->pos[2] - player_pos[2]));
    if (closest_room == NULL || dist < closest_dist) {
      closest_room = room;
      closest_dist = dist;
    }
  }

  if (closest_room && closest_room != game.current_room)
    set_current_room(closest_room->index);
}

static int load_player_model(void)
{
  if (load_bmf("data/player.bmf", GFX_MESH_TYPE_CREATURE, 0, NULL) != 0) {
    debug("** ERROR: can't load player model from data/player.bmf\n");
    return 1;
  }

  struct SKELETON skel;
  if (load_bcf("data/test1.bcf", &skel, GFX_MESH_TYPE_CREATURE, 1, NULL) != 0) {
    debug("** ERROR: can't load player model from data/test1.bcf\n");
    return 1;
  }
  static float matrices[16*SKELETON_MAX_BONES];
  get_skeleton_matrices(&skel, 0, 0.0, matrices);
  for (int i = 0; i < skel.n_bones; i++) {
    console("skel matrix [%d]:\n", i);
    mat4_dump(&matrices[16*i]);
  }
  free_skeleton(&skel);
  return 0;
}

static int init_rooms(void)
{
  init_room_store();
  if (open_bwf(&bwf_reader, "data/world.bwf") != 0) {
    debug("** ERROR: can't open data/world.bmf\n");
    return 1;
  }
  bwf_reader_open = 1;
  if (set_current_room(0) != 0)
    return 1;

  return 0;
}

int init_game(int width, int height)
{
  game.quit = 0;

  debug("- Starting asset loader thread...\n");
  if (start_asset_loader() != 0)
    return 0;
    
  init_camera(&game.camera, width, height);
  game.camera.distance = 6.0;

  if (load_player_model() != 0)
    return 1;
  
  if (init_rooms() != 0)
    return 1;

  vec3_copy(game.creatures[0].pos, game.current_room->pos);
  return 0;
}

void close_game(void)
{
  debug("- Stopping asset loader...\n");
  stop_asset_loader();
  
  if (bwf_reader_open)
    close_bwf(&bwf_reader);
}

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
  
  game.show_camera_info = gamepad.btn_state[PAD_BTN_START];
  
  if (gamepad.btn_state[PAD_BTN_LB]) game.camera.fovy -= 0.01;
  if (gamepad.btn_state[PAD_BTN_RB]) game.camera.fovy += 0.01;
  game.camera.fovy = clamp(game.camera.fovy, M_PI/4, M_PI*0.9);
  
  float cam_x = apply_dead_zone(gamepad.axis[PAD_AXIS_CAM_X]);
  float cam_y = apply_dead_zone(gamepad.axis[PAD_AXIS_CAM_Y]);
  if (cam_x != 0.0 || cam_y != 0.0) {
    game.camera.theta += cam_x * CAM_SENSITIVITY_X;
    game.camera.phi -= cam_y * CAM_SENSITIVITY_Y;
    float phi_min = -asin(game.camera.center[1] / game.camera.distance);
    game.camera.phi = clamp(game.camera.phi, phi_min + 0.05, M_PI/2 - 0.2);
  }

  float zoom_in = apply_dead_zone_at(gamepad.axis[PAD_AXIS_RT], -1.0);
  float zoom_out = apply_dead_zone_at(gamepad.axis[PAD_AXIS_LT], -1.0);
  if (zoom_in != -1.0 || zoom_out != -1.0) {
    zoom_in =  1.0 - (1.0 + zoom_in) * 0.01;
    zoom_out = 1.0 + (1.0 + zoom_out) * 0.01;
    game.camera.distance *= zoom_in * zoom_out;
    game.camera.distance = clamp(game.camera.distance, 1.0, 20.0);
    float phi_min = -asin(game.camera.center[1] / game.camera.distance);
    game.camera.phi = clamp(game.camera.phi, phi_min + 0.05, M_PI/2 - 0.2);
  }
  
  float move_x = apply_dead_zone(gamepad.axis[PAD_AXIS_MOVE_X]);
  float move_y = apply_dead_zone(gamepad.axis[PAD_AXIS_MOVE_Y]);
  if (move_x != 0.0 || move_y != 0.0) {
    float front[3], left[3];
    get_camera_vectors(&game.camera, front, left, NULL, NULL);
    front[1] = 0;
    left[1] = 0;
    vec3_normalize(front);
    vec3_normalize(left);
    vec3_scale(left, -move_x);
    vec3_scale(front, move_y);

    float move[3];
    vec3_add(move, front, left);
    vec3_scale(move, MOVE_SPEED * (gamepad.btn_state[PAD_BTN_A] ? 2.0 : 1.0));
    vec3_add_to(game.creatures[0].pos, move);
    game.creatures[0].theta = atan2(move[2], move[0]) - M_PI/2;
  }
}

static void handle_loaded_assets(void)
{
  struct ASSET_REQUEST resp;
  if (recv_asset_response(&resp) != 0)
    return;
  
  switch (resp.type) {
  case ASSET_TYPE_REPLY_TEXTURE:
    {
      struct ASSET_REPLY_TEXTURE *tex = &resp.data.reply_texture;
      struct MODEL_TEXTURE model_tex;
      model_tex.width = tex->width;
      model_tex.height = tex->height;
      model_tex.n_chan = tex->n_chan;
      model_tex.data = tex->data;
      gfx_upload_model_texture(tex->gfx, &model_tex, 0);
      gfx_release_texture(tex->gfx); // release from asset loader
      free(tex->data);
    }
    break;

  default:
    console("** ERROR: got unknown asset type %d\n", resp.type);
    break;
  }
}

int process_game_step(void)
{
  handle_loaded_assets();
  handle_input();

  check_room_change();
  
  vec3_load(game.camera.center,
            game.creatures[0].pos[0],
            game.creatures[0].pos[1] + 0.8,
            game.creatures[0].pos[2]);

  return game.quit;
}
