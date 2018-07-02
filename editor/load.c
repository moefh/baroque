/* load.c */

#include <string.h>

#include "load.h"
#include "debug.h"
#include "editor.h"
#include "room.h"
#include "json.h"

static int read_room_prop(struct JSON_READER *reader, const char *key, void *p)
{
  //struct EDITOR_ROOM_LIST *rooms = reader->data;
  struct EDITOR_ROOM *room = p;

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
    out_text("* NOT YET IMPLEMENTED: neightbors\n");
    return skip_json_value(reader);
  }

  if (strcmp(key, "tiles") == 0) {
    out_text("* NOT YET IMPLEMENTED: tiles\n");
    return skip_json_value(reader);
  }

  out_text("** WARNING: ignoring unknown property '%s'\n", key);
  return skip_json_value(reader);
}

static int read_room(struct JSON_READER *reader, int index, void *p)
{
  struct EDITOR_ROOM_LIST *rooms = reader->data;

  struct EDITOR_ROOM *room = list_add_room(rooms, "");
  if (! room) {
    out_text("** ERROR: can't add room %d\n", index);
    return 1;
  }

  if (read_json_object(reader, read_room_prop, room) != 0)
    return 1;
  return 0;
}

static int read_rooms(struct JSON_READER *reader)
{
  return read_json_array(reader, read_room, NULL);
}

static int read_main_prop(struct JSON_READER *reader, const char *key, void *data)
{
  if (strcmp(key, "rooms") == 0)
    return read_rooms(reader);
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
  
  init_room_list(rooms);
  reader->data = rooms;
  if (read_json_object(reader, read_main_prop, rooms) != 0)
    goto err;
  
  out_text("-> OK\n");
  free_json_reader(reader);
  return 0;

 err:
  free_json_reader(reader);
  free_room_list(rooms);
  return 1;
}
