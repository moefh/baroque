/* gfx.h */

#ifndef GFX_H_FILE
#define GFX_H_FILE

#define NUM_GFX_MESHES   1024
#define NUM_GFX_TEXTURES 1024

#define GFX_MESH_TYPE_ROOM 0
#define GFX_MESH_TYPE_FONT 1
#define GFX_MESH_TYPE_GRID 2

struct GFX_MESH {
  GLuint vtx_array_obj;
  GLuint vtx_buf_obj;
  GLuint index_buf_obj;
  GLuint texture_id;

  uint32_t index_count;
  uint32_t index_type;
  float matrix[16];
  int used;
  uint32_t type;
  uint32_t info;
};

struct GFX_TEXTURE {
  GLuint id;
  int used;
};

extern int num_gfx_meshes;
extern int num_gfx_textures;
extern struct GFX_MESH gfx_meshes[NUM_GFX_MESHES];
extern struct GFX_TEXTURE gfx_textures[NUM_GFX_TEXTURES];

struct FONT;
struct MODEL;
struct MODEL_MESH;

struct GFX_MESH *upload_font(struct FONT *font);
struct GFX_MESH *upload_model_mesh(struct MODEL_MESH *mesh, uint32_t type, uint32_t info);
int upload_model(struct MODEL *model, uint32_t type, uint32_t info);

#endif /* GFX_H_FILE */
