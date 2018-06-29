/* font.c */

#include <stdlib.h>
#include <stdint.h>

#include <stb_image.h>

#include "font.h"
#include "matrix.h"
#include "debug.h"

static struct MODEL_MESH *create_font_mesh(int tex_w, int tex_h)
{
  int n_vtx = 4 * FONT_MAX_CHARS_PER_DRAW;
  size_t vtx_size = n_vtx * (3 + 2) * sizeof(float);
  
  int n_ind = 6 * FONT_MAX_CHARS_PER_DRAW;
  size_t ind_size = n_ind * sizeof(uint16_t);
  
  struct MODEL_MESH *mesh = new_model_mesh(MODEL_MESH_VTX_POS_UV1, vtx_size, MODEL_MESH_IND_U16, ind_size, n_ind);
  if (! mesh)
    return NULL;
  mat4_id(mesh->matrix);

  float u1 = 1.0 / (2*tex_w);
  float v1 = 1.0 / (2*tex_h);
  float u2 = u1 + 1.0 / tex_w * (tex_w / 16 - 1);
  float v2 = v1 + 1.0 / tex_h * (tex_h / 8 - 1);

  float vtx_info[4][6] = {
    // x  y  z    u   v
    {  0, 0, 0,   u1, v1 },
    {  1, 0, 0,   u2, v1 },
    {  0, 1, 0,   u1, v2 },
    {  1, 1, 0,   u2, v2 },
  };    
  
  float *vtx = mesh->vtx;
  uint16_t *ind = mesh->ind;
  for (int ch = 0; ch < FONT_MAX_CHARS_PER_DRAW; ch++) {
    for (int i = 0; i < 4; i++) {
      vtx[0] = (vtx_info[i][0] + ch) * 0.5;
      vtx[1] = vtx_info[i][1];
      vtx[2] = (float) ch;
      vtx[3] = vtx_info[i][3];
      vtx[4] = vtx_info[i][4];
      vtx += 3 + 2;
    }

    // clockwise because y will be multiplied by -1
    ind[0] = 4*ch + 0;
    ind[1] = 4*ch + 2;
    ind[2] = 4*ch + 1;
    ind[3] = 4*ch + 1;
    ind[4] = 4*ch + 2;
    ind[5] = 4*ch + 3;
    ind += 6;
  }

  return mesh;
}

static void init_font(struct FONT *font)
{
  font->texture.data = NULL;
  font->mesh = NULL;
}

int read_font(struct FONT *font, const char *filename)
{
  init_font(font);
  
  int width, height, n_chan;
  font->texture.data = stbi_load(filename, &width, &height, &n_chan, 4);
  if (! font->texture.data)
    goto err;
  font->texture.width = width;
  font->texture.height = height;
  font->texture.n_chan = 4;
  
  font->mesh = create_font_mesh(width, height);
  if (! font->mesh)
    goto err;
  return 0;
  
 err:
  free_font(font);
  return 1;
}

void free_font(struct FONT *font)
{
  free(font->texture.data);
  free(font->mesh);
}
