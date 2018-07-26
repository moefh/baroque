/* model.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <stb_image.h>

#include "model.h"
#include "gltf.h"
#include "matrix.h"

#define DEBUG_MODEL_READER
#ifdef DEBUG_MODEL_READER
#define debug_log printf
#else
#define debug_log(...)
#endif

struct MODEL_READER {
  struct GLTF_DATA *gltf;
  FILE *file;
  uint32_t data_off;
  uint32_t data_len;
  uint32_t read_flags;
  uint16_t converted_textures[GLTF_MAX_TEXTURES];  // converted_textures[gltf_texture_index] = model_texture_index
};

// === STATIC MODEL ==================================

static const struct MODEL_MESH_VTX_TYPE {
  int vtx_type;
  int n_attribs;
  struct MODEL_MESH_VTX_TYPE_ATTRIB {
    uint16_t attrib_num;
    uint16_t accessor_type;
    uint16_t accessor_component_type;
  } attribs[16];
} supported_vtx_types[] = {
#define ATTR_DEF(attr, type, comp_type) { GLTF_MESH_ATTRIB_ ## attr, GLTF_ACCESSOR_TYPE_ ## type, GLTF_ACCESSOR_COMP_TYPE_ ## comp_type }
  
  { MODEL_MESH_VTX_POS,
    1, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL,
    2, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV1,
    2, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV2,
    3, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV1,
    3, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV2,
    4, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
    }
  },

  { MODEL_MESH_VTX_POS_SKEL1,
    3, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_SKEL1,
    4, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV1_SKEL1,
    4, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV2_SKEL1,
    5, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL1,
    5, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV2_SKEL1,
    6, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
    }
  },
  
  { MODEL_MESH_VTX_POS_SKEL2,
    3, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_SKEL2,
    4, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV1_SKEL2,
    4, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_UV2_SKEL2,
    5, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL2,
    5, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  { MODEL_MESH_VTX_POS_NORMAL_UV2_SKEL2,
    6, {
      ATTR_DEF(POSITION, VEC3, FLOAT),
      ATTR_DEF(NORMAL, VEC3, FLOAT),
      ATTR_DEF(TEXCOORD_0, VEC2, FLOAT),
      ATTR_DEF(TEXCOORD_1, VEC2, FLOAT),
      ATTR_DEF(JOINTS_0, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_0, VEC4, FLOAT),
      ATTR_DEF(JOINTS_1, VEC4, USHORT),
      ATTR_DEF(WEIGHTS_1, VEC4, FLOAT),
    }
  },
  
#undef ATTR_DEF  
};

static int set_file_pos(struct MODEL_READER *reader, uint32_t data_off)
{
  if (fseek(reader->file, reader->data_off + data_off, SEEK_SET) != 0) {
    debug_log("* FILE ERROR: can't seek to position %u\n", reader->data_off + data_off);
    return 1;
  }
  return 0;
}

static int read_file_data(struct MODEL_READER *reader, void *data, size_t size)
{
  if (fread(data, 1, size, reader->file) != size) {
    debug_log("* FILE ERROR: can't read %u bytes\n", (unsigned) size);
    return 1;
  }
  return 0;
}

static int skip_file_data(struct MODEL_READER *reader, size_t size)
{
  if (fseek(reader->file, size, SEEK_CUR) != 0) {
    debug_log("* FILE ERROR: can't seek to skip %u bytes\n", (unsigned) size);
    return 1;
  }
  return 0;
}

static uint16_t get_vtx_type_attrib_size(const struct MODEL_MESH_VTX_TYPE_ATTRIB *vtx_type_attrib)
{
  uint16_t component_size;
  switch (vtx_type_attrib->accessor_component_type) {
  case GLTF_ACCESSOR_COMP_TYPE_BYTE:   component_size = 1; break;
  case GLTF_ACCESSOR_COMP_TYPE_UBYTE:  component_size = 1; break;
  case GLTF_ACCESSOR_COMP_TYPE_SHORT:  component_size = 2; break;
  case GLTF_ACCESSOR_COMP_TYPE_USHORT: component_size = 2; break;
  case GLTF_ACCESSOR_COMP_TYPE_UINT:   component_size = 4; break;
  case GLTF_ACCESSOR_COMP_TYPE_FLOAT:  component_size = 4; break;
  default: return 0;
  }

  switch (vtx_type_attrib->accessor_type) {
  case GLTF_ACCESSOR_TYPE_SCALAR: return component_size; break;
  case GLTF_ACCESSOR_TYPE_VEC2: return 2 * component_size; break;
  case GLTF_ACCESSOR_TYPE_VEC3: return 3 * component_size; break;
  case GLTF_ACCESSOR_TYPE_VEC4: return 4 * component_size; break;
  case GLTF_ACCESSOR_TYPE_MAT2: return 4 * component_size; break;
  case GLTF_ACCESSOR_TYPE_MAT3: return 9 * component_size; break;
  case GLTF_ACCESSOR_TYPE_MAT4: return 16 * component_size; break;
  default: return 0;
  }
}

static int get_vtx_type_size(const struct MODEL_MESH_VTX_TYPE *vtx_type)
{
  uint16_t size = 0;
  for (int i = 0; i < vtx_type->n_attribs; i++) {
    uint16_t attrib_size = get_vtx_type_attrib_size(&vtx_type->attribs[i]);
    if (attrib_size == 0)
      return 0;
    size += attrib_size;
  }
  return size;
}

static int convert_gltf_texture(struct MODEL_READER *reader, int gltf_texture, uint16_t *p_model_texture_index, struct MODEL *model)
{
  // check if texture is already converted
  if (reader->converted_textures[gltf_texture] != MODEL_TEXTURE_NONE) {
    debug_log("  -> texture %d was already converted as model texture %d\n", gltf_texture, reader->converted_textures[gltf_texture]);
    *p_model_texture_index = reader->converted_textures[gltf_texture];
    return 0;
  }

  if (model->n_textures >= MODEL_MAX_TEXTURES) {
    debug_log("* ERROR: too many textures (%d)\n", model->n_textures);
    return 1;
  }
  
  struct GLTF_TEXTURE *texture = &reader->gltf->textures[gltf_texture];
  //struct GLTF_SAMPLER *sampler = &reader->gltf->samplers[texture->sampler];
  struct GLTF_IMAGE *image = &reader->gltf->images[texture->source_image];
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf->buffer_views[image->buffer_view];

  debug_log("  -> loading image from data offset %d, size %d\n", buffer_view->byte_offset, buffer_view->byte_length);
  
  uint16_t model_texture_index = model->n_textures;
  struct MODEL_TEXTURE *model_tex = &model->textures[model_texture_index];

  if (reader->read_flags & MODEL_FLAGS_IMAGE_REFS) {
    size_t image_name_len = strlen(image->name) + 1;
    if (image_name_len > 255)
      return 1;
    model_tex->data = malloc(image_name_len);
    if (! model_tex->data)
      return 1;
    memcpy(model_tex->data, image->name, image_name_len);
    model_tex->width = reader->data_off + buffer_view->byte_offset;
    model_tex->height = buffer_view->byte_length;
    model_tex->n_chan = 0;
    debug_log("  -> added texture %d: '%s'\n", model_texture_index, model_tex->data);
  } else if (reader->read_flags & MODEL_FLAGS_PACKED_IMAGES) {
    if (set_file_pos(reader, buffer_view->byte_offset) != 0)
      return 1;

    model_tex->data = malloc(buffer_view->byte_length);
    if (! model_tex->data)
      return 1;
    if (read_file_data(reader, model_tex->data, buffer_view->byte_length) != 0)
      return 1;
    model_tex->width = buffer_view->byte_length;
    model_tex->height = buffer_view->byte_length;
    model_tex->n_chan = 0;
    debug_log("  -> added texture %d: %u bytes (original format)\n", model_texture_index, buffer_view->byte_length);
  } else {
    if (set_file_pos(reader, buffer_view->byte_offset) != 0)
      return 1;
    
    int width, height, n_chan;
    model_tex->data = stbi_load_from_file(reader->file, &width, &height, &n_chan, 0);
    if (! model_tex->data)
      return 1;
    model_tex->width = width;
    model_tex->height = width;
    model_tex->n_chan = n_chan;
    debug_log("  -> added texture %d: %dx%d %d channels\n", model_texture_index, width, height, n_chan);
  }

  model->n_textures++;
  reader->converted_textures[gltf_texture] = model_texture_index;
  *p_model_texture_index = model_texture_index;
  return 0;
}

static int convert_gltf_material(struct MODEL_READER *reader, struct MODEL_MESH *model_mesh, struct GLTF_MATERIAL *material, struct MODEL *model)
{
  if (material->color_tex.index != GLTF_NONE)
    if (convert_gltf_texture(reader, material->color_tex.index, &model_mesh->tex0_index, model) != 0)
      return 1;

  if (material->normal_tex.index != GLTF_NONE)
    if (convert_gltf_texture(reader, material->normal_tex.index, &model_mesh->tex1_index, model) != 0)
      return 1;
  
  return 0;
}

struct MODEL_MESH *new_model_mesh(uint8_t vtx_type, uint32_t vtx_size, uint8_t ind_type, uint32_t ind_size, uint32_t ind_count)
{
  // align to 8 bytes  
  size_t v_size = ((size_t)vtx_size + 7) / 8 * 8;
  size_t i_size = ((size_t)ind_size + 7) / 8 * 8;
  
  struct MODEL_MESH *mesh = malloc(sizeof *mesh + v_size + i_size);
  if (! mesh)
    return NULL;
  mesh->vtx_type = vtx_type;
  mesh->vtx_size = vtx_size;
  mesh->ind_type = ind_type;
  mesh->ind_size = ind_size;
  mesh->ind_count = ind_count;
  mesh->tex0_index = MODEL_TEXTURE_NONE;
  mesh->tex1_index = MODEL_TEXTURE_NONE;
  mesh->vtx = mesh->data;
  mesh->ind = (char *)mesh->data + v_size;
  return mesh;
}

static const struct MODEL_MESH_VTX_TYPE *convert_gltf_vtx_type(struct GLTF_DATA *gltf, struct GLTF_MESH_PRIMITIVE *prim, uint32_t *ret_buffer_size)
{
  const struct MODEL_MESH_VTX_TYPE *best_vtx_type = NULL;
  int best_num_missing = 0;
  uint32_t best_buffer_size = 0;
  
  for (int vtx_type_index = 0; vtx_type_index < (int) (sizeof(supported_vtx_types)/sizeof(supported_vtx_types[0])); vtx_type_index++) {
    const struct MODEL_MESH_VTX_TYPE *vtx_type = &supported_vtx_types[vtx_type_index];

    // check if accessor types are supported
    int supported = 1;
    for (int i = 0; i < vtx_type->n_attribs; i++) {
      if (! (prim->attribs_present & (1<<vtx_type->attribs[i].attrib_num)))
        continue;
      struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[vtx_type->attribs[i].attrib_num]];
      if (accessor->type != vtx_type->attribs[i].accessor_type && accessor->component_type != vtx_type->attribs[i].accessor_component_type) {
        printf("**** bad type [%d][%d]: have (%d,%d), want (%d,%d)\n",
               vtx_type_index, vtx_type->attribs[i].attrib_num,
               accessor->type, accessor->component_type,
               vtx_type->attribs[i].accessor_type, vtx_type->attribs[i].accessor_component_type);
        supported = 0;
        break;
      }
    }
    if (! supported)
      continue;

    // check if primitive has all attributes
    for (int i = 0; i < vtx_type->n_attribs; i++) {
      if (! (prim->attribs_present & (1<<vtx_type->attribs[i].attrib_num))) {
        supported = 0;
        break;
      }
    }
    if (! supported)
      continue;

    // count all attribs that are not supported and get buffer size
    int num_missing = 0;
    uint32_t buffer_size = 0;
    for (int attrib_num = 0; attrib_num < GLTF_MESH_NUM_ATTRIBS; attrib_num++) {
      if (! (prim->attribs_present & (1<<attrib_num)))
        continue;
      int found = 0;
      for (int i = 0; i < vtx_type->n_attribs; i++) {
        if (vtx_type->attribs[i].attrib_num == attrib_num) {
          struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[attrib_num]];
          buffer_size += accessor->count * get_vtx_type_attrib_size(&vtx_type->attribs[i]);
          found = 1;
          break;
        }
      }
      if (! found)
        num_missing++;
    }

    if (! best_vtx_type || best_num_missing > num_missing) {
      best_vtx_type = vtx_type;
      best_num_missing = num_missing;
      best_buffer_size = buffer_size;
    }
  }

  if (ret_buffer_size)
    *ret_buffer_size = best_buffer_size;
  return best_vtx_type;
}

static int extract_vtx_buffer_data(struct MODEL_MESH *mesh, struct MODEL_READER *reader, struct GLTF_MESH_PRIMITIVE *prim, const struct MODEL_MESH_VTX_TYPE *vtx_type)
{
  struct GLTF_DATA *gltf = reader->gltf;

  uint16_t vtx_stride = get_vtx_type_size(vtx_type);
  
  int attr_off = 0;
  for (int i = 0; i < vtx_type->n_attribs; i++) {
    uint16_t attrib_num = vtx_type->attribs[i].attrib_num;
    struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[attrib_num]];
    struct GLTF_BUFFER_VIEW *buffer_view = &gltf->buffer_views[accessor->buffer_view];
    
    uint32_t buffer_byte_offset = buffer_view->byte_offset + accessor->byte_offset;
    if (set_file_pos(reader, buffer_byte_offset) != 0)
      return 1;
    
    int attr_size = get_vtx_type_attrib_size(&vtx_type->attribs[i]);
    
    debug_log("  -> reading %d vtx elements from buffer offset %-6d stride %-2d -- attribute %2d, size %2d, stride %2d, offset %d\n",
              accessor->count, buffer_byte_offset, buffer_view->byte_stride, attrib_num, attr_size, vtx_stride, attr_off);

    char *vtx = (char *)mesh->vtx + attr_off;
    for (uint32_t i = 0; i < accessor->count; i++) {
      if (read_file_data(reader, vtx, attr_size) != 0)
        return 1;
      vtx += vtx_stride;
      if (buffer_view->byte_stride != 0 && buffer_view->byte_stride != (unsigned) attr_size) {
        if (buffer_view->byte_stride < (unsigned) attr_size) {
          debug_log("** ERROR: invalid byte stride: %d (attribute size is %d)\n", buffer_view->byte_stride, attr_size);
          return 1;
        }
        if (skip_file_data(reader, buffer_view->byte_stride - (unsigned) attr_size) != 0)
          return 1;
      }
    }
    attr_off += attr_size;
  }
  return 0;
}

static uint8_t convert_gltf_ind_type(uint16_t accessor_comp_type)
{
  switch (accessor_comp_type) {
  case GLTF_ACCESSOR_COMP_TYPE_BYTE:
  case GLTF_ACCESSOR_COMP_TYPE_UBYTE:
    return MODEL_MESH_IND_U8;

  case GLTF_ACCESSOR_COMP_TYPE_SHORT:
  case GLTF_ACCESSOR_COMP_TYPE_USHORT:
    return MODEL_MESH_IND_U16;

  case GLTF_ACCESSOR_COMP_TYPE_UINT:
    return MODEL_MESH_IND_U32;

  default:
    return 0xff;
  }
}

static uint8_t convert_gltf_ind_size(uint16_t accessor_comp_type)
{
  switch (accessor_comp_type) {
  case GLTF_ACCESSOR_COMP_TYPE_BYTE:
  case GLTF_ACCESSOR_COMP_TYPE_UBYTE:
    return 1;

  case GLTF_ACCESSOR_COMP_TYPE_SHORT:
  case GLTF_ACCESSOR_COMP_TYPE_USHORT:
    return 2;

  case GLTF_ACCESSOR_COMP_TYPE_UINT:
    return 4;

  default:
    return 0xff;
  }
}

static int extract_ind_buffer_data(struct MODEL_MESH *mesh, struct MODEL_READER *reader, struct GLTF_ACCESSOR *accessor, uint32_t byte_length)
{
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf->buffer_views[accessor->buffer_view];
  uint32_t buffer_byte_offset = buffer_view->byte_offset + accessor->byte_offset;

  debug_log("  -> reading %d index bytes from buffer offset %-5d\n", byte_length, buffer_byte_offset);
  
  if (set_file_pos(reader, buffer_byte_offset) != 0)
    return 1;
  if (read_file_data(reader, mesh->ind, byte_length) != 0)
    return 1;
  return 0;
}

static int convert_gltf_mesh_primitive(struct MODEL_READER *reader, struct GLTF_NODE *node, struct GLTF_MESH_PRIMITIVE *prim, struct MODEL *model)
{
  if (model->n_meshes >= MODEL_MAX_MESHES) {
    debug_log("* WARNING: ignoring primitive: too many meshes converted\n");
    return 0;
  }
  if (prim->mode != GLTF_MESH_MODE_TRIANGLES) {
    debug_log("* WARNING: ignoring primitive with non-triangles mode (not implemented)\n");
    return 0;
  }
  if (prim->indices_accessor == GLTF_NONE) {
    debug_log("* WARNING: ignoring primitive with no indices accessor\n");
    return 0;
  }

  // read vtx data info
  uint32_t vtx_buffer_size;
  const struct MODEL_MESH_VTX_TYPE *vtx_type = convert_gltf_vtx_type(reader->gltf, prim, &vtx_buffer_size);
  if (! vtx_type) {
    debug_log("* WARNING: ignoring primitive with unsupported vertex attributes (%x)\n", prim->attribs_present);
    return 0;
  }

  // read index data info
  struct GLTF_ACCESSOR *indices_accessor = &reader->gltf->accessors[prim->indices_accessor];
  uint8_t mesh_ind_type = convert_gltf_ind_type(indices_accessor->component_type);
  if (mesh_ind_type == 0xff) {
    debug_log("* WARNING: ignoring primitive with unsupported index component type %u\n", indices_accessor->component_type);
    return 0;
  }
  uint32_t ind_buffer_size = indices_accessor->count * convert_gltf_ind_size(indices_accessor->component_type);
  
  debug_log("-> adding mesh with vtx_size=%u, ind_size=%u\n", vtx_buffer_size, ind_buffer_size);
  struct MODEL_MESH *model_mesh = new_model_mesh(vtx_type->vtx_type, vtx_buffer_size, mesh_ind_type, ind_buffer_size, indices_accessor->count);
  mat4_copy(model_mesh->matrix, node->matrix);

  // extract mesh data
  if (extract_vtx_buffer_data(model_mesh, reader, prim, vtx_type) != 0)
    return 1;
  if (extract_ind_buffer_data(model_mesh, reader, indices_accessor, ind_buffer_size) != 0)
    return 1;
  
  // convert material
  if (prim->material != GLTF_NONE) {
    if (convert_gltf_material(reader, model_mesh, &reader->gltf->materials[prim->material], model) != 0)
      return 1;
  }
  
  model->meshes[model->n_meshes++] = model_mesh;
  return 0;
}

static int read_gltf_model_meshes_from_node(struct MODEL_READER *reader, int node_index, char *nodes_done, struct MODEL *model)
{
  struct GLTF_NODE *node = &reader->gltf->nodes[node_index];
  if (nodes_done[node_index])
    return 0;
  nodes_done[node_index] = 1;
  
  if (node->mesh != GLTF_NONE) {
    debug_log("-> converting node %d, mesh %d\n", node_index, node->mesh);
    struct GLTF_MESH *mesh = &reader->gltf->meshes[node->mesh];
    for (uint16_t i = 0; i < mesh->n_primitives; i++) {
      if (convert_gltf_mesh_primitive(reader, node, &mesh->primitives[i], model) != 0)
        return 1;
    }
  }

  for (uint16_t i = 0; i < node->n_children; i++) {
    if (read_gltf_model_meshes_from_node(reader, node->children[i], nodes_done, model) != 0)
      return 1;
  }
  
  return 0;
}

// === SKELETON ======================================

static int find_model_skin_rec(struct GLTF_DATA *gltf, uint16_t *node_indices, int num_nodes, uint16_t *ret_skin_index)
{
  for (int i = 0; i < num_nodes; i++) {
    struct GLTF_NODE *node = &gltf->nodes[node_indices[i]];
    if (node->skin != GLTF_NONE) {
      if (*ret_skin_index != GLTF_NONE && *ret_skin_index != node->skin) {
        debug_log("** ERROR: unsupported format: multiple skins\n");
        return 1;
      }
      *ret_skin_index = node->skin;
    }
    
    if (find_model_skin_rec(gltf, node->children, node->n_children, ret_skin_index) != 0)
      return 1;
  }
  return 0;
}

struct GLTF_SKIN *find_model_skin(struct GLTF_DATA *gltf)
{
  struct GLTF_SCENE *scene = &gltf->scenes[gltf->scene];
  uint16_t skin_index = GLTF_NONE;
  if (find_model_skin_rec(gltf, scene->nodes, scene->n_nodes, &skin_index) != 0)
    return NULL;
  if (skin_index == GLTF_NONE) {
    debug_log("** ERROR: skin not found\n");
    return NULL;
  }
  return &gltf->skins[skin_index];
}

static int read_skeleton(struct GLTF_DATA *gltf, struct MODEL_SKELETON *skel, struct GLTF_SKIN *skin)
{
  skel->n_bones = skin->n_joints;
  uint16_t node_bone_index[GLTF_MAX_NODES];
  for (uint16_t i = 0; i < GLTF_MAX_NODES; i++)
    node_bone_index[i] = GLTF_NONE;
  for (uint16_t i = 0; i < skin->n_joints; i++)
    node_bone_index[skin->joints[i]] = i;
  for (uint16_t i = 0; i < skin->n_joints; i++) {
    uint16_t joint_parent = gltf->nodes[skin->joints[i]].parent;
    skel->bones[i].parent = node_bone_index[joint_parent];
  }

  size_t data_size = skel->n_bones * 2 * sizeof(float) * 16;
  skel->float_data = malloc(data_size);
  if (! skel->float_data) {
    debug_log("** ERROR: out of memory for skeleton: can't allocate %u bytes\n", (unsigned) data_size);
    return 1;
  }
  for (int i = 0; i < skel->n_bones; i++) {
    skel->bones[i].inv_matrix = skel->float_data + i * 2*16;
    skel->bones[i].pose_matrix = skel->float_data + i * 2*16 + 16;
  }
  return 0;
}

static int read_bone_pose_matrices(struct GLTF_DATA *gltf, struct MODEL_SKELETON *skel, struct GLTF_SKIN *skin)
{
  uint16_t skin_root = skin->skeleton;
  if (skin_root == GLTF_NONE)
    skin_root = gltf->scenes[gltf->scene].nodes[0];
  for (int bone_index = 0; bone_index < skel->n_bones; bone_index++) {
    uint16_t node_index = skin->joints[bone_index];
    mat4_id(skel->bones[bone_index].pose_matrix);
    while (node_index != GLTF_NONE && node_index < GLTF_MAX_NODES) {
      struct GLTF_NODE *node = &gltf->nodes[node_index];
      //printf("bone %d transformed by node %d:\n", i, node_index); mat4_dump(node->local_matrix);
      mat4_mul_left(skel->bones[bone_index].pose_matrix, node->local_matrix);
      if (node_index == skin_root)
        break;
      node_index = gltf->nodes[node_index].parent;
    }
  }
  
  return 0;
}

static int read_bone_inverse_matrices(struct MODEL_READER *reader, struct MODEL_SKELETON *skel, struct GLTF_SKIN *skin)
{
  struct GLTF_DATA *gltf = reader->gltf;

  if (skin->inverse_bind_matrices == GLTF_NONE) {
    for (int i = 0; i < skel->n_bones; i++)
      mat4_id(skel->bones[i].inv_matrix);
  } else {
    struct GLTF_ACCESSOR *ibm_accessor = &gltf->accessors[skin->inverse_bind_matrices];
    struct GLTF_BUFFER_VIEW *ibm_buffer_view = &gltf->buffer_views[ibm_accessor->buffer_view];

    if (ibm_accessor->type != GLTF_ACCESSOR_TYPE_MAT4 || ibm_accessor->count != (unsigned) skel->n_bones) {
      debug_log("** ERROR: bad accessor: got (%d, %d), expected (%d, %d)\n",
                ibm_accessor->type, ibm_accessor->count,
                GLTF_ACCESSOR_TYPE_MAT4, skel->n_bones);
      return 1;
    }
    
    uint32_t byte_offset = ibm_buffer_view->byte_offset + ibm_accessor->byte_offset;
    if (set_file_pos(reader, byte_offset) != 0)
      return 1;
    for (int i = 0; i < skel->n_bones; i++) {
      float inv_matrix[16];
      if (read_file_data(reader, inv_matrix, 16*sizeof(float)) != 0)
        return 1;
      mat4_transpose(skel->bones[i].inv_matrix, inv_matrix);
    }
  }

  return 0;
}

static int read_model_skeleton(struct MODEL_READER *reader, struct MODEL_SKELETON *skel)
{
  struct GLTF_DATA *gltf = reader->gltf;
  if (gltf->scene == GLTF_NONE)
    return 1;

  struct GLTF_SKIN *skin = find_model_skin(gltf);
  if (! skin)
    return 1;
  
  if (read_skeleton(gltf, skel, skin) != 0)
    return 1;
  
  if (read_bone_pose_matrices(gltf, skel, skin) != 0)
    return 1;
  
  if (read_bone_inverse_matrices(reader, skel, skin) != 0)
    return 1;
  
  printf("bones:\n");
  for (int i = 0; i < skel->n_bones; i++) {
    struct MODEL_BONE *bone = &skel->bones[i];
    printf("-- bone [%d] --------------------\n", i);
    if (bone->parent == GLTF_NONE)
      printf("parent: --\n");
    else
      printf("parent: %d\n", bone->parent);
    printf("pose_matrix:\n"); mat4_dump(bone->pose_matrix);
    printf("inv_matrix:\n"); mat4_dump(bone->inv_matrix);

    float matrix[16];
    mat4_mul(matrix, bone->pose_matrix, bone->inv_matrix);
    printf("pose * inv:\n"); mat4_dump(matrix);
  }
  return 0;
}

void free_model(struct MODEL *model)
{
  for (int i = 0; i < model->n_meshes; i++)
    free(model->meshes[i]);

  for (int i = 0; i < model->n_textures; i++)
    free(model->textures[i].data);
}

void free_model_skeleton(struct MODEL_SKELETON *skel)
{
  free(skel->float_data);
}

static void init_model(struct MODEL *model)
{
  model->n_meshes = 0;
  model->n_textures = 0;
}

static void init_model_skeleton(struct MODEL_SKELETON *skel)
{
  skel->n_bones = 0;
  skel->float_data = NULL;
}

static void init_model_reader(struct MODEL_READER *reader, struct GLTF_DATA *gltf, uint32_t flags)
{
  reader->file = NULL;
  reader->data_off = 0;
  reader->data_len = 0;
  reader->gltf = gltf;
  reader->read_flags = flags;
  for (int i = 0; i < GLTF_MAX_TEXTURES; i++)
    reader->converted_textures[i] = MODEL_TEXTURE_NONE;
}

static void init_model_reader_from_glb(struct MODEL_READER *reader, struct GLB_FILE *glb, uint32_t flags)
{
  init_model_reader(reader, glb->gltf, flags);
  reader->file = glb->file;
  reader->data_off = glb->data_off;
  reader->data_len = glb->data_len;
}

static int read_full_static_model(struct MODEL_READER *reader, struct MODEL *model)
{
  struct GLTF_DATA *gltf = reader->gltf;
  if (gltf->scene == GLTF_NONE)
    return 1;
  
  struct GLTF_SCENE *scene = &gltf->scenes[gltf->scene];
  char nodes_done[GLTF_MAX_NODES] = { 0 };
  for (uint16_t i = 0; i < scene->n_nodes; i++) {
    int node_index = scene->nodes[i];
    if (read_gltf_model_meshes_from_node(reader, node_index, nodes_done, model) != 0)
      return 1;
  }
  return 0;
}

int read_glb_model(struct MODEL *model, const char *filename, uint32_t flags)
{
  struct GLB_FILE glb;
  if (open_glb(&glb, filename) != 0)
    return 1;
  struct MODEL_READER reader;
  init_model_reader_from_glb(&reader, &glb, flags);

  init_model(model);
  if (read_full_static_model(&reader, model) != 0)
    goto err;

  close_glb(&glb);
  return 0;

 err:
  free_model(model);
  close_glb(&glb);
  return 1;
}

int read_glb_animated_model(struct MODEL *model, struct MODEL_SKELETON *skel, const char *filename, uint32_t flags)
{
  struct GLB_FILE glb;
  if (open_glb(&glb, filename) != 0)
    return 1;
  struct MODEL_READER reader;
  init_model_reader_from_glb(&reader, &glb, flags);
  init_model(model);
  init_model_skeleton(skel);
  
  if (read_full_static_model(&reader, model) != 0)
    goto err;

  if (read_model_skeleton(&reader, skel) != 0)
    goto err;
  
  close_glb(&glb);
  return 0;

 err:
  free_model(model);
  free_model_skeleton(skel);
  close_glb(&glb);
  return 1;
}
