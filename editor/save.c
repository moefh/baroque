/* save.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

#include "save.h"
#include "room.h"
#include "text.h"
#include "base64.h"

struct ROOM_INFO {
  struct EDITOR_ROOM *room;
};

struct SAVE_INFO {
  FILE *f;
  int n_rooms;
  struct ROOM_INFO *rooms;
};

static char to_hex_digit(unsigned int c)
{
  if (c < 10)
    return c + '0';
  if (c < 16)
    return c - 10 + 'f';
  return '0';
}

static void write_vec3(FILE *f, float *v, const char *suffix)
{
  fprintf(f, "[ %f, %f, %f ]", v[0], v[1], v[2]);
  if (suffix)
    fwrite(suffix, 1, strlen(suffix), f);
}

static void write_string(FILE *f, const char *str, const char *suffix)
{
  fputc('"', f);

  const unsigned char *p = (unsigned char *) str;
  while (*p) {
    switch (*p) {
    case '"':  fputc('\\', f); fputc('"',  f); break;
    case '\n': fputc('\\', f); fputc('n',  f); break;
    case '\r': fputc('\\', f); fputc('r',  f); break;
    case '\t': fputc('\\', f); fputc('t',  f); break;
    case '\b': fputc('\\', f); fputc('b',  f); break;
    case '\f': fputc('\\', f); fputc('f',  f); break;
    case '\\': fputc('\\', f); fputc('\\', f); break;
    default:
      if (*p >= 32 && *p < 127) {
        fputc(*p, f);
        break;
      }
      fputc('\\', f);
      fputc('u', f);
      fputc('0', f);
      fputc('0', f);
      fputc(to_hex_digit((*p >> 4 ) & 0xf), f);
      fputc(to_hex_digit((*p      ) & 0xf), f);
      break;
    }
    p++;
  }
  fputc('"', f);
  if (suffix)
    fwrite(suffix, 1, strlen(suffix), f);
}

static void write_room_neighbors(struct SAVE_INFO *s, int n_neighbors, struct EDITOR_ROOM **neighbors, const char *suffix)
{
  fprintf(s->f, "[");
  for (int i = 0; i < n_neighbors; i++) {
    //struct ROOM_INFO *info = &s->rooms[neighbors[i]->serialization_index];
    fprintf(s->f, " %d%c", neighbors[i]->serialization_index, (i+1 == n_neighbors) ? ' ' : ',');
  }
  fprintf(s->f, "]");

  if (suffix)
    fwrite(suffix, 1, strlen(suffix), s->f);
}

static void put_u16(unsigned char *p, uint16_t n)
{
  p[0] = n & 0xff;
  p[1] = (n >> 8) & 0xff;
}

static int write_room_tiles(struct SAVE_INFO *s, uint16_t (*tiles)[256], const char *suffix)
{
  int x_min, y_min, x_max, y_max;
  x_min = y_min = x_max = y_max = -1;
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      if (tiles[y][x]) {
        if (x_min < 0 || x_min > x) x_min = x;
        if (x_max < 0 || x_max < x) x_max = x;
        if (y_min < 0 || y_min > y) y_min = y;
        if (y_max < 0 || y_max < y) y_max = y;
      }
    }
  }
  if (x_min < 0)
    x_min = y_min = 0;
  
  fprintf(s->f, "{\n");
  fprintf(s->f, "        \"pos\" : [ %d, %d ],\n", x_min, y_min);
  if (x_max < x_min || y_max < y_min) {
    fprintf(s->f, "        \"data\" : []\n");
  } else {
    fprintf(s->f, "        \"data\" : [\n");
    for (int y = y_min; y <= y_max; y++) {
      unsigned char tiles_data[2*256];
      for (int x = 0; x < x_max-x_min+1; x++)
        put_u16(&tiles_data[2*x], tiles[y][x_min + x]);
      
      // base64 encoded max string size: 4/3 * (256 u16), rounded up, plus '\0'
      char tiles_data_b64[(256 * sizeof(uint16_t) * 4 + 1) / 3 + 1];
      if (encode_base64(tiles_data_b64, sizeof(tiles_data_b64), tiles_data, (x_max-x_min+1)*sizeof(uint16_t)) != 0) {
        out_text("** ERROR encoding tiles in base64\n");
        return 1;
      }
      fprintf(s->f, "          \"%s\"%s\n", tiles_data_b64, (y == y_max) ? "" : ",");
    }
    fprintf(s->f, "        ]\n");
  }
  fprintf(s->f, "      }");
  if (suffix)
    fwrite(suffix, 1, strlen(suffix), s->f);
  return 0;
}

static int write_room(struct SAVE_INFO *s, struct ROOM_INFO *info)
{
  struct EDITOR_ROOM *room = info->room;
    
  fprintf(s->f, "    {\n");

  fprintf(s->f, "      \"name\" : ");
  write_string(s->f, room->name, ",\n");
  
  fprintf(s->f, "      \"pos\" : ");
  write_vec3(s->f, room->pos, ",\n");

  fprintf(s->f, "      \"neighbors\" : ");
  write_room_neighbors(s, room->n_neighbors, room->neighbors, ",\n");
  
  fprintf(s->f, "      \"tiles\" : ");
  if (write_room_tiles(s, room->tiles, "\n") != 0)
    return 1;
  
  fprintf(s->f, "    }");
  return 0;
}

static int write_main_object(struct SAVE_INFO *s)
{
  fprintf(s->f, "{\n");
  fprintf(s->f, "  \"version\" : \"0.1\",\n");
  fprintf(s->f, "  \"rooms\" : [\n");
  for (int i = 0; i < s->n_rooms; i++) {
    if (write_room(s, &s->rooms[i]) != 0)
      return 1;
    if (i+1 < s->n_rooms)
      fprintf(s->f, ",");
    fprintf(s->f, "\n");
  }
  fprintf(s->f, "  ]\n");
  fprintf(s->f, "}\n");
  return 0;
}

static struct ROOM_INFO *create_room_info(struct EDITOR_ROOM_LIST *rooms, int *p_n_rooms)
{
  int n_rooms = 0;
  for (struct EDITOR_ROOM *room = rooms->list; room != NULL; room = room->next)
    n_rooms++;
  
  struct ROOM_INFO *room_info = malloc(n_rooms * sizeof *room_info);
  if (! room_info)
    return NULL;

  int room_index = n_rooms;
  for (struct EDITOR_ROOM *room = rooms->list; room != NULL; room = room->next) {
    struct ROOM_INFO *info = &room_info[--room_index];
    info->room = room;
    room->serialization_index = room_index;
  }

  *p_n_rooms = n_rooms;
  return room_info;
}

static void init_save_info(struct SAVE_INFO *s)
{
  s->n_rooms = 0;
  s->rooms = NULL;
  s->f = NULL;
}

static void free_save_info(struct SAVE_INFO *s)
{
  free(s->rooms);
  if (s->f)
    fclose(s->f);
}

int save_map_rooms(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  struct SAVE_INFO save_info;
  init_save_info(&save_info);
  
  out_text("-> saving '%s'...\n", filename);

  char tmp_filename[256];
  int tmp_filename_size = snprintf(tmp_filename, sizeof(tmp_filename), "%s.tmp", filename);
  if (tmp_filename_size < 0 || (size_t)tmp_filename_size+1 > sizeof(tmp_filename)) {
    out_text("** ERROR: file name too long\n");
    return 1;
  }
  save_info.f = fopen(tmp_filename, "w");
  if (! save_info.f) {
    out_text("** ERROR: can't open '%s'\n", tmp_filename);
    return 1;
  }

  save_info.rooms = create_room_info(rooms, &save_info.n_rooms);
  if (! save_info.rooms)
    goto err;
  
  if (write_main_object(&save_info) != 0) {
    out_text("** ERROR saving map\n");
    goto err;
  }
  if (ferror(save_info.f)) {
    out_text("** ERROR writing output file\n");
    goto err;
  }

  free_save_info(&save_info);
  unlink(filename);
  if (rename(tmp_filename, filename) != 0) {
    out_text("** ERROR renaming '%s' to '%s'\n", tmp_filename, filename);
    return 1;
  }
  out_text("-> OK\n");
  return 0;

 err:
  free_save_info(&save_info);
  return 1;
}
