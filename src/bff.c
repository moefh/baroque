/* bff.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <stb_image.h>

#include "bff.h"
#include "debug.h"
#include "gfx.h"
#include "model.h"
#include "matrix.h"
#include "room.h"

#define MAX_ROOM_MESHES  256

static int set_file_pos(struct BFF_READER *bff, uint32_t pos)
{
  if (fseek(bff->f, pos, SEEK_SET) != 0)
    return 1;
  return 0;
}

static int get_file_pos(struct BFF_READER *bff, uint32_t *pos)
{
  long n = ftell(bff->f);
  if (n < 0)
    return 1;
  *pos = (uint32_t) n;
  return 0;
}

static int read_data(struct BFF_READER *bff, void *data, size_t size)
{
  if (fread(data, 1, size, bff->f) != size)
    return 1;
  return 0;
}

static int read_u8(struct BFF_READER *bff, uint8_t *ret)
{
  int c = fgetc(bff->f);
  if (c == EOF)
    return 1;
  *ret = (uint8_t) c;
  return 0;
}

static int read_u16(struct BFF_READER *bff, uint16_t *ret)
{
  unsigned char b[2];
  if (read_data(bff, b, 2) != 0)
    return 1;
  
  *ret = (b[1] << 8) | b[0];
  return 0;
}

static int read_u32(struct BFF_READER *bff, uint32_t *ret)
{
  unsigned char b[4];
  if (read_data(bff, b, 4) != 0)
    return 1;
  
  *ret = (((unsigned int)b[3])<<24) | (b[2] << 16) | (b[1] << 8) | b[0];
  return 0;
}

static int read_f32(struct BFF_READER *bff, float *ret)
{
  union {
    float f;
    uint32_t u;
  } pun;

  uint32_t u;
  if (read_u32(bff, &u) != 0)
    return 1;

  pun.u = u;
  *ret = pun.f;
  return 0;
}

static int read_f32_vec(struct BFF_READER *bff, float *vec, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    if (read_f32(bff, &vec[i]) != 0)
      return 1;
  }
  return 0;
}

static struct GFX_MESH *load_bff_mesh(struct BFF_READER *bff, uint32_t type, uint32_t info, void *data, uint32_t *tex0_index, uint32_t *tex1_index)
{
  uint16_t vtx_type, ind_type;
  uint32_t vtx_size, ind_size;
  uint32_t ind_count;

  if (read_u32(bff, &vtx_size) != 0 ||
      read_u32(bff, &ind_size) != 0 ||
      read_u32(bff, &ind_count) != 0 ||
      read_u16(bff, &vtx_type) != 0 ||
      read_u16(bff, &ind_type) != 0) {
    return NULL;
  }
  
  struct MODEL_MESH *mesh = new_model_mesh(vtx_type, vtx_size, ind_type, ind_size, ind_count);
  if (! mesh)
    return NULL;

  if (read_u32(bff, tex0_index) != 0 ||
      read_u32(bff, tex1_index) != 0 ||
      read_f32_vec(bff, mesh->matrix, 16) != 0 ||
      read_data(bff, mesh->vtx, mesh->vtx_size) != 0 ||
      read_data(bff, mesh->ind, mesh->ind_size) != 0) {
    free(mesh);
    return NULL;
  }

  struct GFX_MESH *gfx_mesh = gfx_upload_model_mesh(mesh, type, info, data);
  free(mesh);
  
  return gfx_mesh;
}

static struct GFX_TEXTURE *load_bff_texture(struct BFF_READER *bff)
{
  int width, height, n_chan;
  void *data = stbi_load_from_file(bff->f, &width, &height, &n_chan, 0);
  if (! data)
    return NULL;

  struct MODEL_TEXTURE tex;
  tex.width = width;
  tex.height = height;
  tex.n_chan = n_chan;
  tex.data = data;
  struct GFX_TEXTURE *gfx_tex = gfx_upload_model_texture(&tex, 0);
  free(data);

  return gfx_tex;
}

/* ========================================================================================
 * BMF reader
 */

struct BMF_READER {
  struct BFF_READER bff;
  
  size_t n_meshes;
  struct GFX_MESH *mesh[MODEL_MAX_MESHES];
  uint32_t mesh_texture_index[MODEL_MAX_MESHES];
};

static int read_bmf_header(struct BMF_READER *bmf)
{
  char header[4];
  if (read_data(&bmf->bff, header, 4) != 0)
    return 1;
  if (memcmp(header, "BMF1", 4) != 0)
    return 1;

  return 0;
}

static int load_bmf_meshes(struct BMF_READER *bmf, uint32_t type, uint32_t info, void *data)
{
  uint16_t n_meshes;
  if (read_u16(&bmf->bff, &n_meshes) != 0)
    return 1;
  if (n_meshes > MODEL_MAX_MESHES)
    return 1;

  bmf->n_meshes = n_meshes;
  for (uint16_t i = 0; i < n_meshes; i++) {
    uint32_t tex0_index, tex1_index;
    bmf->mesh[i] = load_bff_mesh(&bmf->bff, type, info, data, &tex0_index, &tex1_index);
    if (! bmf->mesh[i])
      return 1;
    bmf->mesh_texture_index[i] = tex0_index;
  }
  return 0;
}

static int load_bmf_textures(struct BMF_READER *bmf)
{
  uint16_t n_textures;
  if (read_u16(&bmf->bff, &n_textures) != 0)
    return 1;

  for (uint16_t i = 0; i < n_textures; i++) {
    struct GFX_TEXTURE *texture = load_bff_texture(&bmf->bff);
    if (! texture)
      return 1;
    for (size_t mesh = 0; mesh < bmf->n_meshes; mesh++) {
      if (bmf->mesh_texture_index[mesh] == i)
        bmf->mesh[mesh]->texture = texture;
    }
  }
  return 0;
}

int load_bmf(const char *filename, uint32_t type, uint32_t info, void *data)
{
  struct BMF_READER bmf;
  
  bmf.bff.f = fopen(filename, "rb");
  if (! bmf.bff.f)
    return 1;

  if (read_bmf_header(&bmf) != 0)
    goto err;

  if (load_bmf_meshes(&bmf, type, info, data) != 0)
    goto err;

  if (load_bmf_textures(&bmf) != 0)
    goto err;
  
  fclose(bmf.bff.f);
  return 0;

 err:
  fclose(bmf.bff.f);
  return 1;
}

/* ========================================================================================
 * BWF reader
 */

static int read_bwf_index(struct BWF_READER *bwf)
{
  char header[4];
  if (read_data(&bwf->bff, header, 4) != 0)
    return 1;
  if (memcmp(header, "BWF1", 4) != 0)
    return 1;

  uint32_t index_off;
  if (read_u32(&bwf->bff, &index_off) != 0)
    return 1;

  if (set_file_pos(&bwf->bff, index_off) != 0)
    return 1;

  if (read_u32(&bwf->bff, &bwf->n_rooms) != 0)
    return 1;
  for (uint32_t i = 0; i < bwf->n_rooms; i++) {
    if (read_u32(&bwf->bff, &bwf->room_off[i]) != 0)
      return 1;
  }

  if (read_u32(&bwf->bff, &bwf->n_textures) != 0)
    return 1;
  for (uint32_t i = 0; i < bwf->n_textures; i++) {
    if (read_u32(&bwf->bff, &bwf->texture_off[i]) != 0)
      return 1;
  }

  return 0;
}

static int load_bwf_mesh(struct BWF_READER *bwf, struct ROOM *room)
{
  uint32_t tex0_index, tex1_index;
  struct GFX_MESH *gfx_mesh = load_bff_mesh(&bwf->bff, GFX_MESH_TYPE_ROOM, room->index, room, &tex0_index, &tex1_index);
  if (! gfx_mesh)
    return 1;

  if (tex0_index != 0xffffffff) {
    uint32_t cur_file_pos;
    if (get_file_pos(&bwf->bff, &cur_file_pos) != 0)
      return 1;
    
    if (tex0_index >= bwf->n_textures ||
        set_file_pos(&bwf->bff, bwf->texture_off[tex0_index]) != 0)
      return 1;

    gfx_mesh->texture = load_bff_texture(&bwf->bff);
    if (! gfx_mesh->texture)
      return 1;
    
    if (set_file_pos(&bwf->bff, cur_file_pos) != 0)
      return 1;
  } else {
    gfx_mesh->texture = NULL;
  }
  return 0;
}

static int read_bwf_room_info(struct BWF_READER *bwf, struct ROOM *room)
{
  if (read_f32_vec(&bwf->bff, room->pos, 3) != 0)
    return 1;

  uint8_t n_neighbors;
  if (read_u8(&bwf->bff, &n_neighbors) != 0)
    return 1;
  room->n_neighbors = n_neighbors;
  for (uint8_t i = 0; i < n_neighbors; i++) {
    if (read_u32(&bwf->bff, &room->neighbor_index[i]) != 0)
      return 1;
  }

  memset(room->tiles, 0, sizeof(room->tiles));
  uint8_t x_tiles_start, x_tiles_size;
  uint8_t y_tiles_start, y_tiles_size;
  if (read_u8(&bwf->bff, &x_tiles_start) != 0 ||
      read_u8(&bwf->bff, &x_tiles_size) != 0 ||
      read_u8(&bwf->bff, &y_tiles_start) != 0 ||
      read_u8(&bwf->bff, &y_tiles_size) != 0)
    return 1;
  for (int y = 0; y < y_tiles_size; y++) {
    for (int x = 0; x < x_tiles_size; x++) {
      if (read_u16(&bwf->bff, &room->tiles[y][x]) != 0)
        return 1;
    }
  }
  return 0;
}

int load_bwf_room(struct BWF_READER *bwf, struct ROOM *room)
{
  if (room->index < 0 || (uint32_t)room->index >= bwf->n_rooms)
    return 1;

  if (set_file_pos(&bwf->bff, bwf->room_off[room->index]) != 0)
    return 1;

  if (read_bwf_room_info(bwf, room) != 0)
    return 1;
  
  uint16_t n_meshes;
  if (read_u16(&bwf->bff, &n_meshes) != 0)
    return 1;

  for (uint32_t i = 0; i < n_meshes; i++) {
    if (load_bwf_mesh(bwf, room) != 0) {
      // TODO: cleanup
      return 1;
    }
  }

  return 0;
}

int open_bwf(struct BWF_READER *bwf, const char *filename)
{
  bwf->bff.f = fopen(filename, "rb");
  if (! bwf->bff.f)
    return 1;

  if (read_bwf_index(bwf) != 0) {
    fclose(bwf->bff.f);
    return 1;
  }
  return 0;
}

void close_bwf(struct BWF_READER *bwf)
{
  fclose(bwf->bff.f);
}
