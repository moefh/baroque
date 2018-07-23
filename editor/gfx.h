/* gfx.h */

#ifndef GFX_H_FILE
#define GFX_H_FILE

#include <glad/glad.h>

#define NUM_GFX_MESHES   1024
#define NUM_GFX_TEXTURES 1024

#define GFX_TEX_FLAG_NO_REPEAT  (1<<0)
#define GFX_TEX_FLAG_NO_FILTER  (1<<1)
#define GFX_TEX_FLAG_NO_MIPMAP  (1<<2)

#define GFX_MESH_TYPE_ROOM 0
#define GFX_MESH_TYPE_FONT 1
#define GFX_MESH_TYPE_GRID 2
#define GFX_MESH_TYPE_TEST 3

struct GFX_TEXTURE {
  int use_count;
  GLuint id;
};

struct GFX_MESH {
  int use_count;
  GLuint vtx_array_obj;
  GLuint vtx_buf_obj;
  GLuint index_buf_obj;

  struct GFX_TEXTURE *texture;
  uint32_t index_count;
  uint32_t index_type;
  float matrix[16];
  uint32_t type;
  uint32_t info;
  void *data;
};

extern int num_gfx_meshes;
extern int num_gfx_textures;
extern struct GFX_MESH gfx_meshes[NUM_GFX_MESHES];
extern struct GFX_TEXTURE gfx_textures[NUM_GFX_TEXTURES];

struct FONT;
struct GRID_TILES;
struct MODEL;
struct MODEL_MESH;
struct MODEL_TEXTURE;

struct GFX_MESH *gfx_upload_font(struct FONT *font);
struct GFX_MESH *gfx_upload_grid_tiles(struct GRID_TILES *tiles);
struct GFX_MESH *gfx_upload_model_mesh(struct MODEL_MESH *mesh, uint32_t type, uint32_t info, void *data);
struct GFX_TEXTURE *gfx_upload_model_texture(struct MODEL_TEXTURE *texture, unsigned int flags);
void gfx_update_texture(struct GFX_TEXTURE *tex, int xoff, int yoff, int width, int height, void *data, int n_chan);
int gfx_upload_model(struct MODEL *model, uint32_t type, uint32_t info, void *data);

int gfx_free_meshes(uint32_t type, uint32_t info);
void gfx_free_mesh(struct GFX_MESH *mesh);
void gfx_free_texture(struct GFX_TEXTURE *tex);

#endif /* GFX_H_FILE */
