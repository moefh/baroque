/* grid.c */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "grid.h"
#include "debug.h"
#include "matrix.h"

struct MODEL_MESH *make_grid(int num, float size)
{
  int n_vtx = 4 * num;
  size_t vtx_size = n_vtx * 3 * sizeof(float);
  
  int n_ind = 4 * num;
  size_t ind_size = n_ind * sizeof(uint16_t);
  
  struct MODEL_MESH *mesh = new_model_mesh(MODEL_MESH_VTX_POS, vtx_size, MODEL_MESH_IND_U16, ind_size, n_ind);
  if (! mesh)
    return NULL;
  mat4_id(mesh->matrix);

  float min = -(num-1)/2.0 * size;
  float max =  (num-1)/2.0 * size;
  
  float *vtx = mesh->vtx;
  uint16_t *ind = mesh->ind;
  for (int i = 0; i < num; i++) {
    // vertical
    ind[0] = 4*i + 0;
    ind[1] = 4*i + 1;
    ind += 2;

    vec3_load(vtx, min + i*size, 0, min);
    vtx += 3;
    vec3_load(vtx, min + i*size, 0, max);
    vtx += 3;

    // horizontal
    ind[0] = 4*i + 2;
    ind[1] = 4*i + 3;
    ind += 2;

    vec3_load(vtx, min, 0, min + i*size);
    vtx += 3;
    vec3_load(vtx, max, 0, min + i*size);
    vtx += 3;
  }

  return mesh;
}

void free_grid(struct MODEL_MESH *mesh)
{
  free(mesh);
}

static float grid_tile_vtx_data[] = {
  // pos           uv
  +0.5, 0, +0.5,   1, 1,
  -0.5, 0, +0.5,   0, 1,
  +0.5, 0, -0.5,   1, 0,
  -0.5, 0, -0.5,   0, 0,
};

static struct MODEL_MESH *make_grid_tiles_mesh(void)
{
  int n_vtx = 4;
  size_t vtx_size = n_vtx * (3+2) * sizeof(float);

  int n_ind = 6;
  size_t ind_size = n_ind * sizeof(unsigned char);

  struct MODEL_MESH *mesh = new_model_mesh(MODEL_MESH_VTX_POS_UV1, vtx_size, MODEL_MESH_IND_U8, ind_size, n_ind);
  if (! mesh)
    return NULL;
  mat4_id(mesh->matrix);

  memcpy(mesh->vtx, grid_tile_vtx_data, sizeof(grid_tile_vtx_data));
  
  unsigned char *ind = mesh->ind;
  ind[0] = 0;
  ind[1] = 2;
  ind[2] = 1;
  ind[3] = 1;
  ind[4] = 2;
  ind[5] = 3;

  return mesh;
}

static void init_grid_tiles(struct GRID_TILES *tiles)
{
  tiles->mesh = NULL;
  tiles->texture.data = NULL;
}

void set_tile(unsigned char *data, int x, int y, uint32_t color)
{
  data[4*(256*y + x) + 0] = (color>>24) & 0xff;
  data[4*(256*y + x) + 1] = (color>>16) & 0xff;
  data[4*(256*y + x) + 2] = (color>> 8) & 0xff;
  data[4*(256*y + x) + 3] = (color>> 0) & 0xff;
}

int make_grid_tiles(struct GRID_TILES *tiles, int width, int height)
{
  init_grid_tiles(tiles);
  
  tiles->texture.data = calloc(width * height, 4);
  if (! tiles->texture.data)
    goto err;
  tiles->texture.width = width;
  tiles->texture.height = height;
  tiles->texture.n_chan = 4;

  tiles->mesh = make_grid_tiles_mesh();
  if (! tiles->mesh)
    goto err;
  return 0;

 err:
  free_grid_tiles(tiles);
  return 1;
}

void free_grid_tiles(struct GRID_TILES *tiles)
{
  free(tiles->texture.data);
  free(tiles->mesh);
}
