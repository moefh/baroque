/* glb.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "glb.h"
#include "matrix.h"

#define GLTF_MAX_ACCESSORS     64
#define GLTF_MAX_BUFFERS       16
#define GLTF_MAX_BUFFER_VIEWS  64
#define GLTF_MAX_IMAGES        16
#define GLTF_MAX_MATERIALS     16
#define GLTF_MAX_MESHES        16
#define GLTF_MAX_NODES         64
#define GLTF_MAX_NODE_CHILDREN 32
#define GLTF_MAX_SCENES        8
#define GLTF_MAX_SCENE_NODES   8
#define GLTF_MAX_SAMPLERS      16
#define GLTF_MAX_TEXTURES      16

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

#define GLTF_SAMPLER_FILTER_NEAREST                9728
#define GLTF_SAMPLER_FILTER_LINEAR                 9729
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST  9985
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR  9986
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR   9987

#define GLTF_SAMPLER_WRAP_REPEAT          10497
#define GLTF_SAMPLER_WRAP_CLAMP_TO_EDGE   33071
#define GLTF_SAMPLER_WRAP_MIRRORED_REPEAT 33648

struct GLTF_ACCESOR {
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

struct GLTF_MESH {
  uint16_t mode;
  uint16_t n_attribute_accessors;
  uint16_t attribute_accessors[4];
  uint16_t indices_accessor;
  uint16_t material;
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
  struct GLTF_ACCESOR     accessors[GLTF_MAX_ACCESSORS];
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
};

/* ================================================================ */
/* JSON reader */
/* ================================================================ */

struct JSON_READER {
  struct GLTF_DATA gltf;
  char *json;
  char json_text[];
};

typedef int (json_object_reader)(struct JSON_READER *reader, const char *name, void *p);
typedef int (json_array_reader)(struct JSON_READER *reader, int i, void *p);

static struct JSON_READER *new_json_reader(size_t json_text_len)
{
  struct JSON_READER *reader = malloc(sizeof *reader + json_text_len);
  if (! reader)
    return NULL;
  reader->json = reader->json_text;
  return reader;
}

static void free_json_reader(struct JSON_READER *reader)
{
  free(reader);
}

static int is_json_hexdigit(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_json_digit(char c)
{
  return (c >= '0' && c <= '9');
}

static int read_json_string(struct JSON_READER *reader, char *dest, size_t dest_len)
{
  if (*reader->json != '"')
    return 1;
  reader->json++;
  
  size_t n = 0;
  while (*reader->json != '"') {
    if (*reader->json == '\0' || n+1 >= dest_len)
      return 1;

    char c;
    if (*reader->json == '\\') {
      reader->json++;
      switch (*reader->json) {
      case '\'': c = '\\'; break;
      case '/': c = '/'; break;
      case '"': c = '"'; break;
      case 'b': c = '\b'; break;
      case 'f': c = '\f'; break;
      case 'n': c = '\n'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;

      case 'u':
        if (! is_json_hexdigit(reader->json[1]) || ! is_json_hexdigit(reader->json[2]) ||
            ! is_json_hexdigit(reader->json[3]) || ! is_json_hexdigit(reader->json[4]))
          return 1;
        c = '?';
        break;

      default:
        return 1;
      }
    } else {
      c = *reader->json++;
    }

    if (dest)
      dest[n] = c;
    n++;
  }
  reader->json++;
  if (dest)
    dest[n] = '\0';
  return 0;
}

static int read_json_number(struct JSON_READER *reader, double *num)
{
  char *end;
  double n = strtod(reader->json, &end);
  if (end == reader->json)
    return 1;
  if (num)
    *num = n;
  reader->json = end;
  return 0;
}

static int read_json_float(struct JSON_READER *reader, float *num)
{
  double n;
  if (read_json_number(reader, &n) != 0)
    return 1;
  if (num)
    *num = (float) n;
  return 0;
}

static int read_json_u16(struct JSON_READER *reader, uint16_t *num)
{
  double n;
  if (read_json_number(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT16_MAX)
    return 1;
  if (num)
    *num = (uint16_t) n;
  return 0;
}

static int read_json_u32(struct JSON_READER *reader, uint32_t *num)
{
  double n;
  if (read_json_number(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT32_MAX)
    return 1;
  if (num)
    *num = (uint32_t) n;
  return 0;
}

static void skip_json_spaces(struct JSON_READER *reader)
{
  while (*reader->json == ' ' || *reader->json == '\t' || *reader->json == '\r' || *reader->json == '\n')
    reader->json++;
}

static int skip_json_string(struct JSON_READER *reader)
{
  return read_json_string(reader, NULL, 0x10000);
}

static int skip_json_number(struct JSON_READER *reader)
{
  return read_json_number(reader, NULL);
}

static int skip_json_value(struct JSON_READER *reader)
{
  skip_json_spaces(reader);
  //printf("skipping json value '%.20s'\n", reader->json);

  if (strncmp(reader->json, "false", sizeof("false")-1) == 0) {
    reader->json += sizeof("false")-1;
    return 0;
  }

  if (strncmp(reader->json, "true", sizeof("true")-1) == 0) {
    reader->json += sizeof("false")-1;
    return 0;
  }

  if (strncmp(reader->json, "null", sizeof("null")-1) == 0) {
    reader->json += sizeof("null")-1;
    return 0;
  }
  
  switch (*reader->json) {
  case '"':
    return skip_json_string(reader);
    
  case '[':
    reader->json++;
    skip_json_spaces(reader);
    if (*reader->json == ']') {
      reader->json++;
      return 0;
    }
    while (1) {
      if (skip_json_value(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json == ']') {
        reader->json++;
        return 0;
      }
      if (*reader->json != ',')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
    }
    return 0;

  case '{':
    reader->json++;
    skip_json_spaces(reader);
    if (*reader->json == '}') {
      reader->json++;
      return 0;
    }
    while (1) {
      if (skip_json_string(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json != ':')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
      if (skip_json_value(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json == '}') {
        reader->json++;
        return 0;
      }
      if (*reader->json != ',')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
    }
    return 0;

  default:
    if (*reader->json == '-' || is_json_digit(*reader->json))
      return skip_json_number(reader);
    return 1;
  }
}

static int read_json_array(struct JSON_READER *reader, json_array_reader *reader_fun, void *reader_val)
{
  skip_json_spaces(reader);
  if (*reader->json != '[')
    return 1;
  reader->json++;
  skip_json_spaces(reader);
  if (*reader->json == ']') {
    reader->json++;
    return 0;
  }
  
  int element_index = 0;
  while (1) {
    int ret;
    if (reader_fun)
      ret = reader_fun(reader, element_index++, reader_val);
    else
      ret = skip_json_value(reader);
    if (ret)
      return 1;
    skip_json_spaces(reader);
    if (*reader->json == ']') {
      reader->json++;
      return 0;
    }
    if (*reader->json != ',')
      return 1;
    reader->json++;
    skip_json_spaces(reader);
  }
}

static int read_json_object(struct JSON_READER *reader, json_object_reader *reader_fun, void *reader_val)
{
  skip_json_spaces(reader);
  if (*reader->json != '{')
    return 1;
  reader->json++;
  skip_json_spaces(reader);
  if (*reader->json == '}') {
    reader->json++;
    return 0;
  }
  
  while (1) {
    char name[64];
    
    skip_json_spaces(reader);
    if (read_json_string(reader, name, sizeof(name)) != 0)
      return 1;
    skip_json_spaces(reader);
    if (*reader->json != ':')
      return 1;
    reader->json++;
    skip_json_spaces(reader);

    int ret;
    if (reader_fun)
      ret = reader_fun(reader, name, reader_val);
    else
      ret = skip_json_value(reader);
    if (ret != 0)
      return ret;
    skip_json_spaces(reader);
    if (*reader->json == '}') {
      reader->json++;
      return 0;
    }
    if (*reader->json != ',')
      return 1;
    reader->json++;
    skip_json_spaces(reader);
  }
}

static int read_json_boolean(struct JSON_READER *reader, int *val)
{
  skip_json_spaces(reader);
  if (strncmp(reader->json, "false", sizeof("false")-1) == 0) {
    reader->json += sizeof("false")-1;
    *val = 0;
    return 0;
  }
  if (strncmp(reader->json, "true", sizeof("true")-1) == 0) {
    reader->json += sizeof("true")-1;
    *val = 1;
    return 0;
  }
  return 1;
}

/* ================================================================ */
/* GLTF JSON reader */
/* ================================================================ */

struct READ_FLOAT_VEC_INFO {
  float *vec;
  size_t max_vals;
  size_t num_vals;
};

struct READ_U16_VEC_INFO {
  uint16_t *vec;
  size_t max_vals;
  size_t num_vals;
};

static int read_float_vector_element(struct JSON_READER *reader, int index, void *data)
{
  struct READ_FLOAT_VEC_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_float(reader, &info->vec[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

static int read_float_vector(struct JSON_READER *reader, float *vec, size_t max_vals, size_t *num_vals)
{
  struct READ_FLOAT_VEC_INFO info = {
    .vec = vec,
    .max_vals = max_vals,
    .num_vals = 0,
  };
  if (read_json_array(reader, read_float_vector_element, &info) != 0)
    return 1;
  *num_vals = info.num_vals;
  return 0;
}

static int read_u16_vector_element(struct JSON_READER *reader, int index, void *data)
{
  struct READ_U16_VEC_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_u16(reader, &info->vec[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

static int read_u16_vector(struct JSON_READER *reader, uint16_t *vec, size_t max_vals, size_t *num_vals)
{
  struct READ_U16_VEC_INFO info = {
    .vec = vec,
    .max_vals = max_vals,
    .num_vals = 0,
  };
  if (read_json_array(reader, read_u16_vector_element, &info) != 0)
    return 1;
  *num_vals = info.num_vals;
  return 0;
}

static int read_asset_prop(struct JSON_READER *reader, const char *name, void *data)
{
  if (strcmp(name, "version") == 0) {
    int *version_ok = data;
    char version[32];
    if (read_json_string(reader, version, sizeof(version)) != 0)
      return 1;
    // accept version 2.x
    if (version[0] == '2' && version[1] == '.')
      *version_ok = 1;
    return 0;
  }
  
  return skip_json_value(reader);
}

static int read_asset_object(struct JSON_READER *reader)
{
  int asset_ver_ok = 0;
  if (read_json_object(reader, read_asset_prop, &asset_ver_ok) != 0)
    return 1;
  if (! asset_ver_ok)
    return 1;
  return 0;
}

struct JSON_NODE_INFO {
  struct GLTF_NODE *node;
  float rotation[4];
  float scale[3];
  float translation[3];
  uint8_t has_matrix;
  uint8_t has_rotation;
  uint8_t has_scale;
  uint8_t has_translation;
};

static int read_node_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct JSON_NODE_INFO *info = data;
  size_t num;
  
  if (strcmp(name, "mesh") == 0) {
    if (read_json_u16(reader, &info->node->mesh) != 0)
      return 1;
  } else if (strcmp(name, "rotation") == 0) {
    if (read_float_vector(reader, info->rotation, 4, &num) != 0 || num != 4)
      return 1;
    info->has_rotation = 1;
  } else if (strcmp(name, "scale") == 0) {
    if (read_float_vector(reader, info->scale, 3, &num) != 0 || num != 3)
      return 1;
    info->has_scale = 1;
  } else if (strcmp(name, "translation") == 0) {
    if (read_float_vector(reader, info->translation, 3, &num) != 0 || num != 3)
      return 1;
    info->has_translation = 1;
  } else if (strcmp(name, "matrix") == 0) {
    if (read_float_vector(reader, info->node->matrix, 16, &num) != 0 || num != 16)
      return 1;
    info->has_matrix = 1;
  } else {
    printf("-> skipping 'node.%s'\n", name);
    return skip_json_value(reader);
  }
  return 0;
}

static int read_nodes_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_NODES) {
    printf("* WARNING: skipping node %d: too many nodes!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_NODE *node = &reader->gltf.nodes[index];
  struct JSON_NODE_INFO info = {
    .has_rotation = 0,
    .has_scale = 0,
    .has_translation = 0,
    .node = node,
  };
  
  mat4_id(node->matrix);
  node->mesh = GLTF_NONE;
  node->n_children = 0;
  if (read_json_object(reader, read_node_prop, &info) != 0)
    return 1;

  // don't bother with non-mesh nodes
  if (node->mesh == GLTF_NONE)
    return 0;
  
  if (! info.has_matrix) {
    if (info.has_rotation)
      printf("TODO: apply rotation to node matrix\n");
    if (info.has_scale)
      printf("TODO: apply scale to node matrix\n");
    if (info.has_translation)
      printf("TODO: apply translation to node matrix\n");
  }
  return 0;
}

static int read_buffer_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_BUFFER *buffer = data;
  
  if (strcmp(name, "byteLength") == 0)
    return read_json_u32(reader, &buffer->byte_length);

  printf("-> skipping 'buffer.%s'\n", name);
  return skip_json_value(reader);
}

static int read_buffers_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_BUFFERS) {
    printf("* WARNING: skipping buffer %d: too many buffers!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_BUFFER *buffer = &reader->gltf.buffers[index];

  return read_json_object(reader, read_buffer_prop, buffer);
}

static int read_buffer_view_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_BUFFER_VIEW *buffer_view = data;
  
  if (strcmp(name, "byteLength") == 0)
    return read_json_u32(reader, &buffer_view->byte_length);

  if (strcmp(name, "byteOffset") == 0)
    return read_json_u32(reader, &buffer_view->byte_offset);

  if (strcmp(name, "buffer") == 0)
    return read_json_u16(reader, &buffer_view->buffer);

  if (strcmp(name, "target") == 0)
    return read_json_u16(reader, &buffer_view->target);
  
  printf("-> skipping 'bufferView.%s'\n", name);
  return skip_json_value(reader);
}

static int read_buffer_views_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_BUFFER_VIEWS) {
    printf("* WARNING: skipping buffer view %d: too many buffer views!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf.buffer_views[index];
  buffer_view->byte_offset = 0;
  buffer_view->byte_length = 0;
  buffer_view->target = 0;
  buffer_view->buffer = 0;
  
  return read_json_object(reader, read_buffer_view_prop, buffer_view);
  //int ret = read_json_object(reader, read_buffer_view_prop, buffer_view);
  //printf("got buffer view for buffer %d, offset=%-5d len=%-5d target=%d\n",
  //       buffer_view->buffer, buffer_view->byte_offset, buffer_view->byte_length, buffer_view->target);
  //return ret;
}

static int read_sampler_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_SAMPLER *sampler = data;
  
  if (strcmp(name, "magFilter") == 0)
    return read_json_u16(reader, &sampler->mag_filter);
  if (strcmp(name, "minFilter") == 0)
    return read_json_u16(reader, &sampler->min_filter);
  if (strcmp(name, "wrapS") == 0)
    return read_json_u16(reader, &sampler->wrap_s);
  if (strcmp(name, "wrapT") == 0)
    return read_json_u16(reader, &sampler->wrap_t);

  printf("-> skipping 'sampler.%s'\n", name);
  return skip_json_value(reader);
}

static int read_samplers_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_SAMPLERS) {
    printf("* WARNING: skipping sampler %d: too many samplers!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_SAMPLER *sampler = &reader->gltf.samplers[index];
  sampler->mag_filter = GLTF_SAMPLER_FILTER_LINEAR;
  sampler->min_filter = GLTF_SAMPLER_FILTER_LINEAR;
  sampler->wrap_s = GLTF_SAMPLER_WRAP_REPEAT;
  sampler->wrap_t = GLTF_SAMPLER_WRAP_REPEAT;
  
  return read_json_object(reader, read_sampler_prop, sampler);
}

static int read_image_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_IMAGE *image = data;
  
  if (strcmp(name, "bufferView") == 0)
    return read_json_u16(reader, &image->buffer_view);

  if (strcmp(name, "mimeType") == 0) {
    char mime_type[32];
    if (read_json_string(reader, mime_type, sizeof(mime_type)) != 0)
      return 1;
    if (strcmp(mime_type, "image/jpeg") == 0) {
      image->mime_type = GLTF_IMAGE_MIME_TYPE_JPEG;
      return 0;
    }
    if (strcmp(mime_type, "image/png") == 0) {
      image->mime_type = GLTF_IMAGE_MIME_TYPE_PNG;
      return 0;
    }
    printf("* ERROR: invalid image mime type: '%s'\n", mime_type);
    return 1;
  }
  
  printf("-> skipping 'image.%s'\n", name);
  return skip_json_value(reader);
}

static int read_images_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_IMAGES) {
    printf("* WARNING: skipping image %d: too many images!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_IMAGE *image = &reader->gltf.images[index];
  image->buffer_view = 0;
  image->mime_type = 0;
  
  return read_json_object(reader, read_image_prop, image);
}

static int read_scene_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_SCENE *scene = data;
  
  if (strcmp(name, "nodes") == 0) {
    size_t n_nodes;
    if (read_u16_vector(reader, scene->nodes, GLTF_MAX_SCENE_NODES, &n_nodes) != 0)
      return 1;
    scene->n_nodes = n_nodes;
    //printf("-> got %u nodes in scene\n", scene->n_nodes);
    return 0;
  }

  printf("-> skipping 'scene.%s'\n", name);
  return skip_json_value(reader);
}

static int read_scenes_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_SCENES) {
    printf("* WARNING: skipping scene %d: too many scenes!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_SCENE *scene = &reader->gltf.scenes[index];
  scene->n_nodes = 0;
  
  return read_json_object(reader, read_scene_prop, scene);
}

static int read_texture_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_TEXTURE *texture = data;
  
  if (strcmp(name, "sampler") == 0) {
    if (read_json_u16(reader, &texture->sampler) != 0)
      return -1;
    return 0;
  }
  if (strcmp(name, "source") == 0) {
    if (read_json_u16(reader, &texture->source_image) != 0)
      return -1;
    return 0;
  }

  printf("-> skipping 'texture.%s'\n", name);
  return skip_json_value(reader);
}

static int read_textures_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_TEXTURES) {
    printf("* WARNING: skipping texture %d: too many textures!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_TEXTURE *texture = &reader->gltf.textures[index];
  texture->sampler = 0;
  texture->source_image = 0;

  return read_json_object(reader, read_texture_prop, texture);
}

static int read_material_texture_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_MATERIAL_TEXTURE *mat_tex = data;

  if (strcmp(name, "index") == 0)
    return read_json_u16(reader, &mat_tex->index);

  if (strcmp(name, "texCoord") == 0)
    return read_json_u16(reader, &mat_tex->tex_coord);
  
  printf("-> skipping material texture '%s'\n", name);
  return skip_json_value(reader);
}

static int read_material_pbr_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_MATERIAL *material = data;

  if (strcmp(name, "baseColorTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->color_tex);

  if (strcmp(name, "baseColorFactor") == 0) {
    size_t num;
    if (read_float_vector(reader, material->base_color_factor, 4, &num) != 0 || num != 4)
      return 1;
    return 0;
  }

  if (strcmp(name, "metallicFactor") == 0)
    return read_json_float(reader, &material->metallic_factor);
  
  if (strcmp(name, "roughnessFactor") == 0)
    return read_json_float(reader, &material->roughness_factor);
  
  printf("-> skipping 'material.pbrMetallicRoughness.%s'\n", name);
  return skip_json_value(reader);
}

static int read_material_prop(struct JSON_READER *reader, const char *name, void *data)
{
  struct GLTF_MATERIAL *material = data;
  
  if (strcmp(name, "alphaCutoff") == 0)
    return read_json_float(reader, &material->alpha_cutoff);
  
  if (strcmp(name, "alphaMode") == 0) {
    char alpha_mode[32];
    if (read_json_string(reader, alpha_mode, sizeof(alpha_mode)) != 0)
      return 1;
    if (strcmp(alpha_mode, "OPAQUE") == 0)
      material->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_OPAQUE;
    else if (strcmp(alpha_mode, "MASK") == 0)
      material->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_MASK;
    else if (strcmp(alpha_mode, "BLEND") == 0)
      material->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_BLEND;
    else
      return 1;
    return 0;
  }

  if (strcmp(name, "doubleSided") == 0) {
    int double_sided;
    if (read_json_boolean(reader, &double_sided) != 0)
      return 1;
    material->double_sided = double_sided;
    return 0;
  }

  if (strcmp(name, "normalTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->normal_tex);

  if (strcmp(name, "occlusionTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->occlusion_tex);

  if (strcmp(name, "emissiveTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->emissive_tex);
  
  if (strcmp(name, "pbrMetallicRoughness") == 0)
    return read_json_object(reader, read_material_pbr_prop, material);
  
  printf("-> skipping 'material.%s'\n", name);
  return skip_json_value(reader);
}

static void init_material_texture(struct GLTF_MATERIAL_TEXTURE *mat_tex)
{
  mat_tex->index = GLTF_NONE;
  mat_tex->tex_coord = 0;
}

static int read_materials_element(struct JSON_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_MATERIALS) {
    printf("* WARNING: skipping material %d: too many materials!\n", index);
    return skip_json_value(reader);
  }
  struct GLTF_MATERIAL *material = &reader->gltf.materials[index];
  init_material_texture(&material->color_tex);
  init_material_texture(&material->normal_tex);
  init_material_texture(&material->occlusion_tex);
  init_material_texture(&material->emissive_tex);
  vec3_load(material->emissive_factor, 0.0, 0.0, 0.0);
  vec4_load(material->base_color_factor, 1.0, 1.0, 1.0, 1.0);
  material->metallic_factor = 1.0;
  material->roughness_factor = 1.0;
  material->alpha_cutoff = 0.5;
  material->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_OPAQUE;
  material->double_sided = 0;
  
  return read_json_object(reader, read_material_prop, material);
}

static int read_gltf_prop(struct JSON_READER *reader, const char *name, void *data)
{
  if (strcmp(name, "asset") == 0)
    return read_asset_object(reader);
  
  if (strcmp(name, "nodes") == 0)
    return read_json_array(reader, read_nodes_element, NULL);
  
  if (strcmp(name, "scene") == 0)
    return read_json_u16(reader, &reader->gltf.scene);
  
  if (strcmp(name, "scenes") == 0)
    return read_json_array(reader, read_scenes_element, NULL);

  if (strcmp(name, "buffers") == 0)
    return read_json_array(reader, read_buffers_element, NULL);
  
  if (strcmp(name, "bufferViews") == 0)
    return read_json_array(reader, read_buffer_views_element, NULL);

  if (strcmp(name, "samplers") == 0)
    return read_json_array(reader, read_samplers_element, NULL);
  
  if (strcmp(name, "images") == 0)
    return read_json_array(reader, read_images_element, NULL);
  
  if (strcmp(name, "textures") == 0)
    return read_json_array(reader, read_textures_element, NULL);

  if (strcmp(name, "materials") == 0)
    return read_json_array(reader, read_materials_element, NULL);
  
  // TODO:
  
  if (strcmp(name, "meshes") == 0)
    return skip_json_value(reader);

  if (strcmp(name, "accessors") == 0)
    return skip_json_value(reader);
  
  // END TODO
  
  printf("-> skipping '%s'\n", name);
  return skip_json_value(reader);
}

static int read_gltf_json(struct JSON_READER *reader)
{
  if (read_json_object(reader, read_gltf_prop, NULL) != 0)
    return 1;
  
  skip_json_spaces(reader);
  if (*reader->json != '\0')
    return 1;
  return 0;
}

/* ================================================================ */
/* GLB reader */
/* ================================================================ */

static int read_bytes(FILE *f, void *data, size_t len)
{
  if (fread(data, 1, len, f) != len)
    return 1;
  return 0;
}

static uint32_t get_u32(void *data, size_t off)
{
  unsigned char *p = data;

  return p[off] | (p[off+1] << 8) | (p[off+2] << 16) | ((uint32_t)p[off+3] << 24);
}

static void init_glb(struct GLB_FILE *glb)
{
  glb->n_meshes = 0;
  glb->n_images = 0;
}

int read_glb(struct GLB_FILE *glb, const char *filename)
{
  struct JSON_READER *reader = NULL;
  init_glb(glb);

  FILE *f = fopen(filename, "rb");
  if (! f)
    goto err;

  // read and check file header
  unsigned char header[12];
  if (read_bytes(f, header, sizeof(header)) != 0)
    goto err;
  if (memcmp(header, "glTF", 4) != 0)
    goto err;
  uint32_t ver = get_u32(header, 4);
  if (ver != 2)
    goto err;

  while (1) {
    unsigned char chunk_header[8];
    if (read_bytes(f, chunk_header, sizeof(chunk_header)) != 0)
      goto err;
    uint32_t chunk_len = get_u32(chunk_header, 0);

    // JSON chunk
    if (memcmp(chunk_header + 4, "JSON", 4) == 0) {
      if (chunk_len == 0 || chunk_len > 0x10000)    // sanity check
        goto err;
      reader = new_json_reader(chunk_len+1);
      if (! reader)
        goto err;
      if (read_bytes(f, reader->json, chunk_len) != 0)
        goto err;
      reader->json[chunk_len] = '\0';
      if (read_gltf_json(reader) != 0)
        goto err;
      continue;
    }

    // BIN\0 chunk
    if (memcmp(chunk_header + 4, "BIN", 4) == 0) {
      if (! reader)
        goto err;
      // TODO: read GLTF data into glb
      break;
    }

    // unknown chunk before BIN\0
    goto err;
  }
 
  free_json_reader(reader);
  fclose(f);
  return 0;
  
 err:
  if (f)
    fclose(f);
  free_json_reader(reader);
  free_glb(glb);
  return 1;
}

void free_glb(struct GLB_FILE *glb)
{
  for (int i = 0; i < glb->n_meshes; i++)
    free(glb->meshes[i]);

  for (int i = 0; i < glb->n_images; i++)
    free(glb->images[i]);
}

