/* room.c */

#include <stddef.h>

#include "room.h"
#include "debug.h"

#define MAX_LOADED_ROOMS   64

struct ROOM_STORE {
  struct ROOM *alloc_list;
  struct ROOM *free_list;
  struct ROOM rooms[MAX_LOADED_ROOMS];
};

static struct ROOM_STORE room_store;

void init_room_store(void)
{
  room_store.alloc_list = NULL;
  room_store.free_list = &room_store.rooms[0];
  
  for (int i = 0; i < MAX_LOADED_ROOMS-1; i++)
    room_store.rooms[i].next = &room_store.rooms[i + 1];
  room_store.rooms[MAX_LOADED_ROOMS-1].next = NULL;
}

struct ROOM *alloc_room(void)
{
  struct ROOM *room = room_store.free_list;
  if (room == NULL)
    return NULL;
  room_store.free_list = room->next;
  room->next = room_store.alloc_list;
  room_store.alloc_list = room;
  return room;
}

void free_room(struct ROOM *room)
{
  // remove from alloc list
  struct ROOM **p = &room_store.alloc_list;
  while (*p && *p != room)
    p = &(*p)->next;
  if (! *p)
    return;
  *p = room->next;

  // add to free list
  room->next = room_store.free_list;
  room_store.free_list = room;
}

struct ROOM *get_room_list(void)
{
  return room_store.alloc_list;
}

struct ROOM *get_room_by_index(int index)
{
  for (struct ROOM *room = room_store.alloc_list; room != NULL; room = room->next) {
    if (room->index == index)
      return room;
  }
  return NULL;
}

bool has_free_room(void)
{
  return room_store.free_list != NULL;
}

void mark_all_rooms(int mark)
{
  for (struct ROOM *room = room_store.alloc_list; room != NULL; room = room->next)
    room->mark = mark;
}

struct ROOM *get_marked_room(int mark)
{
  for (struct ROOM *room = room_store.alloc_list; room != NULL; room = room->next) {
    if (room->mark == mark)
      return room;
  }
  return NULL;
}
