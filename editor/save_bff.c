/* save_bff.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "room.h"
#include "load.h"
#include "model.h"

struct SAVE_INFO;

typedef int (translate_tex_index_func)(struct SAVE_INFO *s, struct MODEL *model, int model_tex_index, uint32_t *tex_index, void *p);

struct SAVE_INFO {
  FILE *f;
  size_t cur_file_offset;
  translate_tex_index_func *translate_tex_index;
};

static void init_save_info(struct SAVE_INFO *s, translate_tex_index_func *translate_tex_index)
{
  s->f = NULL;
  s->cur_file_offset = 0;
  s->translate_tex_index = translate_tex_index;
}

static void close_save_info(struct SAVE_INFO *s)
{
  if (s->f)
    fclose(s->f);
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

static int set_file_pos(struct SAVE_INFO *s, size_t pos)
{
  if (fseek(s->f, pos, SEEK_SET) != 0)
    return 1;
  s->cur_file_offset = pos;
  return 0;
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
  return write_u32(s, pun.u);
}

static int write_mat4(struct SAVE_INFO *s, float *mat)
{
  for (int i = 0; i < 16; i++)
    if (write_f32(s, mat[i]) != 0)
      return 1;
  return 0;
}

static int write_model_meshes(struct SAVE_INFO *s, struct MODEL *model, void *data)
{
  if (write_u16(s, model->n_meshes) != 0)
    return 1;

  for (int i = 0; i < model->n_meshes; i++) {
    struct MODEL_MESH *mesh = model->meshes[i];

    uint32_t tex0_index, tex1_index;
    if (s->translate_tex_index(s, model, mesh->tex0_index, &tex0_index, data) != 0 ||
        s->translate_tex_index(s, model, mesh->tex1_index, &tex1_index, data) != 0)
      return 1;

    if (write_u32(s, mesh->vtx_size) != 0 ||
        write_u32(s, mesh->ind_size) != 0 ||
        write_u32(s, mesh->ind_count) != 0 ||
        write_u16(s, mesh->vtx_type) != 0 ||
        write_u16(s, mesh->ind_type) != 0 ||
        write_u32(s, tex0_index) != 0 ||
        write_u32(s, tex1_index) != 0 ||
        write_mat4(s, mesh->matrix) != 0 ||
        write_data(s, mesh->vtx, mesh->vtx_size) != 0 ||
        write_data(s, mesh->ind, mesh->ind_size) != 0)
      return 1;
  }

  return 0;
}

/* ==========================================================================================
 * Write BMW
 */

#define BMF_VERSION '1'

static int get_bmf_tex_index(struct SAVE_INFO *s, struct MODEL *model, int model_tex_index, uint32_t *bwf_tex_index, void *data)
{
  *bwf_tex_index = model_tex_index;
  return 0;
}

static int write_bmf_header(struct SAVE_INFO *s)
{
  char header[4];
  
  memcpy(header + 0, "BMF", 3);
  header[3] = BMF_VERSION;
  if (write_data(s, header, 4) != 0) {
    printf("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

int write_bmf_model_textures(struct SAVE_INFO *s, struct MODEL *model)
{
  if (write_u16(s, model->n_textures) != 0)
    return 1;
  
  for (int i = 0; i < model->n_textures; i++) {
    printf("-> writing texture %d\n", i);
    
    struct MODEL_TEXTURE *tex = &model->textures[i];
    uint32_t image_length = tex->height;
    if (write_data(s, tex->data, image_length) != 0) {
      printf("** ERROR: can't write texture data\n");
      return 1;
    }
  }
  return 0;
}

int write_bmf_file(const char *bmf_filename, const char *glb_filename)
{
  struct MODEL model;
  if (read_glb_model(&model, glb_filename, MODEL_FLAGS_PACKED_IMAGES) != 0) {
    printf("** ERROR: can't read model from '%s'\n", glb_filename);
    return 1;
  }

  struct SAVE_INFO info;
  init_save_info(&info, get_bmf_tex_index);
  info.f = fopen(bmf_filename, "wb");
  if (! info.f)
    goto err;

  if (write_bmf_header(&info) != 0)
    goto err;
  
  printf("-> writing model meshes\n");
  if (write_model_meshes(&info, &model, NULL) != 0)
    goto err;

  if (write_bmf_model_textures(&info, &model) != 0)
    goto err;
  
  free_model(&model);
  close_save_info(&info);
  return 0;

 err:
  free_model(&model);
  close_save_info(&info);
  return 1;
}

/* ==========================================================================================
 * Write BFW
 */

#define BWF_VERSION '1'

struct IMAGE_INFO {
  uint32_t index;
  char name[256];
  struct ROOM_INFO *room_info;
  int model_tex_index;
  size_t file_offset;
};

struct ROOM_INFO {
  struct EDITOR_ROOM *room;
  size_t file_offset;
};

struct BWF_SAVE_INFO {
  struct SAVE_INFO info;

  int n_rooms;
  struct ROOM_INFO *rooms;

  int n_images;
  int alloc_images;
  struct IMAGE_INFO *images;
};

static void init_bwf_save_info(struct BWF_SAVE_INFO *bwf, translate_tex_index_func *func)
{
  init_save_info(&bwf->info, func);
  
  bwf->n_rooms = 0;
  bwf->rooms = NULL;
  
  bwf->n_images = 0;
  bwf->alloc_images = 0;
  bwf->images = NULL;
}

static int free_bwf_save_info(struct BWF_SAVE_INFO *bwf)
{
  free(bwf->rooms);
  free(bwf->images);
  close_save_info(&bwf->info);
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

static struct IMAGE_INFO *get_image(struct BWF_SAVE_INFO *bwf, struct MODEL_TEXTURE *tex, struct ROOM_INFO *room_info, int tex_index)
{
  char *image_name = (char *) tex->data;

  for (int i = 0; i < bwf->n_images; i++) {
    if (strcmp(image_name, bwf->images[i].name) == 0)
      return &bwf->images[i];
  }

  if (bwf->n_images + 1 >= bwf->alloc_images) {
    int new_alloc = bwf->alloc_images ? 2*bwf->alloc_images : 16;
    bwf->images = realloc(bwf->images, sizeof *bwf->images * new_alloc);
    if (! bwf->images)
      return NULL;
    bwf->alloc_images = new_alloc;
  }
  struct IMAGE_INFO *image = &bwf->images[bwf->n_images];
  image->index = bwf->n_images++;
  strcpy(image->name, image_name);
  image->room_info = room_info;
  image->model_tex_index = tex_index;
  image->file_offset = 0;
  return image;
}

static int write_bwf_header(struct BWF_SAVE_INFO *bwf)
{
  char header[8];
  
  memcpy(header + 0, "BWF", 3);
  header[3] = BWF_VERSION;
  memset(header + 4, 0xff, 4);
  if (write_data(&bwf->info, header, 8) != 0) {
    printf("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

static int get_bwf_tex_index(struct SAVE_INFO *s, struct MODEL *model, int model_tex_index, uint32_t *bwf_tex_index, void *data)
{
  struct BWF_SAVE_INFO *bwf = (struct BWF_SAVE_INFO *) s;
  struct ROOM_INFO *room_info = data;
  
  if (model_tex_index == MODEL_TEXTURE_NONE) {
    *bwf_tex_index = 0xffffffff;
    return 0;
  }
  
  struct IMAGE_INFO *image = get_image(bwf, &model->textures[model_tex_index], room_info, model_tex_index);
  if (! image)
    return 1;
  *bwf_tex_index = image->index;
  return 0;
}

static int write_bwf_room_neighbors(struct BWF_SAVE_INFO *bwf, struct EDITOR_ROOM *room)
{
  if (write_u8(&bwf->info, room->n_neighbors) != 0)
    return 1;
  for (int i = 0; i < room->n_neighbors; i++) {
    if (write_u32(&bwf->info, room->neighbors[i]->serialization_index) != 0)
      return 1;
  }
  return 0;
}

static int write_bwf_room_tiles(struct BWF_SAVE_INFO *bwf, uint16_t (*tiles)[256])
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
    x_size = x_max - x_min + 1;
    y_size = y_max - y_min + 1;
  }

  if (write_u8(&bwf->info, x_min) != 0 ||
      write_u8(&bwf->info, x_size) != 0 ||
      write_u8(&bwf->info, y_min) != 0 ||
      write_u8(&bwf->info, y_size) != 0)
    return 1;
  for (int y = 0; y < y_size; y++) {
    for (int x = 0; x < x_size; x++) {
      if (write_u16(&bwf->info, tiles[y_min + y][x_min + x]) != 0)
        return 1;
    }
  }
  return 0;
}

static int write_bwf_room(struct BWF_SAVE_INFO *bwf, struct ROOM_INFO *room_info)
{
  struct EDITOR_ROOM *room = room_info->room;
  
  printf("-> writing room %d (%s)\n", room->serialization_index, room->name);

  room_info->file_offset = bwf->info.cur_file_offset;

  if (write_f32(&bwf->info, room->pos[0]) != 0 ||
      write_f32(&bwf->info, room->pos[1]) != 0 ||
      write_f32(&bwf->info, room->pos[2]) != 0) {
    printf("** ERROR: can't write room position\n");
    return 1;
  }

  if (write_bwf_room_neighbors(bwf, room) != 0) {
    printf("** ERROR: can't write room neighbors\n");
    return 1;
  }
  
  if (write_bwf_room_tiles(bwf, room->tiles) != 0) {
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
  if (write_model_meshes(&bwf->info, &model, room_info) != 0) {
    printf("** ERROR: can't write room model\n");
    free_model(&model);
    return 1;
  }

  free_model(&model);
  return 0;
}

static int write_bwf_image(struct BWF_SAVE_INFO *bwf, struct IMAGE_INFO *image)
{
  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", image->room_info->room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, MODEL_FLAGS_IMAGE_REFS) != 0) {
    printf("** ERROR: can't read model images from '%s'\n", filename);
    return 1;
  }
  if (image->model_tex_index >= model.n_textures) {
    printf("** ERROR: invalid image %d for file '%s'\n", image->model_tex_index, filename);
    free_model(&model);
    return 1;
  }
  struct MODEL_TEXTURE *tex = &model.textures[image->model_tex_index];
  printf("-> writing image %d (%s)\n", image->index, (char*)tex->data);
  uint32_t image_offset = tex->width;
  uint32_t image_length = tex->height;
  free_model(&model);

  void *data = malloc(image_length);
  if (! data)
    return 1;

  if (read_file_block(filename, data, image_offset, image_length) != 0) {
    printf("** ERROR: can't read image from '%s'\n", filename);
    free(data);
    return 1;
  }

  image->file_offset = bwf->info.cur_file_offset;
  if (write_data(&bwf->info, data, image_length) != 0) {
    printf("** ERROR: can't write image data\n");
    free(data);
    return 1;
  }

  free(data);
  return 0;
}

static int write_bwf_index(struct BWF_SAVE_INFO *bwf)
{
  printf("-> writing index\n");

  size_t index_offset = bwf->info.cur_file_offset;
  
  if (write_u32(&bwf->info, bwf->n_rooms) != 0) {
    printf("** ERROR: can't write room index\n");
    return 1;
  }
  for (int i = 0; i < bwf->n_rooms; i++) {
    if (write_u32(&bwf->info, bwf->rooms[i].file_offset) != 0) {
      printf("** ERROR: can't write room index\n");
      return 1;
    }
  }

  if (write_u32(&bwf->info, bwf->n_images) != 0) {
    printf("** ERROR: can't write image index\n");
    return 1;
  }
  for (int i = 0; i < bwf->n_images; i++) {
    if (write_u32(&bwf->info, bwf->images[i].file_offset) != 0) {
      printf("** ERROR: can't write image index\n");
      return 1;
    }
  }

  if (set_file_pos(&bwf->info, 4) != 0) {
    printf("** ERROR: can't seek to header\n");
    return 1;
  }
  if (write_u32(&bwf->info, index_offset) != 0) {
    printf("** ERROR: can't write index offset to header\n");
    return 1;
  }
  
  return 0;
}

int write_bwf_file(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  struct BWF_SAVE_INFO info;
  init_bwf_save_info(&info, get_bwf_tex_index);

  info.info.f = fopen(filename, "wb");
  if (! info.info.f) {
    printf("** ERROR: can't open '%s'\n", filename);
    goto err;
  }

  info.rooms = create_room_info(rooms, &info.n_rooms);
  if (! info.rooms) {
    printf("** ERROR: out of memory for rooms info\n");
    goto err;
  }
  
  if (write_bwf_header(&info) != 0)
    goto err;
  
  for (int i = 0; i < info.n_rooms; i++) {
    if (write_bwf_room(&info, &info.rooms[i]) != 0)
      goto err;
  }

  for (int i = 0; i < info.n_images; i++) {
    if (write_bwf_image(&info, &info.images[i]) != 0)
      goto err;
  }

  if (write_bwf_index(&info) != 0)
    goto err;
  
  if (free_bwf_save_info(&info) != 0) {
    printf("** ERROR writing file data\n");
    return 1;
  }

  printf("-> done.\n");
  return 0;

 err:
  free_bwf_save_info(&info);
  return 1;
}
