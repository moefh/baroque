/* room.c */

#include <stdlib.h>
#include <string.h>

#include "room.h"
#include "editor.h"
#include "matrix.h"

void init_room_list(struct EDITOR_ROOM_LIST *list)
{
  list->list = NULL;
  list->next_id = 0;
}

struct EDITOR_ROOM *list_add_room(struct EDITOR_ROOM_LIST *list, const char *name)
{
  struct EDITOR_ROOM *room = malloc(sizeof(*room));
  if (! room)
    return NULL;
  room->next = list->list;
  list->list = room;

  strncpy(room->name, name, sizeof(room->name));
  room->name[sizeof(room->name)-1] = '\0';

  room->n_neighbors = 0;
  vec3_load(room->pos, 0, 0, 0);
  memset(room->tiles, 0, sizeof(room->tiles));
  
  vec4_load(room->display.color, 1, 1, 1, 1);
  room->display.tiles_changed = 1;
  room->display.gfx_id = list->next_id++;
  return room;
}

static int free_room(struct EDITOR_ROOM_LIST *list, struct EDITOR_ROOM *room)
{
  struct EDITOR_ROOM **p = &list->list;
  while (*p && *p != room)
    p = &(*p)->next;
  if (! *p)
    return 1;

  *p = room->next;
  free(room);
  return 0;
}

void free_room_list(struct EDITOR_ROOM_LIST *list)
{
  struct EDITOR_ROOM *room = list->list;
  while (room) {
    struct EDITOR_ROOM *next = room->next;
    free(room);
    room = next;
  }
  list->list = NULL;
}

void list_remove_room(struct EDITOR_ROOM_LIST *list, struct EDITOR_ROOM *room)
{
  // remove room from everyone's neighbor list
  for (struct EDITOR_ROOM *p = list->list; p != NULL; p = p->next) {
    if (p == room)
      continue;
    
    for (int i = 0; i < p->n_neighbors;) {
      if (p->neighbors[i] == room)
        memcpy(&p->neighbors[i], &p->neighbors[i+1], sizeof(p->neighbors[0]) * (p->n_neighbors-1-i));
      else
        i++;
    }
  }

  free_room(list, room);
}

void fill_room_tiles(struct EDITOR_ROOM *room, uint16_t tile, int x_first, int y_first, int x_last, int y_last)
{
  if (x_first > x_last) {
    int tmp = x_first;
    x_first = x_last;
    x_last = tmp;
  }
  if (y_first > y_last) {
    int tmp = y_first;
    y_first = y_last;
    y_last = tmp;
  }
  
  for (int y = y_first; y <= y_last; y++) {
    for (int x = x_first; x <= x_last; x++) {
      int tx = 128 + x;
      int ty = 128 + y;
      if (tx >= 0 && tx < 256 && ty >= 0 && ty < 256)
        room->tiles[ty][tx] = tile;
    }
  }
}

struct EDITOR_ROOM *get_room_by_name(struct EDITOR_ROOM_LIST *list, const char *name)
{
  for (struct EDITOR_ROOM *room = list->list; room != NULL; room = room->next) {
    if (strcmp(room->name, name) == 0)
      return room;
  }
  return NULL;
}
