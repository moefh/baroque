/* glb.h */

#ifndef GLB_H_FILE
#define GLB_H_FILE

#include <stdint.h>

#define GLB_MAX_IMAGES  16
#define GLB_MAX_MESHES  16

#define GLB_MESH_VTX_POS            0
#define GLB_MESH_VTX_POS_UV         1
#define GLB_MESH_VTX_POS_NORMAL     2
#define GLB_MESH_VTX_POS_UV_NORMAL  3

#define GLB_MESH_IND_U8  0
#define GLB_MESH_IND_U16 1
#define GLB_MESH_IND_U32 2

struct GLB_MESH {
  unsigned char vtx_type;
  unsigned char ind_type;
  int n_vtx;
  int n_ind;
  
  float *vtx;
  void *indices;
  unsigned char data[];
};

struct GLB_IMAGE {
  int width;
  int height;
  int n_chan;
  unsigned char data[];
};

struct GLB_FILE {
  int n_meshes;
  struct GLB_MESH *meshes[GLB_MAX_MESHES];

  int n_images;
  struct GLB_IMAGE *images[GLB_MAX_IMAGES];
};

int read_glb(struct GLB_FILE *glb, const char *filename);
void free_glb(struct GLB_FILE *glb);

#endif /* GLB_H_FILE */
