/* model.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "model.h"
#include "gltf.h"
#include "matrix.h"

/* ================================================================ */
/* DATA reader */
/* ================================================================ */

struct DATA_READER {
  struct MODEL *model;
  struct GLTF_DATA *gltf;
  FILE *file;
  uint32_t data_off;
  uint32_t data_len;
};

static uint8_t convert_gltf_vtx_type(uint16_t prim_attibs_present, uint16_t *use_vtx_attribs)
{
#define ATTRS_FOR_POS            ((1<<GLTF_MESH_ATTRIB_POSITION))
#define ATTRS_FOR_POS_UV1        ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0))
#define ATTRS_FOR_POS_UV2        ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_1))
#define ATTRS_FOR_POS_NORMAL     ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL))
#define ATTRS_FOR_POS_NORMAL_UV1 ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0))
#define ATTRS_FOR_POS_NORMAL_UV2 ((1<<GLTF_MESH_ATTRIB_POSITION)|(1<<GLTF_MESH_ATTRIB_NORMAL)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_0)|(1<<GLTF_MESH_ATTRIB_TEXCOORD_1))
  
#define TEST_ATTRIBS(X) do {                                            \
    if ((prim_attibs_present & ATTRS_FOR_##X) == ATTRS_FOR_##X) {       \
      *use_vtx_attribs = ATTRS_FOR_##X;                                 \
      return MODEL_MESH_VTX_##X;                                          \
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

static int extract_vtx_buffer_size_from_attribs(struct GLTF_DATA *gltf, struct GLTF_MESH_PRIMITIVE *prim, uint16_t used_vtx_attribs)
{
  uint32_t vtx_buffer_size = 0;
  for (uint16_t attrib_num = 0; attrib_num < GLTF_MESH_NUM_ATTRIBS; attrib_num++) {
    if (used_vtx_attribs & (1<<attrib_num)) {
      struct GLTF_ACCESSOR *accessor = &gltf->accessors[prim->attribs[attrib_num]];
      struct GLTF_BUFFER_VIEW *buffer_view = &gltf->buffer_views[accessor->buffer_view];
      vtx_buffer_size += buffer_view->byte_length;
    }
  }
  return vtx_buffer_size;
}

static int read_gltf_mesh_primitive(struct DATA_READER *reader, struct GLTF_MESH_PRIMITIVE *prim)
{
  if (reader->model->n_meshes >= MODEL_MAX_MESHES) {
    printf("* WARNING: ignoring primitive: too many meshes converted\n");
    return 0;
  }
  if (prim->mode != GLTF_MESH_MODE_TRIANGLES) {
    printf("* WARNING: ignoring primitive with non-triangles mode (not implemented)\n");
    return 0;
  }
  if (prim->indices_accessor == GLTF_NONE) {
    printf("* WARNING: ignoring primitive with no indices accessor\n");
    return 0;
  }

  // read vtx data info
  uint16_t used_vtx_attribs;
  uint8_t model_vtx_type = convert_gltf_vtx_type(prim->attribs_present, &used_vtx_attribs);
  if (model_vtx_type == 0xff) {
    printf("* WARNING: ignoring primitive with unsupported attributes (%x)\n", prim->attribs_present);
    return 0;
  }
  uint32_t vtx_buffer_size = extract_vtx_buffer_size_from_attribs(reader->gltf, prim, used_vtx_attribs);

  // read index data info
  struct GLTF_ACCESSOR *indices_accessor = &reader->gltf->accessors[prim->indices_accessor];
  uint8_t model_ind_type = convert_gltf_ind_type(indices_accessor->component_type);
  if (model_ind_type == 0xff) {
    printf("* WARNING: ignoring primitive with unsupported index component type %u\n", indices_accessor->component_type);
    return 0;
  }
  struct GLTF_BUFFER_VIEW *indices_buffer_view = &reader->gltf->buffer_views[indices_accessor->buffer_view];
  uint32_t ind_buffer_size = indices_buffer_view->byte_length;

  // read material info
  //struct GLTF_MATERIAL *material = (prim->material == GLTF_NONE) ? NULL : &gltf->materials[prim->material];
  
  printf("mesh with vtx_size=%u, ind_size=%u\n", vtx_buffer_size, ind_buffer_size);
  struct MODEL_MESH *model_mesh = new_model_mesh(model_vtx_type, vtx_buffer_size, model_ind_type, ind_buffer_size);

  //if (convert_gltf_vtx_buffer(model_mesh->vtx, prim->attribs_present, &used_vtx_attribs) != 0)
  //  return 1;
  //if (convert_gltf_ind_buffer(model_mesh->ind, indices_accessor) != 0)
  //  return 1;
  
  reader->model->meshes[reader->model->n_meshes++] = model_mesh;
  return 0;
}

static int read_gltf_node(struct DATA_READER *reader, struct GLTF_NODE *node)
{
  if (node->mesh == GLTF_NONE) {
    printf("* ERROR: node contains invalid mesh reference\n");
    return 1;
  }
  struct GLTF_MESH *mesh = &reader->gltf->meshes[node->mesh];
  for (uint16_t i = 0; i < mesh->n_primitives; i++) {
    if (read_gltf_mesh_primitive(reader, &mesh->primitives[i]) != 0)
      return 1;
  }
  
  return 0;
}

struct MODEL_MESH *new_model_mesh(uint8_t vtx_type, uint32_t vtx_size, uint8_t ind_type, uint32_t ind_size)
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
  mesh->vtx = mesh->data;
  mesh->ind = (char *)mesh->data + v_size;
  return mesh;
}

void free_model(struct MODEL *model)
{
  for (int i = 0; i < model->n_meshes; i++)
    free(model->meshes[i]);

  for (int i = 0; i < model->n_images; i++)
    free(model->images[i]);
}

int read_glb_model(struct MODEL *model, const char *filename)
{
  struct GLB_READER glb;
  if (read_glb(&glb, filename) != 0)
    return 1;
  
  struct DATA_READER reader = {
    .model = model,
    .gltf = &glb.gltf_reader->gltf,
    .file = glb.file,
    .data_off = glb.data_off,
    .data_len = glb.data_len,
  };
  
  struct GLTF_DATA *gltf = reader.gltf;
  if (gltf->scene == GLTF_NONE)
    goto err;

  struct GLTF_SCENE *scene = &gltf->scenes[gltf->scene];
  for (uint16_t i = 0; i < scene->n_nodes; i++) {
    if (read_gltf_node(&reader, &gltf->nodes[i]) != 0)
      goto err;
  }

  fclose(glb.file);
  free_gltf_reader(glb.gltf_reader);
  return 0;

 err:
  fclose(glb.file);
  free_gltf_reader(glb.gltf_reader);
  return 1;
}
