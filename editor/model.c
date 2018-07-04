/* model.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <stb_image.h>

#include "model.h"
#include "gltf.h"
#include "matrix.h"

//#define DEBUG_MODEL_READER
#ifdef DEBUG_MODEL_READER
#define debug_log printf
#else
#define debug_log(...)
#endif

struct MODEL_READER {
  struct MODEL *model;
  struct GLTF_DATA *gltf;
  FILE *file;
  uint32_t data_off;
  uint32_t data_len;
  uint32_t read_flags;
  uint16_t converted_textures[GLTF_MAX_TEXTURES];  // converted_textures[gltf_texture_index] = model_texture_index
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

static int convert_gltf_texture(struct MODEL_READER *reader, int gltf_texture, uint16_t *p_model_texture_index)
{
  // check if texture is already converted
  if (reader->converted_textures[gltf_texture] != MODEL_TEXTURE_NONE) {
    debug_log("  -> texture %d was already converted as model texture %d\n", gltf_texture, reader->converted_textures[gltf_texture]);
    *p_model_texture_index = reader->converted_textures[gltf_texture];
    return 0;
  }

  if (reader->model->n_textures >= MODEL_MAX_TEXTURES) {
    debug_log("* ERROR: too many textures (%d)\n", reader->model->n_textures);
    return 1;
  }
  
  struct GLTF_TEXTURE *texture = &reader->gltf->textures[gltf_texture];
  //struct GLTF_SAMPLER *sampler = &reader->gltf->samplers[texture->sampler];
  struct GLTF_IMAGE *image = &reader->gltf->images[texture->source_image];
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf->buffer_views[image->buffer_view];

  debug_log("  -> loading image from data offset %d, size %d\n", buffer_view->byte_offset, buffer_view->byte_length);
  
  uint16_t model_texture_index = reader->model->n_textures;
  struct MODEL_TEXTURE *model_tex = &reader->model->textures[model_texture_index];

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
  }

  debug_log("  -> added texture %d: %dx%d %d channels\n", model_texture_index, width, height, n_chan);
  
  reader->model->n_textures++;
  reader->converted_textures[gltf_texture] = model_texture_index;
  *p_model_texture_index = model_texture_index;
  return 0;
}

static int convert_gltf_material(struct MODEL_READER *reader, struct MODEL_MESH *model_mesh, struct GLTF_MATERIAL *material)
{
  if (material->color_tex.index != GLTF_NONE)
    if (convert_gltf_texture(reader, material->color_tex.index, &model_mesh->tex0_index) != 0)
      return 1;

  if (material->normal_tex.index != GLTF_NONE)
    if (convert_gltf_texture(reader, material->normal_tex.index, &model_mesh->tex1_index) != 0)
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

static uint8_t convert_gltf_vtx_type(uint16_t prim_attribs_present, uint16_t *use_vtx_attribs)
{
#define ATTRS_FOR_POS            ((1<<GLTF_MESH_ATTRIB_POSITION))
#define ATTRS_FOR_POS_UV1        ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0))
#define ATTRS_FOR_POS_UV2        ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_1))
#define ATTRS_FOR_POS_NORMAL     ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL))
#define ATTRS_FOR_POS_NORMAL_UV1 ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0))
#define ATTRS_FOR_POS_NORMAL_UV2 ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_1))

#define TEST_ATTRIBS(X) do {                                            \
    if ((prim_attribs_present & ATTRS_FOR_##X) == ATTRS_FOR_##X) {      \
      *use_vtx_attribs = ATTRS_FOR_##X;                                 \
      return MODEL_MESH_VTX_##X;                                        \
    }                                                                   \
  } while (0)
  
  // the order here matters (fuller bitmaps go first):
  TEST_ATTRIBS(POS_NORMAL_UV2);
  TEST_ATTRIBS(POS_NORMAL_UV1);
  TEST_ATTRIBS(POS_NORMAL);
  TEST_ATTRIBS(POS_UV2);
  TEST_ATTRIBS(POS_UV1);
  TEST_ATTRIBS(POS);

  return 0xff;
}

static int extract_vtx_buffer_size_from_attribs(struct GLTF_DATA *gltf,
                                                struct GLTF_MESH_PRIMITIVE *prim,
                                                uint16_t used_vtx_attribs,
                                                uint32_t *p_vtx_buffer_size)
{
  uint32_t vtx_buffer_size = 0;
  for (uint16_t attrib_num = 0; attrib_num < GLTF_MESH_NUM_ATTRIBS; attrib_num++) {
    if (used_vtx_attribs & (1<<attrib_num)) {
      struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[attrib_num]];
      struct GLTF_BUFFER_VIEW *buffer_view = &gltf->buffer_views[accessor->buffer_view];
      if (accessor->component_type != GLTF_ACCESSOR_COMP_TYPE_FLOAT) {
        debug_log("* WARNING: ignoring primitive with unsupported vertex attribute component type: %d\n", accessor->component_type);
        return 1;
      }
      vtx_buffer_size += buffer_view->byte_length;
    }
  }

  *p_vtx_buffer_size = vtx_buffer_size;
  return 0;
}

static int get_mesh_vtx_stride(struct MODEL_MESH *mesh)
{
  switch (mesh->vtx_type) {
  case MODEL_MESH_VTX_POS:            return sizeof(float) * (3);
  case MODEL_MESH_VTX_POS_UV1:        return sizeof(float) * (3+2);
  case MODEL_MESH_VTX_POS_UV2:        return sizeof(float) * (3+2+2);
  case MODEL_MESH_VTX_POS_NORMAL:     return sizeof(float) * (3+3);
  case MODEL_MESH_VTX_POS_NORMAL_UV1: return sizeof(float) * (3+3+2);
  case MODEL_MESH_VTX_POS_NORMAL_UV2: return sizeof(float) * (3+3+2+2);
  default: return 0;
  }
}

static int get_gltf_mesh_attrib_size(uint16_t attrib_num)
{
  switch (attrib_num) {
  case GLTF_MESH_ATTRIB_POSITION: return sizeof(float) * 3;
  case GLTF_MESH_ATTRIB_NORMAL:   return sizeof(float) * 3;
  case GLTF_MESH_ATTRIB_TANGENT:  return sizeof(float) * 3;
    
  case GLTF_MESH_ATTRIB_TEXCOORD_0:
  case GLTF_MESH_ATTRIB_TEXCOORD_1:
  case GLTF_MESH_ATTRIB_TEXCOORD_2:
  case GLTF_MESH_ATTRIB_TEXCOORD_3:
  case GLTF_MESH_ATTRIB_TEXCOORD_4:
    return sizeof(float) * 2;
    
  default:
    return 0;
  }
}

static int extract_vtx_buffer_data(struct MODEL_MESH *mesh, struct MODEL_READER *reader, struct GLTF_MESH_PRIMITIVE *prim, uint16_t used_vtx_attribs)
{
  struct GLTF_DATA *gltf = reader->gltf;

  int vtx_stride = get_mesh_vtx_stride(mesh);

  int attr_off = 0;
  for (uint16_t attrib_num = 0; attrib_num < GLTF_MESH_NUM_ATTRIBS; attrib_num++) {
    if (used_vtx_attribs & (1<<attrib_num)) {
      struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[attrib_num]];
      struct GLTF_BUFFER_VIEW *buffer_view = &gltf->buffer_views[accessor->buffer_view];

      if (set_file_pos(reader, buffer_view->byte_offset) != 0)
        return 1;

      int attr_size = get_gltf_mesh_attrib_size(attrib_num);

      debug_log("  -> reading %d vtx elements from file offset %-5d -- attribute %d, size %2d, stride %2d, offset %d\n",
             accessor->count, buffer_view->byte_offset, attrib_num, attr_size, vtx_stride, attr_off);

      char *vtx = (char *)mesh->vtx + attr_off;
      for (uint32_t i = 0; i < accessor->count; i++) {
        if (read_file_data(reader, vtx, attr_size) != 0)
          return 1;
        vtx += vtx_stride;
      }
      attr_off += attr_size;
    }
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

static int extract_ind_buffer_data(struct MODEL_MESH *mesh, struct MODEL_READER *reader, struct GLTF_ACCESSOR *accessor)
{
  struct GLTF_BUFFER_VIEW *buffer_view = &reader->gltf->buffer_views[accessor->buffer_view];

  debug_log("  -> reading %d index bytes from file offset %-5d\n", buffer_view->byte_length, buffer_view->byte_offset);
  
  if (set_file_pos(reader, buffer_view->byte_offset) != 0)
    return 1;
  if (read_file_data(reader, mesh->ind, buffer_view->byte_length) != 0)
    return 1;
  return 0;
}

static int convert_gltf_mesh_primitive(struct MODEL_READER *reader, struct GLTF_NODE *node, struct GLTF_MESH_PRIMITIVE *prim)
{
  if (reader->model->n_meshes >= MODEL_MAX_MESHES) {
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
  uint16_t used_vtx_attribs;
  uint8_t mesh_vtx_type = convert_gltf_vtx_type(prim->attribs_present, &used_vtx_attribs);
  if (mesh_vtx_type == 0xff) {
    debug_log("* WARNING: ignoring primitive with unsupported vertex attributes (%x)\n", prim->attribs_present);
    return 0;
  }
  uint32_t vtx_buffer_size;
  if (extract_vtx_buffer_size_from_attribs(reader->gltf, prim, used_vtx_attribs, &vtx_buffer_size) != 0)
    return 0;

  // read index data info
  struct GLTF_ACCESSOR *indices_accessor = &reader->gltf->accessors[prim->indices_accessor];
  uint8_t mesh_ind_type = convert_gltf_ind_type(indices_accessor->component_type);
  if (mesh_ind_type == 0xff) {
    debug_log("* WARNING: ignoring primitive with unsupported index component type %u\n", indices_accessor->component_type);
    return 0;
  }
  struct GLTF_BUFFER_VIEW *indices_buffer_view = &reader->gltf->buffer_views[indices_accessor->buffer_view];
  uint32_t ind_buffer_size = indices_buffer_view->byte_length;

  debug_log("-> adding mesh with vtx_size=%u, ind_size=%u\n", vtx_buffer_size, ind_buffer_size);
  struct MODEL_MESH *model_mesh = new_model_mesh(mesh_vtx_type, vtx_buffer_size, mesh_ind_type, ind_buffer_size, indices_accessor->count);
  mat4_copy(model_mesh->matrix, node->matrix);

  // extract mesh data
  if (extract_vtx_buffer_data(model_mesh, reader, prim, used_vtx_attribs) != 0)
    return 1;
  if (extract_ind_buffer_data(model_mesh, reader, indices_accessor) != 0)
    return 1;
  
  // convert material
  if (prim->material != GLTF_NONE) {
    if (convert_gltf_material(reader, model_mesh, &reader->gltf->materials[prim->material]) != 0)
      return 1;
  }
  
  reader->model->meshes[reader->model->n_meshes++] = model_mesh;
  return 0;
}

static int convert_gltf_node(struct MODEL_READER *reader, struct GLTF_NODE *node)
{
  if (node->mesh == GLTF_NONE)
    return 0;
  struct GLTF_MESH *mesh = &reader->gltf->meshes[node->mesh];
  for (uint16_t i = 0; i < mesh->n_primitives; i++) {
    if (convert_gltf_mesh_primitive(reader, node, &mesh->primitives[i]) != 0)
      return 1;
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

static void init_model(struct MODEL *model)
{
  model->n_meshes = 0;
  model->n_textures = 0;
}

int read_glb_model(struct MODEL *model, const char *filename, uint32_t flags)
{
  struct GLB_FILE glb;
  if (open_glb(&glb, filename) != 0)
    return 1;
  init_model(model);
  
  struct MODEL_READER reader = {
    .model = model,
    .gltf = glb.gltf,
    .file = glb.file,
    .data_off = glb.data_off,
    .data_len = glb.data_len,
    .read_flags = flags,
  };
  for (int i = 0; i < GLTF_MAX_TEXTURES; i++)
    reader.converted_textures[i] = MODEL_TEXTURE_NONE;
  
  struct GLTF_DATA *gltf = reader.gltf;
  if (gltf->scene == GLTF_NONE)
    goto err;
  struct GLTF_SCENE *scene = &gltf->scenes[gltf->scene];
  for (uint16_t i = 0; i < scene->n_nodes; i++) {
    if (convert_gltf_node(&reader, &gltf->nodes[i]) != 0)
      goto err;
  }

  close_glb(&glb);
  return 0;

 err:
  free_model(model);
  close_glb(&glb);
  return 1;
}
