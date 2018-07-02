/* load.c */

#include "load.h"
#include "editor.h"
#include "room.h"

int load_map_rooms(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  out_text("-> loading '%s'...\n", filename);

  out_text("** load not implemented, here's a complimentary room just for you\n");
  
  init_room_list(rooms);
  list_add_room(rooms, "world");
  
  out_text("-> OK\n");
  return 0;
}
