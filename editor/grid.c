/* grid.c */

#include <stdlib.h>
#include <stdint.h>

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
