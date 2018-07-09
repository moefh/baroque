/* room.h */

#ifndef ROOM_H_FILE
#define ROOM_H_FILE

#include <stdint.h>
#include <stdbool.h>

#define ROOM_MAX_NEIGHBORS 16

struct ROOM {
  struct ROOM *next;

  int index;
  int mark;
  float pos[3];
  
  int n_neighbors;
  uint32_t neighbor_index[ROOM_MAX_NEIGHBORS]; 
  struct ROOM *neighbor[ROOM_MAX_NEIGHBORS];
 
  uint16_t tiles[256][256];
};

void init_room_store(void);
struct ROOM *alloc_room(void);
void free_room(struct ROOM *room);
struct ROOM *get_room_by_index(int index);

bool has_free_room(void);
void mark_all_rooms(int mark);
struct ROOM *get_marked_room(int mark);

#endif /* ROOM_H_FILE */
