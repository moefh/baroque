/* gltf.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "gltf.h"
#include "matrix.h"

//#define DEBUG_GLTF_READER
#ifdef DEBUG_GLTF_READER
#define debug_log printf
#else
#define debug_log(s,...)
#endif

/* ================================================================ */
/* JSON parser */
/* ================================================================ */

typedef int (json_object_reader)(struct GLTF_READER *reader, const char *name, void *p);
typedef int (json_array_reader)(struct GLTF_READER *reader, int i, void *p);

static int is_json_hexdigit(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_json_digit(char c)
{
  return (c >= '0' && c <= '9');
}

static int read_json_string(struct GLTF_READER *reader, char *dest, size_t dest_len)
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

static int read_json_double(struct GLTF_READER *reader, double *num)
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

static int read_json_float(struct GLTF_READER *reader, float *num)
{
  double n;
  if (read_json_double(reader, &n) != 0)
    return 1;
  if (num)
    *num = (float) n;
  return 0;
}

static int read_json_u16(struct GLTF_READER *reader, uint16_t *num)
{
  double n;
  if (read_json_double(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT16_MAX)
    return 1;
  if (num)
    *num = (uint16_t) n;
  return 0;
}

static int read_json_u32(struct GLTF_READER *reader, uint32_t *num)
{
  double n;
  if (read_json_double(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT32_MAX)
    return 1;
  if (num)
    *num = (uint32_t) n;
  return 0;
}

static void skip_json_spaces(struct GLTF_READER *reader)
{
  while (*reader->json == ' ' || *reader->json == '\t' || *reader->json == '\r' || *reader->json == '\n')
    reader->json++;
}

static int skip_json_string(struct GLTF_READER *reader)
{
  return read_json_string(reader, NULL, 0x10000);
}

static int skip_json_number(struct GLTF_READER *reader)
{
  return read_json_double(reader, NULL);
}

static int skip_json_value(struct GLTF_READER *reader)
{
  skip_json_spaces(reader);
  //debug_log("skipping json value '%.20s'\n", reader->json);

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

static int read_json_array(struct GLTF_READER *reader, json_array_reader *reader_fun, void *reader_val)
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

static int read_json_object(struct GLTF_READER *reader, json_object_reader *reader_fun, void *reader_val)
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

static int read_json_boolean(struct GLTF_READER *reader, uint8_t *val)
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
/* glTF JSON parser */
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

static int read_float_vector_element(struct GLTF_READER *reader, int index, void *data)
{
  struct READ_FLOAT_VEC_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_float(reader, &info->vec[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

static int read_float_vector(struct GLTF_READER *reader, float *vec, size_t max_vals, size_t *num_vals)
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

static int read_u16_vector_element(struct GLTF_READER *reader, int index, void *data)
{
  struct READ_U16_VEC_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_u16(reader, &info->vec[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

static int read_u16_vector(struct GLTF_READER *reader, uint16_t *vec, size_t max_vals, size_t *num_vals)
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

static int read_asset_prop(struct GLTF_READER *reader, const char *name, void *data)
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

static int read_asset_object(struct GLTF_READER *reader)
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

static int read_node_prop(struct GLTF_READER *reader, const char *name, void *data)
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
    debug_log("-> skipping 'node.%s'\n", name);
    return skip_json_value(reader);
  }
  return 0;
}

static int read_nodes_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_NODES) {
    debug_log("* ERROR: too many nodes (%d)\n", index);
    return 1;
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
    if (info.has_translation) {
      float translation[16];
      mat4_load_translation(translation, info.translation[0], info.translation[1], info.translation[2]);
      mat4_mul_right(node->matrix, translation);
    }
    if (info.has_scale) {
      float scale[16];
      mat4_load_scale(scale, info.scale[0], info.scale[1], info.scale[2]);
      mat4_mul_right(node->matrix, scale);
    }
  }
  return 0;
}

static int read_buffer_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_BUFFER *buffer = data;
  
  if (strcmp(name, "byteLength") == 0)
    return read_json_u32(reader, &buffer->byte_length);

  debug_log("-> skipping 'buffer.%s'\n", name);
  return skip_json_value(reader);
}

static int read_buffers_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_BUFFERS) {
    debug_log("* ERROR: too many buffers (%d)\n", index);
    return 1;
  }
  struct GLTF_BUFFER *buffer = &reader->gltf.buffers[index];

  return read_json_object(reader, read_buffer_prop, buffer);
}

static int read_buffer_view_prop(struct GLTF_READER *reader, const char *name, void *data)
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
  
  debug_log("-> skipping 'bufferView.%s'\n", name);
  return skip_json_value(reader);
}

static int read_buffer_views_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_BUFFER_VIEWS) {
    debug_log("* ERROR: too many buffer views (%d)\n", index);
    return 1;
  }
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf.buffer_views[index];
  buffer_view->byte_offset = 0;
  buffer_view->byte_length = 0;
  buffer_view->target = 0;
  buffer_view->buffer = 0;
  
  return read_json_object(reader, read_buffer_view_prop, buffer_view);
  //int ret = read_json_object(reader, read_buffer_view_prop, buffer_view);
  //debug_log("got buffer view for buffer %d, offset=%-5d len=%-5d target=%d\n",
  //       buffer_view->buffer, buffer_view->byte_offset, buffer_view->byte_length, buffer_view->target);
  //return ret;
}

static int read_sampler_prop(struct GLTF_READER *reader, const char *name, void *data)
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

  debug_log("-> skipping 'sampler.%s'\n", name);
  return skip_json_value(reader);
}

static int read_samplers_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_SAMPLERS) {
    debug_log("* ERROR: too many samplers (%d)\n", index);
    return 1;
  }
  struct GLTF_SAMPLER *sampler = &reader->gltf.samplers[index];
  sampler->mag_filter = GLTF_SAMPLER_FILTER_LINEAR;
  sampler->min_filter = GLTF_SAMPLER_FILTER_LINEAR;
  sampler->wrap_s = GLTF_SAMPLER_WRAP_REPEAT;
  sampler->wrap_t = GLTF_SAMPLER_WRAP_REPEAT;
  
  return read_json_object(reader, read_sampler_prop, sampler);
}

static int read_image_prop(struct GLTF_READER *reader, const char *name, void *data)
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
    debug_log("* ERROR: invalid image mime type: '%s'\n", mime_type);
    return 1;
  }
  
  debug_log("-> skipping 'image.%s'\n", name);
  return skip_json_value(reader);
}

static int read_images_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_IMAGES) {
    debug_log("* ERROR: too many images (%d)\n", index);
    return 1;
  }
  struct GLTF_IMAGE *image = &reader->gltf.images[index];
  image->buffer_view = 0;
  image->mime_type = 0;
  
  return read_json_object(reader, read_image_prop, image);
}

static int read_scene_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_SCENE *scene = data;
  
  if (strcmp(name, "nodes") == 0) {
    size_t n_nodes;
    if (read_u16_vector(reader, scene->nodes, GLTF_MAX_SCENE_NODES, &n_nodes) != 0)
      return 1;
    scene->n_nodes = n_nodes;
    //debug_log("-> got %u nodes in scene\n", scene->n_nodes);
    return 0;
  }

  debug_log("-> skipping 'scene.%s'\n", name);
  return skip_json_value(reader);
}

static int read_scenes_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_SCENES) {
    debug_log("* ERROR: too many scenes (%d)\n", index);
    return 1;
  }
  struct GLTF_SCENE *scene = &reader->gltf.scenes[index];
  scene->n_nodes = 0;
  
  return read_json_object(reader, read_scene_prop, scene);
}

static int read_texture_prop(struct GLTF_READER *reader, const char *name, void *data)
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

  debug_log("-> skipping 'texture.%s'\n", name);
  return skip_json_value(reader);
}

static int read_textures_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_TEXTURES) {
    debug_log("* ERROR: too many textures (%d)\n", index);
    return 1;
  }
  struct GLTF_TEXTURE *texture = &reader->gltf.textures[index];
  texture->sampler = 0;
  texture->source_image = 0;

  return read_json_object(reader, read_texture_prop, texture);
}

static int read_material_texture_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_MATERIAL_TEXTURE *mat_tex = data;

  if (strcmp(name, "index") == 0)
    return read_json_u16(reader, &mat_tex->index);

  if (strcmp(name, "texCoord") == 0)
    return read_json_u16(reader, &mat_tex->tex_coord);
  
  debug_log("-> skipping material texture '%s'\n", name);
  return skip_json_value(reader);
}

static int read_material_pbr_prop(struct GLTF_READER *reader, const char *name, void *data)
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
  
  debug_log("-> skipping 'material.pbrMetallicRoughness.%s'\n", name);
  return skip_json_value(reader);
}

static int read_material_prop(struct GLTF_READER *reader, const char *name, void *data)
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

  if (strcmp(name, "doubleSided") == 0)
    return read_json_boolean(reader, &material->double_sided);

  if (strcmp(name, "normalTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->normal_tex);

  if (strcmp(name, "occlusionTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->occlusion_tex);

  if (strcmp(name, "emissiveTexture") == 0)
    return read_json_object(reader, read_material_texture_prop, &material->emissive_tex);
  
  if (strcmp(name, "pbrMetallicRoughness") == 0)
    return read_json_object(reader, read_material_pbr_prop, material);
  
  debug_log("-> skipping 'material.%s'\n", name);
  return skip_json_value(reader);
}

static void init_material_texture(struct GLTF_MATERIAL_TEXTURE *mat_tex)
{
  mat_tex->index = GLTF_NONE;
  mat_tex->tex_coord = 0;
}

static int read_materials_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_MATERIALS) {
    debug_log("* ERROR: too many materials (%d)\n", index);
    return 1;
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

static int read_accessor_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_ACCESSOR *accessor = data;

  if (strcmp(name, "bufferView") == 0)
    return read_json_u16(reader, &accessor->buffer_view);

  if (strcmp(name, "byteOffset") == 0)
    return read_json_u32(reader, &accessor->byte_offset);

  if (strcmp(name, "componentType") == 0)
    return read_json_u16(reader, &accessor->component_type);

  if (strcmp(name, "normalized") == 0)
    return read_json_boolean(reader, &accessor->normalized);

  if (strcmp(name, "count") == 0)
    return read_json_u32(reader, &accessor->count);

  if (strcmp(name, "type") == 0) {
    char type[32];
    if (read_json_string(reader, type, sizeof(type)) != 0)
      return 1;
    if      (strcmp(type, "SCALAR") == 0) accessor->type = GLTF_ACCESSOR_TYPE_SCALAR;
    else if (strcmp(type, "VEC2"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_VEC2;
    else if (strcmp(type, "VEC3"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_VEC3;
    else if (strcmp(type, "VEC4"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_VEC4;
    else if (strcmp(type, "MAT2"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_MAT2;
    else if (strcmp(type, "MAT3"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_MAT3;
    else if (strcmp(type, "MAT4"  ) == 0) accessor->type = GLTF_ACCESSOR_TYPE_MAT4;
    else {
      debug_log("* invalid accessor type: '%s'\n", type);
      return 1;
    }
    return 0;
  }

  if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0)
    return skip_json_value(reader);

  debug_log("-> skipping 'accessor.%s'\n", name);
  return skip_json_value(reader);
}

static int read_accessors_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_ACCESSORS) {
    debug_log("* ERROR: too many accessors (%d)\n", index);
    return 1;
  }
  struct GLTF_ACCESSOR *accessor = &reader->gltf.accessors[index];
  accessor->buffer_view = GLTF_NONE;
  accessor->byte_offset = 0;
  accessor->normalized = 0;
  accessor->count = 0;
  accessor->component_type = GLTF_ACCESSOR_COMP_TYPE_UBYTE;
  accessor->type = GLTF_ACCESSOR_TYPE_SCALAR;
  
  return read_json_object(reader, read_accessor_prop, accessor);
}

static int read_mesh_prim_attr_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_MESH_PRIMITIVE *prim = data;

  uint16_t attrib_num;
  if      (strcmp(name, "POSITION") == 0)   attrib_num = GLTF_MESH_ATTRIB_POSITION;
  else if (strcmp(name, "NORMAL") == 0)     attrib_num = GLTF_MESH_ATTRIB_NORMAL;
  else if (strcmp(name, "TANGENT") == 0)    attrib_num = GLTF_MESH_ATTRIB_TANGENT;
  else if (strcmp(name, "TEXCOORD_0") == 0) attrib_num = GLTF_MESH_ATTRIB_TEXCOORD_0;
  else if (strcmp(name, "TEXCOORD_1") == 0) attrib_num = GLTF_MESH_ATTRIB_TEXCOORD_1;
  else if (strcmp(name, "TEXCOORD_2") == 0) attrib_num = GLTF_MESH_ATTRIB_TEXCOORD_2;
  else if (strcmp(name, "TEXCOORD_3") == 0) attrib_num = GLTF_MESH_ATTRIB_TEXCOORD_3;
  else if (strcmp(name, "TEXCOORD_4") == 0) attrib_num = GLTF_MESH_ATTRIB_TEXCOORD_4;
  else {
    debug_log("* WARNING: ignoring unknown attribute '%s'\n", name);
    return skip_json_value(reader);
  }
  prim->attribs_present |= 1<<attrib_num;
  return read_json_u16(reader, &prim->attribs[attrib_num]);
}

static int read_mesh_primitive_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_MESH_PRIMITIVE *prim = data;

  if (strcmp(name, "indices") == 0)
    return read_json_u16(reader, &prim->indices_accessor);

  if (strcmp(name, "mode") == 0)
    return read_json_u16(reader, &prim->mode);

  if (strcmp(name, "material") == 0)
    return read_json_u16(reader, &prim->material);
  
  if (strcmp(name, "attributes") == 0)
    return read_json_object(reader, read_mesh_prim_attr_prop, prim);

  debug_log("-> skipping 'mesh.primitive.%s'\n", name);
  return skip_json_value(reader);
}

static int read_mesh_primitives_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_MESH_PRIMITIVES) {
    debug_log("* ERROR: too many mesh primitives (%d)\n", index);
    return 1;
  }
  
  struct GLTF_MESH *mesh = data;
  mesh->n_primitives = index+1;
  
  struct GLTF_MESH_PRIMITIVE *prim = &mesh->primitives[index];
  prim->mode = GLTF_MESH_MODE_TRIANGLES;
  prim->indices_accessor = GLTF_NONE;
  prim->material = GLTF_NONE;
  prim->attribs_present = 0;

  return read_json_object(reader, read_mesh_primitive_prop, prim);
}

static int read_mesh_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  struct GLTF_MESH *mesh = data;

  if (strcmp(name, "primitives") == 0)
    return read_json_array(reader, read_mesh_primitives_element, mesh);
  
  debug_log("-> skipping 'mesh.%s'\n", name);
  return skip_json_value(reader);
}

static int read_meshes_element(struct GLTF_READER *reader, int index, void *data)
{
  if (index >= GLTF_MAX_MESHES) {
    debug_log("* ERROR: too many meshes (%d)\n", index);
    return 1;
  }
  struct GLTF_MESH *mesh = &reader->gltf.meshes[index];

  return read_json_object(reader, read_mesh_prop, mesh);
}

static int read_main_prop(struct GLTF_READER *reader, const char *name, void *data)
{
  if (strcmp(name, "asset") == 0)
    return read_asset_object(reader);
  
  if (strcmp(name, "accessors") == 0)
    return read_json_array(reader, read_accessors_element, NULL);
  
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
  
  if (strcmp(name, "meshes") == 0)
    return read_json_array(reader, read_meshes_element, NULL);

  debug_log("-> skipping '%s'\n", name);
  return skip_json_value(reader);
}

static struct GLTF_READER *new_json_reader(size_t json_len)
{
  struct GLTF_READER *reader = malloc(sizeof *reader + json_len);
  if (! reader)
    return NULL;
  reader->json = reader->json_text;
  return reader;
}

void free_gltf_reader(struct GLTF_READER *reader)
{
  free(reader);
}

static struct GLTF_READER *parse_gltf_json(FILE *f, int json_len)
{
  struct GLTF_READER *reader = new_json_reader(json_len+1);
  if (fread(reader->json, 1, json_len, f) != json_len)
    goto err;
  reader->json[json_len] = '\0';
  
  reader->gltf.scene = GLTF_NONE;
  
  if (read_json_object(reader, read_main_prop, NULL) != 0)
    goto err;
  
  skip_json_spaces(reader);
  if (*reader->json != '\0')
    goto err;
  return reader;

 err:
  free_gltf_reader(reader);
  return NULL;
}

struct GLTF_READER *read_gltf(const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (! f)
    return NULL;
  if (fseek(f, 0, SEEK_END) < 0)
    goto err;
  long file_size = ftell(f);
  if (file_size < 0 || file_size > INT_MAX)
    goto err;
  if (fseek(f, 0, SEEK_SET) < 0)
    goto err;

  struct GLTF_READER *reader = parse_gltf_json(f, file_size);
  if (! reader)
    goto err;
  fclose(f);
  return reader;

 err:
  fclose(f);
  return NULL;
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

int read_glb(struct GLB_READER *glb, const char *filename)
{
  struct GLTF_READER *reader = NULL;

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
      reader = parse_gltf_json(f, chunk_len);
      if (! reader)
        goto err;
      continue;
    }

    // BIN\0 chunk
    if (memcmp(chunk_header + 4, "BIN", 4) == 0) {
      if (! reader)
        goto err;
      long data_offset = ftell(f);
      if (data_offset < 0)
        goto err;
      
      glb->gltf_reader = reader;
      glb->file = f;
      glb->data_off = data_offset;
      glb->data_len = chunk_len;
      return 0;
    }

    // unknown chunk before BIN\0
    goto err;
  }
 
 err:
  if (f)
    fclose(f);
  if (reader)
    free_gltf_reader(reader);
  return 1;
}

