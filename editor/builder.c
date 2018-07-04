/* build.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "room.h"
#include "load.h"
#include "save_bff.h"

int main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("USAGE: %s file.map file.bff\n", argv[0]);
    exit(1);
  }
  const char *in_filename = argv[1];
  const char *out_filename = argv[2];
  
  struct EDITOR_ROOM_LIST rooms;
  if (load_map_rooms(in_filename, &rooms) != 0)
    return 1;

  if (write_bff_file(out_filename, &rooms) != 0)
    goto err;
  free_room_list(&rooms);
  return 0;

 err:
  free_room_list(&rooms);
  return 1;
}
