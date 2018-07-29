/* model.h */

#ifndef MODEL_H_FILE
#define MODEL_H_FILE

#include <stdint.h>

#define MODEL_FLAGS_IMAGE_REFS    (1<<0)
#define MODEL_FLAGS_PACKED_IMAGES (1<<1)

#define MODEL_TEXTURE_NONE 0xffff

#define MODEL_MAX_TEXTURES    64
#define MODEL_MAX_MESHES      128
#define MODEL_MAX_BONES       32

#define MODEL_MESH_VTX_POS                  0
#define MODEL_MESH_VTX_POS_UV1              1
#define MODEL_MESH_VTX_POS_UV2              2
#define MODEL_MESH_VTX_POS_NORMAL           3
#define MODEL_MESH_VTX_POS_NORMAL_UV1       4
#define MODEL_MESH_VTX_POS_NORMAL_UV2       5
#define MODEL_MESH_VTX_POS_SKEL1            6
#define MODEL_MESH_VTX_POS_UV1_SKEL1        7
#define MODEL_MESH_VTX_POS_UV2_SKEL1        8
#define MODEL_MESH_VTX_POS_NORMAL_SKEL1     9
#define MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL1 10
#define MODEL_MESH_VTX_POS_NORMAL_UV2_SKEL1 11
#define MODEL_MESH_VTX_POS_SKEL2            12
#define MODEL_MESH_VTX_POS_UV1_SKEL2        13
#define MODEL_MESH_VTX_POS_UV2_SKEL2        14
#define MODEL_MESH_VTX_POS_NORMAL_SKEL2     15
#define MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL2 16
#define MODEL_MESH_VTX_POS_NORMAL_UV2_SKEL2 17

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
  float matrix[16];
  
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

struct MODEL_BONE_KEYFRAME {
  float time;
  float data[4];
};

struct MODEL_BONE_ANIMATION {
  uint16_t n_trans_keyframes;
  uint16_t n_rot_keyframes;
  uint16_t n_scale_keyframes;
  struct MODEL_BONE_KEYFRAME *trans_keyframes;
  struct MODEL_BONE_KEYFRAME *rot_keyframes;
  struct MODEL_BONE_KEYFRAME *scale_keyframes;
};

struct MODEL_ANIMATION {
  char name[32];
  float start_time;
  float end_time;
  float loop_start_time;
  float loop_end_time;
  struct MODEL_BONE_ANIMATION bones[MODEL_MAX_BONES];
};

struct MODEL_BONE {
  int parent;
  float *inv_matrix;
  float *pose_matrix;
};

struct MODEL_SKELETON {
  int n_bones;
  int n_animations;
  struct MODEL_BONE bones[MODEL_MAX_BONES];
  struct MODEL_ANIMATION *animations;
  float *float_data;
  struct MODEL_BONE_KEYFRAME *keyframe_data;
};

struct MODEL_MESH *new_model_mesh(uint8_t vtx_type, uint32_t vtx_size, uint8_t ind_type, uint32_t ind_size, uint32_t ind_count);
int read_glb_model(struct MODEL *model, const char *filename, uint32_t flags);
int read_glb_animated_model(struct MODEL *model, struct MODEL_SKELETON *skel, const char *filename, uint32_t flags);
void free_model(struct MODEL *model);
void free_model_skeleton(struct MODEL_SKELETON *skel);

#endif /* MODEL_H_FILE */
