/* room.h */

#ifndef ROOM_H_FILE
#define ROOM_H_FILE

#include <stdint.h>

#define EDITOR_ROOM_NAME_LEN      32
#define EDITOR_ROOM_MAX_NEIGHBORS 16

struct EDITOR_ROOM_DISPLAY_INFO {
  int gfx_id;
  float color[4];
  int tiles_changed;
};

struct EDITOR_ROOM {
  struct EDITOR_ROOM *next;
  char name[EDITOR_ROOM_NAME_LEN];
  float pos[3];
  
  uint16_t tiles[256][256];
  
  int n_neighbors;
  struct EDITOR_ROOM *neighbors[EDITOR_ROOM_MAX_NEIGHBORS];

  struct EDITOR_ROOM_DISPLAY_INFO display;
  int serialization_index; // used during load/save
};

struct EDITOR_ROOM_LIST {
  int next_id;
  struct EDITOR_ROOM *list;
};

void init_room_list(struct EDITOR_ROOM_LIST *list);
void free_room_list(struct EDITOR_ROOM_LIST *list);

struct EDITOR_ROOM *list_add_room(struct EDITOR_ROOM_LIST *list, const char *name);
struct EDITOR_ROOM *get_room_by_name(struct EDITOR_ROOM_LIST *list, const char *name);
void list_remove_room(struct EDITOR_ROOM_LIST *list, struct EDITOR_ROOM *room);

void fill_room_tiles(struct EDITOR_ROOM *room, uint16_t tile, int x_first, int y_first, int x_last, int y_last);

#endif /* ROOM_H_FILE */
