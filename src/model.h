/* model.h */

#ifndef MODEL_H_FILE
#define MODEL_H_FILE

#include <stdint.h>

#define MODEL_TEXTURE_NONE 0xffff

#define MODEL_MAX_TEXTURES 16
#define MODEL_MAX_MESHES   16

#define MODEL_MESH_VTX_POS             0
#define MODEL_MESH_VTX_POS_UV1         1
#define MODEL_MESH_VTX_POS_UV2         2
#define MODEL_MESH_VTX_POS_NORMAL      3
#define MODEL_MESH_VTX_POS_NORMAL_UV1  4
#define MODEL_MESH_VTX_POS_NORMAL_UV2  5

#define MODEL_MESH_IND_U8  0
#define MODEL_MESH_IND_U16 1
#define MODEL_MESH_IND_U32 2

struct MODEL_MESH {
  uint32_t vtx_size;
  uint32_t ind_size;
  uint32_t ind_count;
  uint16_t tex0_index;
  uint16_t tex1_index;
  uint8_t  vtx_type;
  uint8_t  ind_type;
  
  void *vtx;
  void *ind;
  unsigned char data[];
};

struct MODEL_TEXTURE {
  uint32_t width;
  uint32_t height;
  uint32_t n_chan;
  unsigned char *data;
};

struct MODEL {
  int n_meshes;
  struct MODEL_MESH *meshes[MODEL_MAX_MESHES];

  int n_textures;
  struct MODEL_TEXTURE textures[MODEL_MAX_TEXTURES];
};

int read_glb_model(struct MODEL *model, const char *filename);
void free_model(struct MODEL *model);

#endif /* MODEL_H_FILE */
