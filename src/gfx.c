/* gfx.c */

#include <math.h>
#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gfx.h"
#include "debug.h"
#include "gl_error.h"
#include "matrix.h"
#include "model.h"
#include "font.h"

//#define DEBUG_GFX
#ifdef DEBUG_GFX
#define debug_log console
#else
#define debug_log(...)
#endif

static struct GFX_MESH *gfx_mesh_free_list;
static struct GFX_MESH *gfx_mesh_used_list;
static struct GFX_TEXTURE *gfx_texture_free_list;
static struct GFX_TEXTURE *gfx_texture_used_list;
struct GFX_MESH gfx_meshes[NUM_GFX_MESHES];
struct GFX_TEXTURE gfx_textures[NUM_GFX_TEXTURES];

void init_gfx(void)
{
  for (int i = 0; i < NUM_GFX_MESHES-1; i++)
    gfx_meshes[i].next = &gfx_meshes[i+1];
  gfx_meshes[NUM_GFX_MESHES-1].next = NULL;
  gfx_mesh_free_list = &gfx_meshes[0];
  gfx_mesh_used_list = NULL;

  for (int i = 0; i < NUM_GFX_TEXTURES-1; i++)
    gfx_textures[i].next = &gfx_textures[i+1];
  gfx_textures[NUM_GFX_TEXTURES-1].next = NULL;
  gfx_texture_free_list = &gfx_textures[0];
  gfx_texture_used_list = NULL;
}

static void gfx_free_texture(struct GFX_TEXTURE *tex)
{
  glDeleteTextures(1, &tex->id);
  tex->use_count = 0;

  // remove from used list
  struct GFX_TEXTURE **p = &gfx_texture_used_list;
  while (*p && *p != tex)
    p = &(*p)->next;
  if (! *p) {
    debug("** ERROR: trying to free unused gfx texture\n");
    return;
  }
  *p = tex->next;
  
  // add to free list
  tex->next = gfx_texture_free_list;
  gfx_texture_free_list = tex;
}

struct GFX_TEXTURE *gfx_alloc_texture(void)
{
  struct GFX_TEXTURE *tex = gfx_texture_free_list;
  if (tex == NULL)
    return NULL;
  gfx_texture_free_list = tex->next;
  tex->next = gfx_texture_used_list;
  gfx_texture_used_list = tex;
  
  tex->use_count = 1;
  tex->flags = 0;
  return tex;
}

void gfx_release_texture(struct GFX_TEXTURE *tex)
{
  tex->use_count--;
  if (tex->use_count == 0)
    gfx_free_texture(tex);
}

static void gfx_unload_mesh(struct GFX_MESH *mesh)
{
  GL_CHECK(glDeleteVertexArrays(1, &mesh->vtx_array_obj));
  GL_CHECK(glDeleteBuffers(1, &mesh->vtx_buf_obj));
  GL_CHECK(glDeleteBuffers(1, &mesh->index_buf_obj));

  if (mesh->texture)
    gfx_release_texture(mesh->texture);

  mesh->use_count = 0;
}

void gfx_free_mesh(struct GFX_MESH *mesh)
{
  gfx_unload_mesh(mesh);
  
  // remove from used list
  struct GFX_MESH **p = &gfx_mesh_used_list;
  while (*p && *p != mesh)
    p = &(*p)->next;
  if (! *p) {
    debug("** ERROR: trying to free unused gfx mesh\n");
    return;
  }
  *p = mesh->next;
  
  // add to free list
  mesh->next = gfx_mesh_free_list;
  gfx_mesh_free_list = mesh;
}

int gfx_free_meshes(uint32_t type, uint32_t info)
{
  debug_log("-> freeing all meshes with (type=%u, info=%u)\n", type, info);
  int n_released_meshes = 0;
  struct GFX_MESH **p = &gfx_mesh_used_list;
  while (*p) {
    struct GFX_MESH *mesh = *p;
    if (mesh->type == type && mesh->info == info) {
      debug_log("-> freeing mesh %d\n", (int) (mesh - gfx_meshes));
      n_released_meshes++;
      gfx_unload_mesh(mesh);
      
      // remove from used list
      *p = mesh->next;

      // add to free list
      mesh->next = gfx_mesh_free_list;
      gfx_mesh_free_list = mesh;
    } else {
      p = &mesh->next;
    }
  }

  return n_released_meshes;
}

static struct GFX_MESH *gfx_alloc_mesh(void)
{
  struct GFX_MESH *mesh = gfx_mesh_free_list;
  if (mesh == NULL)
    return NULL;
  gfx_mesh_free_list = mesh->next;
  mesh->next = gfx_mesh_used_list;
  gfx_mesh_used_list = mesh;

  mesh->use_count = 1;
  return mesh;
}

struct GFX_MESH *gfx_upload_model_mesh(struct MODEL_MESH *mesh, uint32_t type, uint32_t info, void *data)
{
  struct GFX_MESH *gfx = gfx_alloc_mesh();
  if (! gfx)
    return NULL;
  gfx->type = type;
  gfx->info = info;
  gfx->data = data;
  gfx->texture = NULL;

  debug_log("-> uploading mesh %d with (type=%u, info=%u)\n", (int) (gfx - gfx_meshes), type, info);

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
  
  // TODO: handle *_SKEL types properly
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

  case MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL1:
#define USHORT GL_UNSIGNED_SHORT
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4) + sizeof(uint16_t)*4, (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4) + sizeof(uint16_t)*4, (void *) (sizeof(float)*(3))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4) + sizeof(uint16_t)*4, (void *) (sizeof(float)*(3+3))));
    GL_CHECK(glVertexAttribPointer(3, 4, USHORT,   GL_FALSE, sizeof(float)*(3+3+2+4) + sizeof(uint16_t)*4, (void *) (sizeof(float)*(3+3+2))));
    GL_CHECK(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4) + sizeof(uint16_t)*4, (void *) (sizeof(float)*(3+3+2)+sizeof(uint16_t)*(4))));
#undef USHORT
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glEnableVertexAttribArray(4));
    break;
    
  case MODEL_MESH_VTX_POS_NORMAL_UV1_SKEL2:
#define USHORT GL_UNSIGNED_SHORT
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(0))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3+3))));
    GL_CHECK(glVertexAttribPointer(3, 4, USHORT,   GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3+3+2))));
    GL_CHECK(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3+3+2)+sizeof(uint16_t)*(4))));
    GL_CHECK(glVertexAttribPointer(5, 4, USHORT,   GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3+3+2+4)+sizeof(uint16_t)*(4))));
    GL_CHECK(glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(float)*(3+3+2+4+4) + sizeof(uint16_t)*(4+4), (void *) (sizeof(float)*(3+3+2+4)+sizeof(uint16_t)*(4+4))));
#undef USHORT
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glEnableVertexAttribArray(4));
    GL_CHECK(glEnableVertexAttribArray(5));
    GL_CHECK(glEnableVertexAttribArray(6));
    break;

  default:
    console("** WARNING: unsupported vertex type: %d\n", mesh->vtx_type);
    break;
  }    
  return gfx;
}

void gfx_update_texture(struct GFX_TEXTURE *gfx, int xoff, int yoff, int width, int height, void *data, int n_chan)
{
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, gfx->id));
  if (n_chan == 3)
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, width, height, GL_RGB,  GL_UNSIGNED_BYTE, data));
  else
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

void gfx_upload_model_texture(struct GFX_TEXTURE *gfx, struct MODEL_TEXTURE *texture, unsigned int flags)
{
  GL_CHECK(glGenTextures(1, &gfx->id));
  debug_log("-> uploading texture id %d\n", gfx->id);
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, gfx->id));

  if (flags & GFX_TEX_UPLOAD_FLAG_NO_REPEAT) {
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  } else {
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  }

  if (flags & GFX_TEX_UPLOAD_FLAG_NO_FILTER) {
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  } else {
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & GFX_TEX_UPLOAD_FLAG_NO_MIPMAP) ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  }

  if (texture->n_chan == 3)
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  texture->width, texture->height, 0, GL_RGB,  GL_UNSIGNED_BYTE, texture->data));
  else
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data));

  if ((flags & GFX_TEX_UPLOAD_FLAG_NO_MIPMAP) == 0)
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  gfx->flags = GFX_TEX_FLAG_LOADED;
}

int gfx_upload_model(struct MODEL *model, uint32_t type, uint32_t info, void *data)
{
  struct GFX_TEXTURE *gfx_textures[64];
  for (int i = 0; i < 64; i++)
    gfx_textures[i] = NULL;
  
  for (int i = 0; i < model->n_meshes; i++) {
    struct GFX_MESH *mesh = gfx_upload_model_mesh(model->meshes[i], type, info, data);
    if (! mesh) {
      debug("** ERROR: can't upload mesh\n");
      return 1;
    }

    int tex_num = model->meshes[i]->tex0_index;
    if (tex_num != MODEL_TEXTURE_NONE) {
      struct GFX_TEXTURE *tex = gfx_textures[tex_num];
      if (! tex) {
        tex = gfx_alloc_texture();
        gfx_upload_model_texture(tex, &model->textures[tex_num], 0);
        if (! tex) {
          debug("** ERROR: can't upload texture\n");
          return 1;
        }
        gfx_textures[tex_num] = tex;
      } else {
        tex->use_count++;
      }
      mesh->texture = tex;
    }
  }

  return 0;
}

struct GFX_MESH *gfx_upload_font(struct FONT *font)
{
  struct GFX_MESH *mesh = gfx_upload_model_mesh(font->mesh, GFX_MESH_TYPE_STATIC, 0, NULL);
  if (! mesh)
    return NULL;
  
  mesh->texture = gfx_alloc_texture();
  gfx_upload_model_texture(mesh->texture, &font->texture, 0);
  if (! mesh->texture) {
    gfx_free_mesh(mesh);
    return NULL;
  }
  return mesh;
}
