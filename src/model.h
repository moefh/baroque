/* model.h */

#ifndef MODEL_H_FILE
#define MODEL_H_FILE

#include <stdint.h>

#define MODEL_MAX_IMAGES  16
#define MODEL_MAX_MESHES  16

#define MODEL_MESH_VTX_POS             0
#define MODEL_MESH_VTX_POS_UV1         2
#define MODEL_MESH_VTX_POS_UV2         3
#define MODEL_MESH_VTX_POS_NORMAL      1
#define MODEL_MESH_VTX_POS_NORMAL_UV1  4
#define MODEL_MESH_VTX_POS_NORMAL_UV2  5

#define MODEL_MESH_IND_U8  0
#define MODEL_MESH_IND_U16 1
#define MODEL_MESH_IND_U32 2

struct MODEL_MESH {
  uint32_t vtx_size;
  uint32_t ind_size;
  uint16_t tex_image;
  uint8_t  vtx_type;
  uint8_t  ind_type;
  
  void *vtx;
  void *ind;
  unsigned char data[];
};

struct MODEL_IMAGE {
  uint32_t width;
  uint32_t height;
  uint32_t n_chan;
  unsigned char data[];
};

struct MODEL {
  int n_meshes;
  struct MODEL_MESH *meshes[MODEL_MAX_MESHES];

  int n_images;
  struct MODEL_IMAGE *images[MODEL_MAX_IMAGES];
};

int read_glb_model(struct MODEL *model, const char *filename);
void free_model(struct MODEL *model);

struct MODEL_MESH *new_model_mesh(uint8_t vtx_type, uint32_t vtx_size, uint8_t ind_type, uint32_t ind_size);

#endif /* MODEL_H_FILE */
