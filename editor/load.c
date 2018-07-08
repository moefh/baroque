/* load.c */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "load.h"
#include "debug.h"
#include "editor.h"
#include "room.h"
#include "json.h"
#include "base64.h"

struct ROOM_INFO {
  struct EDITOR_ROOM *room;
  int n_neighbors;
  uint16_t neighbor_ids[EDITOR_ROOM_MAX_NEIGHBORS];
  uint16_t tiles_pos[2];
  int tiles_size_x;
};

struct LOAD_INFO {
  int n_rooms;
  int n_alloc;
  struct ROOM_INFO *rooms_info;
  struct EDITOR_ROOM_LIST *rooms;
};

static void init_load_info(struct LOAD_INFO *info, struct EDITOR_ROOM_LIST *rooms)
{
  info->n_rooms = 0;
  info->n_alloc = 0;
  info->rooms_info = NULL;
  info->rooms = rooms;
}

static void free_load_info(struct LOAD_INFO *info)
{
  free(info->rooms_info);
}

static void init_room_info(struct ROOM_INFO *room_info, struct EDITOR_ROOM *room)
{
  room_info->room = room;
  room_info->n_neighbors = 0;
  room_info->tiles_pos[0] = 0;
  room_info->tiles_pos[1] = 0;
  room_info->tiles_size_x = 0;
}

static uint16_t get_u16(unsigned char *p)
{
  return p[0] | p[1] << 8;
}

static int read_tiles_data(struct JSON_READER *reader, int index, void *p)
{
  struct ROOM_INFO *room_info = p;
  struct EDITOR_ROOM *room = room_info->room;

  int tiles_pos_x = room_info->tiles_pos[0];
  int tiles_pos_y = room_info->tiles_pos[1] + index;
  if (tiles_pos_y >= 256) {
    out_text("** ERROR: tile height too large\n");
    return 1;
  }
  
  // base64 encoded max string size: 4/3 * original data size (256 u16), rounded up, plus '\0'
  char tiles_data_b64[(256 * sizeof(uint16_t) * 4 + 1) / 3 + 1];
  if (read_json_string(reader, tiles_data_b64, sizeof(tiles_data_b64)) != 0)
    return 1;

  unsigned char tiles_data[2*256];
  size_t tiles_data_size;
  if (decode_base64(tiles_data, sizeof(tiles_data), &tiles_data_size, tiles_data_b64) != 0) {
    out_text("** ERROR: bad base64 tiles\n");
    return 1;
  }
  if (tiles_data_size % sizeof(uint16_t)) {
    out_text("** ERROR: invalid tile data\n");
    return 1;
  }
  int tiles_size_x = (int) (tiles_data_size / sizeof(uint16_t));
  if (tiles_pos_x + tiles_size_x >= 256) {
    out_text("** ERROR: too many tiles\n");
    return 1;
  }
  
  // first line determines the length of all subsequent lines
  if (index == 0) {
    room_info->tiles_size_x = tiles_size_x;
  } else if (room_info->tiles_size_x != tiles_size_x) {
    out_text("** ERROR: inconsistent tiles size\n");
    return 1;
  }
  for (int i = 0; i < tiles_size_x; i++)
    room->tiles[tiles_pos_y][tiles_pos_x + i] = get_u16(&tiles_data[2*i]);
  return 0;
}

static int read_tiles_prop(struct JSON_READER *reader, const char *key, void *p)
{
  struct ROOM_INFO *room_info = p;

  if (strcmp(key, "pos") == 0) {
    size_t n_vals;
    if (read_json_u16_array(reader, room_info->tiles_pos, 2, &n_vals) != 0)
      return 1;
    if (n_vals != 2)
      return 1;
    return 0;
  }

  if (strcmp(key, "data") == 0)
    return read_json_array(reader, read_tiles_data, room_info);

  out_text("** WARNING: ignoring unknown tiles property '%s'\n", key);
  return skip_json_value(reader);
}

static int read_room_prop(struct JSON_READER *reader, const char *key, void *p)
{
  struct ROOM_INFO *room_info = p;
  struct EDITOR_ROOM *room = room_info->room;

  if (strcmp(key, "name") == 0)
    return read_json_string(reader, room->name, sizeof(room->name));

  if (strcmp(key, "pos") == 0) {
    size_t n_vals;
    if (read_json_float_array(reader, room->pos, 3, &n_vals) != 0)
      return 1;
    if (n_vals != 3)
      return 1;
    return 0;
  }

  if (strcmp(key, "neighbors") == 0) {
    size_t n_vals;
    if (read_json_u16_array(reader, room_info->neighbor_ids, EDITOR_ROOM_MAX_NEIGHBORS, &n_vals) != 0)
      return 1;
    room_info->n_neighbors = (int) n_vals;
    return 0;
  }

  if (strcmp(key, "tiles") == 0)
    return read_json_object(reader, read_tiles_prop, room_info);

  out_text("** WARNING: ignoring unknown room property '%s'\n", key);
  return skip_json_value(reader);
}

static struct EDITOR_ROOM *add_room(struct LOAD_INFO *info)
{
  struct EDITOR_ROOM *room = list_add_room(info->rooms, "");
  if (! room)
    return NULL;
  room->serialization_index = info->n_rooms;

  if (info->n_rooms >= info->n_alloc) {
    int new_n_alloc = info->n_alloc ? 2*info->n_alloc : 16;
    info->rooms_info = realloc(info->rooms_info, sizeof *info->rooms_info * new_n_alloc);
    if (! info->rooms_info)
      return NULL;
    info->n_alloc = new_n_alloc;
  }
  init_room_info(&info->rooms_info[info->n_rooms], room);
  info->n_rooms++;
  
  return room;
}

static int read_room(struct JSON_READER *reader, int index, void *p)
{
  struct LOAD_INFO *info = reader->data;

  struct EDITOR_ROOM *room = add_room(info);
  if (! room || room->serialization_index != index) {
    out_text("** ERROR: can't add room %d\n", index);
    return 1;
  }

  if (read_json_object(reader, read_room_prop, &info->rooms_info[index]) != 0)
    return 1;
  return 0;
}

static int read_main_prop(struct JSON_READER *reader, const char *key, void *data)
{
  if (strcmp(key, "version") == 0) {
    char version[32];
    if (read_json_string(reader, version, sizeof(version)) != 0)
      return 1;
    return 0;
  }
  
  if (strcmp(key, "rooms") == 0)
    return read_json_array(reader, read_room, NULL);

  out_text("** WARNING: ignoring unknown property '%s'\n", key);
  return skip_json_value(reader);
}

static int fix_room_neighbors(struct LOAD_INFO *info)
{
  for (int room_index = 0; room_index < info->n_rooms; room_index++) {
    struct ROOM_INFO *room_info = &info->rooms_info[room_index];
    struct EDITOR_ROOM *room = room_info->room;

    room->n_neighbors = room_info->n_neighbors;
    for (int i = 0; i < room_info->n_neighbors; i++) {
      if (room_info->neighbor_ids[i] < 0 || room_info->neighbor_ids[i] >= info->n_rooms) {
        out_text("** ERROR: room %d has invalid neighbor %d\n", room_index, room_info->neighbor_ids[i]);
        return 1;
      }
      room->neighbors[i] = info->rooms_info[room_info->neighbor_ids[i]].room;
    }
  }
  return 0;
}

int load_map_rooms(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  out_text("-> loading '%s'...\n", filename);

  struct JSON_READER *reader = read_json_file(filename);
  if (! reader) {
    out_text("** ERROR: can't read '%s'\n", filename);
    return 1;
  }

  struct LOAD_INFO info;
  init_room_list(rooms);
  init_load_info(&info, rooms);
  reader->data = &info;
  if (read_json_object(reader, read_main_prop, NULL) != 0)
    goto err;

  if (fix_room_neighbors(&info) != 0)
    goto err;
  
  free_json_reader(reader);
  free_load_info(&info);
  out_text("-> load OK\n");
  return 0;

 err:
  out_text("** ERROR: load failed\n");
  free_json_reader(reader);
  free_load_info(&info);
  free_room_list(rooms);
  return 1;
}
