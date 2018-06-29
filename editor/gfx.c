/* gfx.c */

#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gfx.h"
#include "debug.h"
#include "gl_error.h"
#include "matrix.h"
#include "model.h"
#include "font.h"

int num_gfx_meshes;
int num_gfx_textures;
struct GFX_MESH gfx_meshes[NUM_GFX_MESHES];
struct GFX_TEXTURE gfx_textures[NUM_GFX_TEXTURES];

struct GFX_MESH *upload_model_mesh(struct MODEL_MESH *mesh, uint32_t type, uint32_t info)
{
  struct GFX_MESH *gfx = NULL;
  for (int i = 0; i < num_gfx_meshes; i++) {
    if (! gfx_meshes[i].used) {
      gfx = &gfx_meshes[i];
      break;
    }
  }
  if (! gfx) {
    if (num_gfx_meshes >= NUM_GFX_MESHES)
      return NULL;
    gfx = &gfx_meshes[num_gfx_meshes++];
  }
  gfx->used = 1;
  gfx->type = type;
  gfx->info = info;
  
  GL_CHECK(glGenVertexArrays(1, &gfx->vtx_array_obj));
  GL_CHECK(glBindVertexArray(gfx->vtx_array_obj));

  //dump_vtx_buffer(mesh->vtx, mesh->vtx_size);
  
  GL_CHECK(glGenBuffers(1, &gfx->vtx_buf_obj));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, gfx->vtx_buf_obj));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, mesh->vtx_size, mesh->vtx, GL_STATIC_DRAW));
  
  GL_CHECK(glGenBuffers(1, &gfx->index_buf_obj));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->index_buf_obj));
  GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->ind_size, mesh->ind, GL_STATIC_DRAW));

  mat4_copy(gfx->matrix, mesh->matrix);
  
  gfx->index_count = mesh->ind_count;
  switch (mesh->ind_type) {
  case MODEL_MESH_IND_U8:  gfx->index_type = GL_UNSIGNED_BYTE; break;
  case MODEL_MESH_IND_U16: gfx->index_type = GL_UNSIGNED_SHORT; break;
  case MODEL_MESH_IND_U32: gfx->index_type = GL_UNSIGNED_INT; break;
  }
  
  switch (mesh->vtx_type) {
  case MODEL_MESH_VTX_POS:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, (void *) 0));
    GL_CHECK(glEnableVertexAttribArray(0));
    break;

  case MODEL_MESH_VTX_POS_UV1:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+2), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+2), (void *) (sizeof(float)*(3))));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    break;
    
  case MODEL_MESH_VTX_POS_UV2:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+2+2), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+2+2), (void *) (sizeof(float)*(3))));
    GL_CHECK(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+2+2), (void *) (sizeof(float)*(3+2))));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glEnableVertexAttribArray(2));
    break;
    
  case MODEL_MESH_VTX_POS_NORMAL:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3), (void *) (sizeof(float)*(3))));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    break;
    
  case MODEL_MESH_VTX_POS_NORMAL_UV1:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2), (void *) (sizeof(float)*(3))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2), (void *) (sizeof(float)*(3+3))));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glEnableVertexAttribArray(2));
    break;
    
  case MODEL_MESH_VTX_POS_NORMAL_UV2:
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+2), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+2), (void *) (sizeof(float)*(3))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+2), (void *) (sizeof(float)*(3+3))));
    GL_CHECK(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+2), (void *) (sizeof(float)*(3+3+2))));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glEnableVertexAttribArray(3));
    break;
  }    
  return gfx;
}

static struct GFX_TEXTURE *upload_model_texture(struct MODEL_TEXTURE *texture)
{
  struct GFX_TEXTURE *gfx = NULL;
  for (int i = 0; i < num_gfx_textures; i++) {
    if (! gfx_textures[i].used) {
      gfx = &gfx_textures[i];
      break;
    }
  }
  if (! gfx) {
    if (num_gfx_textures >= NUM_GFX_TEXTURES)
      return NULL;
    gfx = &gfx_textures[num_gfx_textures++];
  }
  gfx->used = 1;
  
  GL_CHECK(glGenTextures(1, &gfx->id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, gfx->id));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (texture->n_chan == 3)
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  texture->width, texture->height, 0, GL_RGB,  GL_UNSIGNED_BYTE, texture->data));
  else
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data));
  
  GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  return gfx;
}

int upload_model(struct MODEL *model, uint32_t type, uint32_t info)
{
  struct GFX_TEXTURE *gfx_textures[64];
  for (int i = 0; i < 64; i++)
    gfx_textures[i] = NULL;
  
  for (int i = 0; i < model->n_meshes; i++) {
    struct GFX_MESH *mesh = upload_model_mesh(model->meshes[i], type, info + i);
    if (! mesh) {
      console("can't upload mesh\n");
      return 1;
    }

    int tex_num = model->meshes[i]->tex0_index;
    if (tex_num != MODEL_TEXTURE_NONE) {
      struct GFX_TEXTURE *tex = gfx_textures[tex_num];
      if (! tex) {
        tex = upload_model_texture(&model->textures[tex_num]);
        if (! tex) {
          console("can't upload texture\n");
          return 1;
        }
        gfx_textures[tex_num] = tex;
      }
      mesh->texture_id = tex->id;
    }
  }

  return 0;
}

struct GFX_MESH *upload_font(struct FONT *font)
{
  struct GFX_MESH *mesh = upload_model_mesh(font->mesh, GFX_MESH_TYPE_FONT, 0);
  if (! mesh)
    return NULL;
  
  struct GFX_TEXTURE *tex = upload_model_texture(&font->texture);
  if (! tex)
    return NULL;
  
  mesh->texture_id = tex->id;
  return mesh;
}
