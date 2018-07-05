/* save_bff.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "room.h"
#include "load.h"
#include "model.h"

#define BFF_VERSION  '1'

struct IMAGE_INFO {
  uint32_t index;
  char name[256];
  struct ROOM_INFO *room_info;
  int tex_index;
  size_t file_offset;
};

struct ROOM_INFO {
  struct EDITOR_ROOM *room;
  size_t file_offset;
};

struct SAVE_INFO {
  FILE *f;
  size_t cur_file_offset;

  int n_rooms;
  struct ROOM_INFO *rooms;

  int n_images;
  int alloc_images;
  struct IMAGE_INFO *images;
};

static void init_save_info(struct SAVE_INFO *s)
{
  s->n_rooms = 0;
  s->rooms = NULL;
  
  s->n_images = 0;
  s->alloc_images = 0;
  s->images = NULL;
  
  s->f = NULL;
  s->cur_file_offset = 0;
}

static int free_save_info(struct SAVE_INFO *s)
{
  free(s->rooms);
  free(s->images);
  if (s->f) {
    if (fclose(s->f) != 0)
      return 1;
  }
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

static struct IMAGE_INFO *get_image(struct SAVE_INFO *s, struct MODEL_TEXTURE *tex, struct ROOM_INFO *room_info, int tex_index)
{
  char *image_name = (char *) tex->data;

  for (int i = 0; i < s->n_images; i++) {
    if (strcmp(image_name, s->images[i].name) == 0)
      return &s->images[i];
  }

  if (s->n_images + 1 >= s->alloc_images) {
    int new_alloc = s->alloc_images ? 2*s->alloc_images : 16;
    s->images = realloc(s->images, sizeof *s->images * new_alloc);
    if (! s->images)
      return NULL;
    s->alloc_images = new_alloc;
  }
  struct IMAGE_INFO *image = &s->images[s->n_images];
  image->index = s->n_images++;
  strcpy(image->name, image_name);
  image->room_info = room_info;
  image->tex_index = tex_index;
  image->file_offset = 0;
  return image;
}

static int read_file_block(const char *filename, void *data, size_t offset, size_t size)
{
  FILE *f = fopen(filename, "rb");
  if (! f)
    return 1;

  if (fseek(f, offset, SEEK_SET) != 0)
    goto err;

  size_t n;
  if ((n = fread(data, 1, size, f)) != size)
    goto err;
  
  fclose(f);
  return 0;

 err:
  fclose(f);
  return 1;
}
                
static int write_data(struct SAVE_INFO *s, void *data, size_t size)
{
  if (fwrite(data, 1, size, s->f) != size)
    return 1;
  s->cur_file_offset += size;
  return 0;
}

static int write_u8(struct SAVE_INFO *s, uint8_t n)
{
  if (fputc(n, s->f) == EOF)
    return 1;
  s->cur_file_offset++;
  return 0;
}

static int write_u16(struct SAVE_INFO *s, uint16_t n)
{
  if (fputc((n      ) & 0xff, s->f) == EOF ||
      fputc((n >>  8) & 0xff, s->f) == EOF)
    return 1;
  s->cur_file_offset += 2;
  return 0;
}

static int write_u32(struct SAVE_INFO *s, uint32_t n)
{
  if (fputc((n      ) & 0xff, s->f) == EOF ||
      fputc((n >>  8) & 0xff, s->f) == EOF ||
      fputc((n >> 16) & 0xff, s->f) == EOF ||
      fputc((n >> 24) & 0xff, s->f) == EOF)
    return 1;
  s->cur_file_offset += 4;
  return 0;
}

static int write_f32(struct SAVE_INFO *s, float n)
{
  union {
    float f;
    uint32_t u;
  } pun;

  pun.f = n;
  return write_u32(s, pun.f);
}

static int write_mat4(struct SAVE_INFO *s, float *mat)
{
  for (int i = 0; i < 16; i++)
    if (write_f32(s, mat[i]) != 0)
      return 1;
  return 0;
}

static int write_bff_header(struct SAVE_INFO *s)
{
  char header[8];
  
  memcpy(header + 0, "BFF", 3);
  header[3] = BFF_VERSION;
  memset(header + 4, 0xff, 4);
  if (write_data(s, header, 8) != 0) {
    printf("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

static int write_bff_room_neighbors(struct SAVE_INFO *s, struct EDITOR_ROOM *room)
{
  if (write_u8(s, room->n_neighbors) != 0)
    return 1;
  for (int i = 0; i < room->n_neighbors; i++) {
    if (write_u32(s, room->neighbors[i]->serialization_index) != 0)
      return 1;
  }
  return 0;
}

static int write_bff_room_tiles(struct SAVE_INFO *s, uint16_t (*tiles)[256])
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
  int x_size, y_size;
  if (x_min < 0) {
    x_min = y_min = 0;
    x_size = y_size = 0;
  } else {
    x_size = x_max - x_min;
    y_size = y_max - y_min;
  }

  if (write_u8(s, x_min) != 0 ||
      write_u8(s, x_size) != 0 ||
      write_u8(s, y_min) != 0 ||
      write_u8(s, y_size) != 0)
    return 1;
  for (int y = y_min; y <= y_max; y++) {
    for (int x = x_min; x <= x_max; x++) {
      if (write_u16(s, tiles[y][x]) != 0)
        return 1;
    }
  }
  return 0;
}

static int write_bff_model(struct SAVE_INFO *s, struct MODEL *model, struct ROOM_INFO *room_info)
{
  if (write_u16(s, model->n_meshes) != 0)
    return 1;

  for (int i = 0; i < model->n_meshes; i++) {
    struct MODEL_MESH *mesh = model->meshes[i];
    if (write_u32(s, mesh->vtx_size) != 0 ||
        write_u32(s, mesh->ind_size) != 0 ||
        write_u32(s, mesh->ind_count) != 0 ||
        write_u16(s, mesh->tex0_index) != 0 ||
        write_u16(s, mesh->tex1_index) != 0 ||
        write_u16(s, mesh->vtx_type) != 0 ||
        write_u16(s, mesh->ind_type) != 0 ||
        write_mat4(s, mesh->matrix) != 0 ||
        write_data(s, mesh->vtx, mesh->vtx_size) != 0 ||
        write_data(s, mesh->ind, mesh->ind_size) != 0)
      return 1;
  }

  if (write_u16(s, model->n_textures) != 0)
    return 1;

  for (int i = 0; i < model->n_textures; i++) {
    struct MODEL_TEXTURE *tex = &model->textures[i];
    struct IMAGE_INFO *image = get_image(s, tex, room_info, i);
    if (! image || write_u32(s, image->index) != 0)
      return 1;
  }
  
  return 0;
}

static int write_bff_room(struct SAVE_INFO *s, struct ROOM_INFO *room_info)
{
  struct EDITOR_ROOM *room = room_info->room;
  
  printf("-> writing room %d (%s)\n", room->serialization_index, room->name);

  room_info->file_offset = s->cur_file_offset;

  if (write_f32(s, room->pos[0]) != 0 ||
      write_f32(s, room->pos[1]) != 0 ||
      write_f32(s, room->pos[2]) != 0) {
    printf("** ERROR: can't write room position\n");
    return 1;
  }

  if (write_bff_room_neighbors(s, room) != 0) {
    printf("** ERROR: can't write room neighbors\n");
    return 1;
  }
  
  if (write_bff_room_tiles(s, room->tiles) != 0) {
    printf("** ERROR: can't write room tiles\n");
    return 1;
  }

  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, MODEL_FLAGS_IMAGE_REFS) != 0) {
    printf("** ERROR: can't read model from '%s'\n", filename);
    return 1;
  }
  if (write_bff_model(s, &model, room_info) != 0) {
    printf("** ERROR: can't write room model\n");
    free_model(&model);
    return 1;
  }

  free_model(&model);
  return 0;
}

static int write_bff_image(struct SAVE_INFO *s, struct IMAGE_INFO *image)
{
  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", image->room_info->room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, MODEL_FLAGS_IMAGE_REFS) != 0) {
    printf("** ERROR: can't read model images from '%s'\n", filename);
    return 1;
  }
  if (image->tex_index >= model.n_textures) {
    printf("** ERROR: invalid image %d for file '%s'\n", image->tex_index, filename);
    free_model(&model);
    return 1;
  }
  struct MODEL_TEXTURE *tex = &model.textures[image->tex_index];
  printf("-> writing image %d (%s)\n", image->index, (char*)tex->data);
  uint32_t image_offset = tex->width;
  uint32_t image_length = tex->height;
  free_model(&model);

  void *data = malloc(image_length);
  if (! data)
    goto err;

  if (read_file_block(filename, data, image_offset, image_length) != 0) {
    printf("** ERROR: can't read image from '%s'\n", filename);
    free(data);
    return 1;
  }

  image->file_offset = s->cur_file_offset;
  if (write_data(s, data, tex->height) != 0) {
    printf("** ERROR: can't write image data\n");
    free(data);
    goto err;
  }

  free(data);
  return 0;
  
 err:
  free_model(&model);
  return 1;
}

static int write_bff_index(struct SAVE_INFO *s)
{
  printf("-> writing index\n");

  size_t index_offset = s->cur_file_offset;
  
  if (write_u32(s, s->n_rooms) != 0) {
    printf("** ERROR: can't write room index\n");
    return 1;
  }
  for (int i = 0; i < s->n_rooms; i++) {
    if (write_u32(s, s->rooms[i].file_offset) != 0) {
      printf("** ERROR: can't write room index\n");
      return 1;
    }
  }

  if (write_u32(s, s->n_images) != 0) {
    printf("** ERROR: can't write image index\n");
    return 1;
  }
  for (int i = 0; i < s->n_rooms; i++) {
    if (write_u32(s, s->images[i].file_offset) != 0) {
      printf("** ERROR: can't write image index\n");
      return 1;
    }
  }

  if (fseek(s->f, 4, SEEK_SET) != 0) {
    printf("** ERROR: can't seek to header\n");
    return 1;
  }
  if (write_u32(s, index_offset) != 0) {
    printf("** ERROR: can't write index offset to header\n");
    return 1;
  }
  
  return 0;
}

int write_bff_file(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  struct SAVE_INFO info;
  init_save_info(&info);

  info.f = fopen(filename, "wb");
  if (! info.f) {
    printf("** ERROR: can't open '%s'\n", filename);
    goto err;
  }

  info.rooms = create_room_info(rooms, &info.n_rooms);
  if (! info.rooms) {
    printf("** ERROR: out of memory for rooms info\n");
    goto err;
  }
  
  if (write_bff_header(&info) != 0)
    goto err;
  
  for (int i = 0; i < info.n_rooms; i++) {
    if (write_bff_room(&info, &info.rooms[i]) != 0)
      goto err;
  }

  for (int i = 0; i < info.n_images; i++) {
    if (write_bff_image(&info, &info.images[i]) != 0)
      goto err;
  }

  if (write_bff_index(&info) != 0)
    goto err;
  
  if (free_save_info(&info) != 0) {
    printf("** ERROR writing file data\n");
    return 1;
  }

  printf("-> done.\n");
  return 0;

 err:
  free_save_info(&info);
  return 1;
}
