/* render.c */

#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "render.h"
#include "shader.h"
#include "model.h"
#include "matrix.h"
#include "camera.h"
#include "game.h"

struct GFX_MESH {
  GLuint vtx_array_obj;
  GLuint vtx_buf_obj;
  GLuint index_buf_obj;
  
  uint32_t index_count;
  uint32_t index_type;
  float matrix[16];
  GLuint texture_id;
};

struct GFX_TEXTURE {
  GLuint id;

  int uploaded;
};

struct GFX_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_mat_model_view_projection;
  GLint uni_mat_normal;
};


static int n_meshes;
static struct GFX_MESH meshes[16];
static struct GFX_TEXTURE textures[16];
static struct GFX_SHADER shader;

static float mat_projection[16];


static int load_shader(void)
{
  shader.id = load_program_shader("data/vert.glsl", "data/frag.glsl");
  if (shader.id == 0)
    return 1;

  get_shader_uniform_id(shader.id, &shader.uni_tex1, "tex1");
  get_shader_uniform_id(shader.id, &shader.uni_mat_model_view_projection, "mat_model_view_projection");
  get_shader_uniform_id(shader.id, &shader.uni_mat_normal, "mat_normal");
  return 0;
}

#if 0
static void dump_vtx_buffer(float *data, size_t size)
{
  float *end = data + size/sizeof(float);
  while (data < end) {
    printf("pos [ %+f, %+f, %+f ] normal [ %+f, %+f, %+f ] uv [ %+f, %+f ]\n",
           data[0], data[1], data[2],
           data[3], data[4], data[5],
           data[6], data[7]);
    data += 8;
  }
}
#endif

static int upload_model_mesh(struct MODEL_MESH *mesh, struct GFX_MESH *gfx)
{
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
  return 0;
}

static int upload_model_texture(struct MODEL_TEXTURE *texture, struct GFX_TEXTURE *gfx)
{
  GL_CHECK(glGenTextures(1, &gfx->id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, gfx->id));
  gfx->uploaded = 1;

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (texture->n_chan == 3)
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  texture->width, texture->height, 0, GL_RGB,  GL_UNSIGNED_BYTE, texture->data));
  else
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data));
  
  GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  return 0;
}

static int load_model(void)
{
  const char *filename = "data/rooms0-2.glb";
  //const char *filename = "data/tex_plane.glb";
  //const char *filename = "data/square.glb";

  struct MODEL model;
  if (read_glb_model(&model, filename) != 0) {
    console("ERROR reading model GLB file\n");
    return 1;
  }

  for (int i = 0; i < (int) (sizeof(textures)/sizeof(textures[0])); i++)
    textures[i].uploaded = 0;
  
  n_meshes = model.n_meshes;
  for (int i = 0; i < n_meshes; i++) {
    upload_model_mesh(model.meshes[i], &meshes[i]);
    int tex_index = model.meshes[i]->tex0_index;

    if (! textures[tex_index].uploaded)
      upload_model_texture(&model.textures[tex_index], &textures[tex_index]);
    meshes[i].texture_id = textures[tex_index].id;
  }
  free_model(&model);
  return 0;
}

static void setup_projection(float aspect)
{
  //mat4_frustum(mat_projection, -1.0,1.0,  -1.0,1.0,  1.0,1200.0);
  //mat4_perspective(mat_projection, aspect, M_PI/3, 1.0, 1200.0);
  mat4_inf_perspective(mat_projection, aspect, M_PI/3, 1.0);
}

int render_setup(float aspect)
{
  if (load_shader() != 0)
    return 1;

  if (load_model() != 0)
    return 1;

  setup_projection(aspect);
  init_camera(&camera);
  
  glClearColor(0.0, 0.0, 0.4, 1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glEnable(GL_CULL_FACE);
  //glFrontFace(GL_CCW);
  
  return 0;
}

void render_set_viewport(int width, int height)
{
  glViewport(0, 0, width, height);
  setup_projection((float) width / height);
}

static void render_mesh(struct GFX_MESH *mesh, float *mat_view_projection, float *mat_view)
{
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, mesh->texture_id));

  // model_view_projection
  float mat_model_view_projection[16];
  mat4_mul(mat_model_view_projection, mat_view_projection, mesh->matrix);

  // normal
  float mat_model_view[16], mat_inv[16], mat_normal[16];
  mat4_mul(mat_model_view, mat_view, mesh->matrix);
  mat4_inverse(mat_inv, mat_model_view);
  mat4_transpose(mat_normal, mat_inv);
  
  if (shader.uni_mat_model_view_projection >= 0)
    GL_CHECK(glUniformMatrix4fv(shader.uni_mat_model_view_projection, 1, GL_TRUE, mat_model_view_projection));
  if (shader.uni_mat_normal >= 0)
    GL_CHECK(glUniformMatrix4fv(shader.uni_mat_normal, 1, GL_TRUE, mat_normal));
  
  GL_CHECK(glBindVertexArray(mesh->vtx_array_obj));
  GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->index_count, mesh->index_type, 0));
}

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  GL_CHECK(glUseProgram(shader.id));
  GL_CHECK(glUniform1i(shader.uni_tex1, 0));

  float mat_view[16];
  get_camera_matrix(&camera, mat_view);
  
  float mat_view_projection[16];
  mat4_mul(mat_view_projection, mat_projection, mat_view);

  for (int i = 0; i < n_meshes; i++)
    render_mesh(&meshes[i], mat_view_projection, mat_view);
}
