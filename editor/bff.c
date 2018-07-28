/* bff.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "room.h"
#include "load.h"
#include "model.h"

#define DEBUG_BFF_WRITER
#ifdef DEBUG_BFF_WRITER
#define debug_log printf
#else
#define debug_log(...)
#endif

struct BFF_WRITER;

typedef int (translate_tex_index_func)(struct BFF_WRITER *bff, struct MODEL *model, int model_tex_index, uint32_t *tex_index, void *p);

struct BFF_WRITER {
  FILE *f;
  size_t cur_file_offset;
  translate_tex_index_func *translate_tex_index;
};

static int open_bff(struct BFF_WRITER *bff, const char *filename, translate_tex_index_func *translate_tex_index)
{
  bff->f = fopen(filename, "wb");
  if (! bff->f)
    return 1;
  bff->cur_file_offset = 0;
  bff->translate_tex_index = translate_tex_index;
  return 0;
}

static int close_bff(struct BFF_WRITER *bff)
{
  if (bff->f) {
    if (fclose(bff->f) != 0)
      return 1;
  }
  return 0;
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

static int set_file_pos(struct BFF_WRITER *bff, size_t pos)
{
  if (fseek(bff->f, pos, SEEK_SET) != 0)
    return 1;
  bff->cur_file_offset = pos;
  return 0;
}

static int write_data(struct BFF_WRITER *bff, void *data, size_t size)
{
  if (fwrite(data, 1, size, bff->f) != size)
    return 1;
  bff->cur_file_offset += size;
  return 0;
}

static int write_u8(struct BFF_WRITER *bff, uint8_t n)
{
  if (fputc(n, bff->f) == EOF)
    return 1;
  bff->cur_file_offset++;
  return 0;
}

static int write_u16(struct BFF_WRITER *bff, uint16_t n)
{
  if (fputc((n      ) & 0xff, bff->f) == EOF ||
      fputc((n >>  8) & 0xff, bff->f) == EOF)
    return 1;
  bff->cur_file_offset += 2;
  return 0;
}

static int write_u32(struct BFF_WRITER *bff, uint32_t n)
{
  if (fputc((n      ) & 0xff, bff->f) == EOF ||
      fputc((n >>  8) & 0xff, bff->f) == EOF ||
      fputc((n >> 16) & 0xff, bff->f) == EOF ||
      fputc((n >> 24) & 0xff, bff->f) == EOF)
    return 1;
  bff->cur_file_offset += 4;
  return 0;
}

static int write_f32(struct BFF_WRITER *bff, float n)
{
  union {
    float f;
    uint32_t u;
  } pun;

  pun.f = n;
  return write_u32(bff, pun.u);
}

static int write_f32_array(struct BFF_WRITER *bff, float *arr, int num)
{
  for (int i = 0; i < num; i++)
    if (write_f32(bff, arr[i]) != 0)
      return 1;
  return 0;
}

static int write_mat4(struct BFF_WRITER *bff, float *mat)
{
  return write_f32_array(bff, mat, 16);
}

static int write_model_meshes(struct BFF_WRITER *bff, struct MODEL *model, void *data)
{
  if (write_u16(bff, model->n_meshes) != 0)
    return 1;

  for (int i = 0; i < model->n_meshes; i++) {
    struct MODEL_MESH *mesh = model->meshes[i];

    uint32_t tex0_index, tex1_index;
    if (bff->translate_tex_index(bff, model, mesh->tex0_index, &tex0_index, data) != 0 ||
        bff->translate_tex_index(bff, model, mesh->tex1_index, &tex1_index, data) != 0)
      return 1;

    if (write_u32(bff, mesh->vtx_size) != 0 ||
        write_u32(bff, mesh->ind_size) != 0 ||
        write_u32(bff, mesh->ind_count) != 0 ||
        write_u16(bff, mesh->vtx_type) != 0 ||
        write_u16(bff, mesh->ind_type) != 0 ||
        write_u32(bff, tex0_index) != 0 ||
        write_u32(bff, tex1_index) != 0 ||
        write_mat4(bff, mesh->matrix) != 0 ||
        write_data(bff, mesh->vtx, mesh->vtx_size) != 0 ||
        write_data(bff, mesh->ind, mesh->ind_size) != 0)
      return 1;
  }

  return 0;
}

/* ==========================================================================================
 * Write BCF
 */

#define BCF_VERSION '1'

static int get_bcf_tex_index(struct BFF_WRITER *bff, struct MODEL *model, int model_tex_index, uint32_t *bcf_tex_index, void *data)
{
  *bcf_tex_index = model_tex_index;
  return 0;
}

static int write_bcf_header(struct BFF_WRITER *bff)
{
  char header[4];
  
  memcpy(header, "BCF", 3);
  header[3] = BCF_VERSION;
  if (write_data(bff, header, 4) != 0) {
    debug_log("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

static int write_bcf_model_textures(struct BFF_WRITER *bff, struct MODEL *model)
{
  if (write_u16(bff, model->n_textures) != 0)
    return 1;
  
  for (int i = 0; i < model->n_textures; i++) {
    debug_log("-> writing texture %d\n", i);
    
    struct MODEL_TEXTURE *tex = &model->textures[i];
    uint32_t data_size = tex->height;
    if (write_u32(bff, data_size) != 0 ||
        write_data(bff, tex->data, data_size) != 0) {
      debug_log("** ERROR: can't write texture data\n");
      return 1;
    }
  }
  return 0;
}

static int write_bcf_keyframes(struct BFF_WRITER *bff, struct MODEL_BONE_KEYFRAME *keyframes, int n_keyframes, int n_comp)
{
  if (write_u16(bff, n_keyframes) != 0)
    return 1;
  for (int i = 0; i < n_keyframes; i++) {
    if (write_f32(bff, keyframes->time) != 0)
      return 1;
    if (write_f32_array(bff, keyframes->data, n_comp) != 0)
      return 1;
  }
  return 0;
}

static int write_bcf_skeleton(struct BFF_WRITER *bff, struct MODEL_SKELETON *skel)
{
  uint32_t n_keyframes = 0;
  for (int anim_index = 0; anim_index < skel->n_animations; anim_index++) {
    struct MODEL_ANIMATION *anim = &skel->animations[anim_index];
    for (int bone_index = 0; bone_index < skel->n_bones; bone_index++) {
      struct MODEL_BONE_ANIMATION *bone_anim = &anim->bones[bone_index];
      n_keyframes += bone_anim->n_trans_keyframes;
      n_keyframes += bone_anim->n_rot_keyframes;
      n_keyframes += bone_anim->n_scale_keyframes;
    }
  }

  if (write_u16(bff, skel->n_bones) != 0 ||
      write_u16(bff, skel->n_animations) != 0 ||
      write_u32(bff, n_keyframes) != 0)
    return 1;

  debug_log("-> writing %d bones\n", skel->n_bones);
  for (int bone_index = 0; bone_index < skel->n_bones; bone_index++) {
    struct MODEL_BONE *bone = &skel->bones[bone_index];
    if (write_u16(bff, bone->parent) != 0 ||
        write_mat4(bff, bone->inv_matrix) != 0 ||
        write_mat4(bff, bone->pose_matrix) != 0)
      return 1;
  }

  debug_log("-> writing %d keyframes\n", (int) n_keyframes);
  for (int anim_index = 0; anim_index < skel->n_animations; anim_index++) {
    struct MODEL_ANIMATION *anim = &skel->animations[anim_index];
    for (int bone_index = 0; bone_index < skel->n_bones; bone_index++) {
      struct MODEL_BONE_ANIMATION *bone_anim = &anim->bones[bone_index];
      if (write_bcf_keyframes(bff, bone_anim->trans_keyframes, bone_anim->n_trans_keyframes, 3) != 0)
        return 1;
      if (write_bcf_keyframes(bff, bone_anim->rot_keyframes, bone_anim->n_rot_keyframes, 4) != 0)
        return 1;
      if (write_bcf_keyframes(bff, bone_anim->scale_keyframes, bone_anim->n_scale_keyframes, 3) != 0)
        return 1;
    }
  }

  return 0;
}

int write_bcf_file(const char *bcf_filename, const char *glb_filename)
{
  struct MODEL model;
  struct MODEL_SKELETON skel;
  if (read_glb_animated_model(&model, &skel, glb_filename, MODEL_FLAGS_PACKED_IMAGES) != 0) {
    debug_log("** ERROR: can't read model from '%s'\n", glb_filename);
    return 1;
  }

  struct BFF_WRITER bff;
  if (open_bff(&bff, bcf_filename, get_bcf_tex_index) != 0) {
    debug_log("** ERROR opening file '%s'\n", bcf_filename);
    goto err;
  }

  if (write_bcf_header(&bff) != 0)
    goto err;
  
  debug_log("-> writing model meshes\n");
  if (write_model_meshes(&bff, &model, NULL) != 0)
    goto err;

  if (write_bcf_model_textures(&bff, &model) != 0)
    goto err;
 
  if (write_bcf_skeleton(&bff, &skel) != 0)
    goto err;
  
  free_model_skeleton(&skel);
  free_model(&model);
  if (close_bff(&bff) != 0) {
    free_model(&model);
    debug_log("** ERROR writing file data\n");
    return 1;
  }
  return 0;

 err:
  free_model_skeleton(&skel);
  free_model(&model);
  close_bff(&bff);
  return 1;
}

/* ==========================================================================================
 * Write BMF
 */

#define BMF_VERSION '1'

static int get_bmf_tex_index(struct BFF_WRITER *bff, struct MODEL *model, int model_tex_index, uint32_t *bwf_tex_index, void *data)
{
  *bwf_tex_index = model_tex_index;
  return 0;
}

static int write_bmf_header(struct BFF_WRITER *bff)
{
  char header[4];
  
  memcpy(header, "BMF", 3);
  header[3] = BMF_VERSION;
  if (write_data(bff, header, 4) != 0) {
    debug_log("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

int write_bmf_model_textures(struct BFF_WRITER *bff, struct MODEL *model)
{
  if (write_u16(bff, model->n_textures) != 0)
    return 1;
  
  for (int i = 0; i < model->n_textures; i++) {
    debug_log("-> writing texture %d\n", i);
    
    struct MODEL_TEXTURE *tex = &model->textures[i];
    uint32_t data_size = tex->height;
    if (write_u32(bff, data_size) != 0 ||
        write_data(bff, tex->data, data_size) != 0) {
      debug_log("** ERROR: can't write texture data\n");
      return 1;
    }
  }
  return 0;
}

int write_bmf_file(const char *bmf_filename, const char *glb_filename)
{
  struct MODEL model;
  if (read_glb_model(&model, glb_filename, MODEL_FLAGS_PACKED_IMAGES) != 0) {
    debug_log("** ERROR: can't read model from '%s'\n", glb_filename);
    return 1;
  }

  struct BFF_WRITER bff;
  if (open_bff(&bff, bmf_filename, get_bmf_tex_index) != 0) {
    debug_log("** ERROR opening file '%s'\n", bmf_filename);
    goto err;
  }

  if (write_bmf_header(&bff) != 0)
    goto err;
  
  debug_log("-> writing model meshes\n");
  if (write_model_meshes(&bff, &model, NULL) != 0)
    goto err;

  if (write_bmf_model_textures(&bff, &model) != 0)
    goto err;
  
  free_model(&model);
  if (close_bff(&bff) != 0) {
    free_model(&model);
    debug_log("** ERROR writing file data\n");
    return 1;
  }
  return 0;

 err:
  free_model(&model);
  close_bff(&bff);
  return 1;
}

/* ==========================================================================================
 * Write BWF
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

struct BWF_WRITER {
  struct BFF_WRITER bff;

  int n_rooms;
  struct ROOM_INFO *rooms;

  int n_images;
  int alloc_images;
  struct IMAGE_INFO *images;
};

static int open_bwf(struct BWF_WRITER *bwf, const char *filename, translate_tex_index_func *func)
{
  if (open_bff(&bwf->bff, filename, func) != 0)
    return 1;
  
  bwf->n_rooms = 0;
  bwf->rooms = NULL;
  
  bwf->n_images = 0;
  bwf->alloc_images = 0;
  bwf->images = NULL;
  return 0;
}

static int close_bwf(struct BWF_WRITER *bwf)
{
  free(bwf->rooms);
  free(bwf->images);
  return close_bff(&bwf->bff);
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

static struct IMAGE_INFO *get_image(struct BWF_WRITER *bwf, struct MODEL_TEXTURE *tex, struct ROOM_INFO *room_info, int tex_index)
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

static int write_bwf_header(struct BWF_WRITER *bwf)
{
  char header[8];
  
  memcpy(header + 0, "BWF", 3);
  header[3] = BWF_VERSION;
  memset(header + 4, 0xff, 4);
  if (write_data(&bwf->bff, header, 8) != 0) {
    debug_log("** ERROR: can't write file header\n");
    return 1;
  }
  return 0;
}

static int get_bwf_tex_index(struct BFF_WRITER *bff, struct MODEL *model, int model_tex_index, uint32_t *bwf_tex_index, void *data)
{
  struct BWF_WRITER *bwf = (struct BWF_WRITER *) bff;
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

static int write_bwf_room_neighbors(struct BWF_WRITER *bwf, struct EDITOR_ROOM *room)
{
  if (write_u8(&bwf->bff, room->n_neighbors) != 0)
    return 1;
  for (int i = 0; i < room->n_neighbors; i++) {
    if (write_u32(&bwf->bff, room->neighbors[i]->serialization_index) != 0)
      return 1;
  }
  return 0;
}

static int write_bwf_room_tiles(struct BWF_WRITER *bwf, uint16_t (*tiles)[256])
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

  if (write_u8(&bwf->bff, x_min) != 0 ||
      write_u8(&bwf->bff, x_size) != 0 ||
      write_u8(&bwf->bff, y_min) != 0 ||
      write_u8(&bwf->bff, y_size) != 0)
    return 1;
  for (int y = 0; y < y_size; y++) {
    for (int x = 0; x < x_size; x++) {
      if (write_u16(&bwf->bff, tiles[y_min + y][x_min + x]) != 0)
        return 1;
    }
  }
  return 0;
}

static int write_bwf_room(struct BWF_WRITER *bwf, struct ROOM_INFO *room_info)
{
  struct EDITOR_ROOM *room = room_info->room;
  
  debug_log("-> writing room %d (%s)\n", room->serialization_index, room->name);

  room_info->file_offset = bwf->bff.cur_file_offset;

  if (write_f32(&bwf->bff, room->pos[0]) != 0 ||
      write_f32(&bwf->bff, room->pos[1]) != 0 ||
      write_f32(&bwf->bff, room->pos[2]) != 0) {
    debug_log("** ERROR: can't write room position\n");
    return 1;
  }

  if (write_bwf_room_neighbors(bwf, room) != 0) {
    debug_log("** ERROR: can't write room neighbors\n");
    return 1;
  }
  
  if (write_bwf_room_tiles(bwf, room->tiles) != 0) {
    debug_log("** ERROR: can't write room tiles\n");
    return 1;
  }

  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, MODEL_FLAGS_IMAGE_REFS) != 0) {
    debug_log("** ERROR: can't read model from '%s'\n", filename);
    return 1;
  }
  if (write_model_meshes(&bwf->bff, &model, room_info) != 0) {
    debug_log("** ERROR: can't write room model\n");
    free_model(&model);
    return 1;
  }

  free_model(&model);
  return 0;
}

static int write_bwf_image(struct BWF_WRITER *bwf, struct IMAGE_INFO *image)
{
  char filename[256];
  snprintf(filename, sizeof(filename), "data/%s.glb", image->room_info->room->name);
  
  struct MODEL model;
  if (read_glb_model(&model, filename, MODEL_FLAGS_IMAGE_REFS) != 0) {
    debug_log("** ERROR: can't read model images from '%s'\n", filename);
    return 1;
  }
  if (image->model_tex_index >= model.n_textures) {
    debug_log("** ERROR: invalid image %d for file '%s'\n", image->model_tex_index, filename);
    free_model(&model);
    return 1;
  }
  struct MODEL_TEXTURE *tex = &model.textures[image->model_tex_index];
  debug_log("-> writing image %d (%s)\n", image->index, (char*)tex->data);
  uint32_t data_offset = tex->width;
  uint32_t data_size = tex->height;
  free_model(&model);

  void *data = malloc(data_size);
  if (! data)
    return 1;

  if (read_file_block(filename, data, data_offset, data_size) != 0) {
    debug_log("** ERROR: can't read image from '%s'\n", filename);
    free(data);
    return 1;
  }

  image->file_offset = bwf->bff.cur_file_offset;
  if (write_u32(&bwf->bff, data_size) != 0 ||
      write_data(&bwf->bff, data, data_size) != 0) {
    debug_log("** ERROR: can't write image data\n");
    free(data);
    return 1;
  }

  free(data);
  return 0;
}

static int write_bwf_index(struct BWF_WRITER *bwf)
{
  debug_log("-> writing index\n");

  size_t index_offset = bwf->bff.cur_file_offset;
  
  if (write_u32(&bwf->bff, bwf->n_rooms) != 0) {
    debug_log("** ERROR: can't write room index\n");
    return 1;
  }
  for (int i = 0; i < bwf->n_rooms; i++) {
    if (write_u32(&bwf->bff, bwf->rooms[i].file_offset) != 0) {
      debug_log("** ERROR: can't write room index\n");
      return 1;
    }
  }

  if (write_u32(&bwf->bff, bwf->n_images) != 0) {
    debug_log("** ERROR: can't write image index\n");
    return 1;
  }
  for (int i = 0; i < bwf->n_images; i++) {
    if (write_u32(&bwf->bff, bwf->images[i].file_offset) != 0) {
      debug_log("** ERROR: can't write image index\n");
      return 1;
    }
  }

  if (set_file_pos(&bwf->bff, 4) != 0) {
    debug_log("** ERROR: can't seek to header\n");
    return 1;
  }
  if (write_u32(&bwf->bff, index_offset) != 0) {
    debug_log("** ERROR: can't write index offset to header\n");
    return 1;
  }
  
  return 0;
}

int write_bwf_file(const char *filename, struct EDITOR_ROOM_LIST *rooms)
{
  struct BWF_WRITER bwf;
  if (open_bwf(&bwf, filename, get_bwf_tex_index) != 0) {
    debug_log("** ERROR opening file '%s'\n", filename);
    return 1;
  }

  bwf.rooms = create_room_info(rooms, &bwf.n_rooms);
  if (! bwf.rooms) {
    debug_log("** ERROR: out of memory for rooms info\n");
    goto err;
  }
  
  if (write_bwf_header(&bwf) != 0)
    goto err;
  
  for (int i = 0; i < bwf.n_rooms; i++) {
    if (write_bwf_room(&bwf, &bwf.rooms[i]) != 0)
      goto err;
  }

  for (int i = 0; i < bwf.n_images; i++) {
    if (write_bwf_image(&bwf, &bwf.images[i]) != 0)
      goto err;
  }

  if (write_bwf_index(&bwf) != 0)
    goto err;
  
  if (close_bwf(&bwf) != 0) {
    debug_log("** ERROR writing file data\n");
    return 1;
  }

  debug_log("-> done.\n");
  return 0;

 err:
  close_bwf(&bwf);
  return 1;
}
