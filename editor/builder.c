/* build.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "room.h"
#include "load.h"
#include "save_bff.h"

static int convert_world(const char *in_filename, const char *out_filename)
{
  printf("-> reading '%s'...\n", in_filename);
  struct EDITOR_ROOM_LIST rooms;
  if (load_map_rooms(in_filename, &rooms) != 0) {
    printf("** ERROR\n");
    return 1;
  }

  printf("-> writing '%s'...\n", out_filename);
  int ret = write_bwf_file(out_filename, &rooms);

  printf("-> done\n");
  free_room_list(&rooms);
  return ret;
}

static int convert_model(const char *in_filename, const char *out_filename)
{
  printf("-> converting '%s' to '%s'...\n", in_filename, out_filename);
  if (write_bmf_file(out_filename, in_filename) != 0) {
    printf("** ERROR\n");
    return 1;
  }
  
  printf("-> done\n");
  return 0;
}

static int convert_character(const char *in_filename, const char *out_filename)
{
  printf("-> converting '%s' to '%s'...\n", in_filename, out_filename);
  if (write_bcf_file(out_filename, in_filename) != 0) {
    printf("** ERROR\n");
    return 1;
  }
  
  printf("-> done\n");
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 4) {
    printf("USAGE: %s command input_file output_file\n", argv[0]);
    printf("\n");
    printf("commands:\n");
    printf("   world      convert json to bwf\n");
    printf("   model      convert glb to bmf\n");
    printf("   char       convert glb to bcf\n");
    exit(1);
  }
  const char *command = argv[1];
  const char *in_filename = argv[2];
  const char *out_filename = argv[3];

  if (strcmp(command, "world") == 0)
    return convert_world(in_filename, out_filename);

  if (strcmp(command, "model") == 0)
    return convert_model(in_filename, out_filename);

  if (strcmp(command, "char") == 0)
    return convert_character(in_filename, out_filename);
  
  printf("** ERROR: unknown command: '%s'\n", command);
  return 1;
}
