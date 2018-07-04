/* gltf.h */

#ifndef GLTF_H_FILE
#define GLTF_H_FILE

#include <stdint.h>
#include "json.h"

#define GLTF_MAX_MESH_PRIMITIVES 8
#define GLTF_MAX_NODE_CHILDREN   32
#define GLTF_MAX_SCENE_NODES     8
#define GLTF_MAX_ACCESSORS       64
#define GLTF_MAX_BUFFERS         16
#define GLTF_MAX_BUFFER_VIEWS    64
#define GLTF_MAX_IMAGES          16
#define GLTF_MAX_MATERIALS       16
#define GLTF_MAX_MESHES          16
#define GLTF_MAX_NODES           64
#define GLTF_MAX_SCENES          8
#define GLTF_MAX_SAMPLERS        16
#define GLTF_MAX_TEXTURES        16

#define GLTF_NONE ((uint16_t)0xffff)

#define GLTF_ACCESSOR_COMP_TYPE_BYTE   5120
#define GLTF_ACCESSOR_COMP_TYPE_UBYTE  5121
#define GLTF_ACCESSOR_COMP_TYPE_SHORT  5122
#define GLTF_ACCESSOR_COMP_TYPE_USHORT 5123
#define GLTF_ACCESSOR_COMP_TYPE_UINT   5125
#define GLTF_ACCESSOR_COMP_TYPE_FLOAT  5126

#define GLTF_ACCESSOR_TYPE_SCALAR 0
#define GLTF_ACCESSOR_TYPE_VEC2   1
#define GLTF_ACCESSOR_TYPE_VEC3   2
#define GLTF_ACCESSOR_TYPE_VEC4   3
#define GLTF_ACCESSOR_TYPE_MAT2   4
#define GLTF_ACCESSOR_TYPE_MAT3   5
#define GLTF_ACCESSOR_TYPE_MAT4   6

#define GLTF_BUFFER_VIEW_TARGET_ARRAY         34962
#define GLTF_BUFFER_VIEW_TARGET_ELEMENT_ARRAY 34963

#define GLTF_IMAGE_MIME_TYPE_PNG  0
#define GLTF_IMAGE_MIME_TYPE_JPEG 1

#define GLTF_MATERIAL_ALPHA_MODE_OPAQUE 0
#define GLTF_MATERIAL_ALPHA_MODE_MASK   1
#define GLTF_MATERIAL_ALPHA_MODE_BLEND  2

#define GLTF_MESH_MODE_POINTS         0
#define GLTF_MESH_MODE_LINES          1
#define GLTF_MESH_MODE_LINE_LOOP      2
#define GLTF_MESH_MODE_LINE_STRIP     3
#define GLTF_MESH_MODE_TRIANGLES      4
#define GLTF_MESH_MODE_TRIANGLE_STRIP 5
#define GLTF_MESH_MODE_TRIANGLE_FAN   6

#define GLTF_MESH_NUM_ATTRIBS       8
#define GLTF_MESH_ATTRIB_POSITION   0
#define GLTF_MESH_ATTRIB_NORMAL     1
#define GLTF_MESH_ATTRIB_TANGENT    2
#define GLTF_MESH_ATTRIB_TEXCOORD_0 3
#define GLTF_MESH_ATTRIB_TEXCOORD_1 4
#define GLTF_MESH_ATTRIB_TEXCOORD_2 5
#define GLTF_MESH_ATTRIB_TEXCOORD_3 6
#define GLTF_MESH_ATTRIB_TEXCOORD_4 7

#define GLTF_SAMPLER_FILTER_NEAREST                9728
#define GLTF_SAMPLER_FILTER_LINEAR                 9729
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST  9985
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR  9986
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR   9987

#define GLTF_SAMPLER_WRAP_REPEAT          10497
#define GLTF_SAMPLER_WRAP_CLAMP_TO_EDGE   33071
#define GLTF_SAMPLER_WRAP_MIRRORED_REPEAT 33648

struct GLTF_ACCESSOR {
  uint32_t byte_offset;
  uint32_t count;
  uint16_t buffer_view;
  uint16_t type;
  uint16_t component_type;
  uint8_t normalized;
};

struct GLTF_BUFFER {
  uint32_t byte_length;
};

struct GLTF_BUFFER_VIEW {
  uint32_t byte_offset;
  uint32_t byte_length;
  uint32_t byte_stride;
  uint16_t target;
  uint16_t buffer;
};

struct GLTF_IMAGE {
  uint16_t buffer_view;
  uint16_t mime_type;
  char name[64];
};

struct GLTF_MATERIAL_TEXTURE {
  uint16_t index;
  uint16_t tex_coord;
};

struct GLTF_MATERIAL {
  struct GLTF_MATERIAL_TEXTURE color_tex;
  struct GLTF_MATERIAL_TEXTURE normal_tex;
  struct GLTF_MATERIAL_TEXTURE occlusion_tex;
  struct GLTF_MATERIAL_TEXTURE emissive_tex;
  float base_color_factor[4];
  float emissive_factor[3];
  float metallic_factor;
  float roughness_factor;
  float alpha_cutoff;
  uint8_t alpha_mode;
  uint8_t double_sided;
};

struct GLTF_MESH_PRIMITIVE {
  uint16_t mode;
  uint16_t attribs_present;
  uint16_t attribs[GLTF_MESH_NUM_ATTRIBS];
  uint16_t indices_accessor;
  uint16_t material;
};

struct GLTF_MESH {
  uint16_t n_primitives;
  struct GLTF_MESH_PRIMITIVE primitives[GLTF_MAX_MESH_PRIMITIVES];
};

struct GLTF_NODE {
  float matrix[16];
  uint16_t mesh;
  uint16_t children[GLTF_MAX_NODE_CHILDREN];
  uint16_t n_children;
};

struct GLTF_SAMPLER {
  uint16_t mag_filter;
  uint16_t min_filter;
  uint16_t wrap_s;
  uint16_t wrap_t;
};

struct GLTF_SCENE {
  uint16_t n_nodes;
  uint16_t nodes[GLTF_MAX_SCENE_NODES];
};

struct GLTF_TEXTURE {
  uint16_t sampler;
  uint16_t source_image;
};

struct GLTF_DATA {
  struct GLTF_ACCESSOR    accessors[GLTF_MAX_ACCESSORS];
  struct GLTF_BUFFER      buffers[GLTF_MAX_BUFFERS];
  struct GLTF_BUFFER_VIEW buffer_views[GLTF_MAX_BUFFER_VIEWS];
  struct GLTF_IMAGE       images[GLTF_MAX_IMAGES];
  struct GLTF_MATERIAL    materials[GLTF_MAX_MATERIALS];
  struct GLTF_MESH        meshes[GLTF_MAX_MESHES];
  struct GLTF_NODE        nodes[GLTF_MAX_NODES];
  struct GLTF_SCENE       scenes[GLTF_MAX_SCENES];
  struct GLTF_SAMPLER     samplers[GLTF_MAX_SAMPLERS];
  struct GLTF_TEXTURE     textures[GLTF_MAX_TEXTURES];
  uint16_t scene;

  struct JSON_READER json;
  char json_text[];
};

struct GLB_FILE {
  struct GLTF_DATA *gltf;
  FILE *file;
  uint32_t data_off;
  uint32_t data_len;
};

struct GLTF_DATA *read_gltf(const char *filename);
void free_gltf(struct GLTF_DATA *gltf);

int open_glb(struct GLB_FILE *glb, const char *filename);
void close_glb(struct GLB_FILE *glb);

#endif /* GLTF_H_FILE */
