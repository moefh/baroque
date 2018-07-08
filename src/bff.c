/* bff.c */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stb_image.h>

#include "bff.h"
#include "debug.h"
#include "gfx.h"
#include "model.h"
#include "matrix.h"

#define MAX_ROOM_MESHES  256

static int set_file_pos(struct BFF *bff, uint32_t pos)
{
  if (fseek(bff->f, pos, SEEK_SET) != 0)
    return 1;
  return 0;
}

static int get_file_pos(struct BFF *bff, uint32_t *pos)
{
  long n = ftell(bff->f);
  if (n < 0)
    return 1;
  *pos = (uint32_t) n;
  return 0;
}

static int read_data(struct BFF *bff, void *data, size_t size)
{
  if (fread(data, 1, size, bff->f) != size)
    return 1;
  return 0;
}

static int read_u8(struct BFF *bff, uint8_t *ret)
{
  int c = fgetc(bff->f);
  if (c == EOF)
    return 1;
  *ret = (uint8_t) c;
  return 0;
}

static int read_u16(struct BFF *bff, uint16_t *ret)
{
  unsigned char b[2];
  if (read_data(bff, b, 2) != 0)
    return 1;
  
  *ret = (b[1] << 8) | b[0];
  return 0;
}

static int read_u32(struct BFF *bff, uint32_t *ret)
{
  unsigned char b[4];
  if (read_data(bff, b, 4) != 0)
    return 1;
  
  *ret = (((unsigned int)b[3])<<24) | (b[2] << 16) | (b[1] << 8) | b[0];
  return 0;
}

static int read_f32(struct BFF *bff, float *ret)
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

static int read_f32_vec(struct BFF *bff, float *vec, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    if (read_f32(bff, &vec[i]) != 0)
      return 1;
  }
  return 0;
}

static int read_index(struct BFF *bff)
{
  char header[4];
  if (read_data(bff, header, 4) != 0)
    return 1;
  if (memcmp(header, "BFF1", 4) != 0)
    return 1;

  uint32_t index_off;
  if (read_u32(bff, &index_off) != 0)
    return 1;

  if (set_file_pos(bff, index_off) != 0)
    return 1;

  if (read_u32(bff, &bff->n_rooms) != 0)
    return 1;
  for (uint32_t i = 0; i < bff->n_rooms; i++) {
    if (read_u32(bff, &bff->room_off[i]) != 0)
      return 1;
  }

  if (read_u32(bff, &bff->n_textures) != 0)
    return 1;
  for (uint32_t i = 0; i < bff->n_textures; i++) {
    if (read_u32(bff, &bff->texture_off[i]) != 0)
      return 1;
  }

  return 0;
}

static struct MODEL_MESH *read_mesh(struct BFF *bff, uint32_t *tex0_index, uint32_t *tex1_index)
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

  return mesh;
}

static struct GFX_TEXTURE *load_bff_texture(struct BFF *bff, int texture)
{
  if (texture < 0 || (uint32_t)texture >= bff->n_textures)
    return NULL;

  if (set_file_pos(bff, bff->texture_off[texture]) != 0)
    return NULL;

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

static int load_bff_mesh(struct BFF *bff, int room)
{
  uint32_t tex0_index, tex1_index;
  struct MODEL_MESH *mesh = read_mesh(bff, &tex0_index, &tex1_index);
  if (! mesh)
    return 1;

  struct GFX_MESH *gfx_mesh = gfx_upload_model_mesh(mesh, GFX_MESH_TYPE_ROOM, room, NULL);
  if (! gfx_mesh)
    goto err;

  if (tex0_index != 0xffffffff) {
    uint32_t cur_file_pos;
    if (get_file_pos(bff, &cur_file_pos) != 0)
      goto err;
    
    gfx_mesh->texture = load_bff_texture(bff, tex0_index);
    if (! gfx_mesh->texture)
      goto err;
    
    if (set_file_pos(bff, cur_file_pos) != 0)
      goto err;
  } else {
    gfx_mesh->texture = NULL;
  }
  free(mesh);
  return 0;

 err:
  free(mesh);
  return 1;
}

static int read_bff_room_info(struct BFF *bff)
{
  /* This is a hack: we currently discard all room info. This should
   * be saved in a global room info storage somewhere.
   */
  
  float pos[3];
  if (read_f32_vec(bff, pos, 3) != 0)
    return 1;

  uint8_t n_neighbors;
  if (read_u8(bff, &n_neighbors) != 0)
    return 1;
  for (uint8_t i = 0; i < n_neighbors; i++) {
    uint32_t neighbor_index;
    if (read_u32(bff, &neighbor_index) != 0)
      return 1;
  }

  uint8_t tiles_dim[4];
  if (read_data(bff, tiles_dim, 4) != 0)
    return 1;
  int tiles_size = 2 * (int)tiles_dim[1] * (int)tiles_dim[3];
  static uint16_t tiles[256][256];
  if (read_data(bff, tiles, tiles_size) != 0)
    return 1;
  
  return 0;
}

int load_bff_room(struct BFF *bff, int room)
{
  if (room < 0 || (uint32_t)room >= bff->n_rooms)
    return 1;

  if (set_file_pos(bff, bff->room_off[room]) != 0)
    return 1;

  if (read_bff_room_info(bff) != 0)
    return 1;
  
  uint16_t n_meshes;
  if (read_u16(bff, &n_meshes) != 0)
    return 1;

  for (uint32_t i = 0; i < n_meshes; i++) {
    if (load_bff_mesh(bff, room) != 0) {
      // TODO: cleanup
      return 1;
    }
  }

  return 0;
}

int open_bff(struct BFF *bff, const char *filename)
{
  bff->f = fopen(filename, "rb");
  if (! bff->f)
    return 1;

  if (read_index(bff) != 0) {
    fclose(bff->f);
    return 1;
  }
  return 0;
}

void close_bff(struct BFF *bff)
{
  fclose(bff->f);
}
