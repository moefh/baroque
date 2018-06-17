/* render.c */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "render.h"
#include "shader.h"
#include "model.h"

static GLuint vtx_array_obj;
static GLuint vtx_buf_obj;
static GLuint index_buf_obj;
static GLuint texture_id;

static GLuint shader_id;
static GLint shader_uni_tex1;

uint32_t model_index_count;
uint32_t model_index_type;

float mat_projection[16];

static int load_shader(void)
{
  shader_id = load_program_shader("data/vert.glsl", "data/frag.glsl");
  if (shader_id == 0)
    return 1;

  get_shader_uniform_id(shader_id, &shader_uni_tex1, "tex1");
  
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

static int upload_model_mesh(struct MODEL_MESH *mesh)
{
  GL_CHECK(glGenVertexArrays(1, &vtx_array_obj));
  GL_CHECK(glBindVertexArray(vtx_array_obj));

  GL_CHECK(glGenBuffers(1, &vtx_buf_obj));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vtx_buf_obj));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, mesh->vtx_size, mesh->vtx, GL_STATIC_DRAW));
  
  GL_CHECK(glGenBuffers(1, &index_buf_obj));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buf_obj));
  GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->ind_size, mesh->ind, GL_STATIC_DRAW));

  model_index_count = mesh->ind_count;
  switch (mesh->ind_type) {
  case MODEL_MESH_IND_U8:  model_index_type = GL_UNSIGNED_BYTE; break;
  case MODEL_MESH_IND_U16: model_index_type = GL_UNSIGNED_SHORT; break;
  case MODEL_MESH_IND_U32: model_index_type = GL_UNSIGNED_INT; break;
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

static int upload_model_texture(struct MODEL_TEXTURE *texture)
{
  GL_CHECK(glGenTextures(1, &texture_id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id));

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

int render_setup(void)
{
  if (load_shader() != 0)
    return 1;

  struct MODEL model;
  //const char *filename = "data/rooms0-2.glb";
  //const char *filename = "data/tex_plane.glb";
  const char *filename = "data/square.glb";
  if (read_glb_model(&model, filename) == 0) {
    upload_model_mesh(model.meshes[0]);
    upload_model_texture(&model.textures[model.meshes[0]->tex0_index]);
    free_model(&model);
  } else {
    console("ERROR reading model GLB file\n");
    return 1;
  }

  glClearColor(0.0, 0.0, 0.4, 1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  return 0;
}

void render_set_viewport(int width, int height)
{
  glViewport(0, 0, width, height);
  //float aspect = (float) width / height;
  //mat4_frustum(mat_projection, -aspect, aspect, -1.0, 1.0, 1.0, 1200.0);
}

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id));

  GL_CHECK(glUseProgram(shader_id));
  GL_CHECK(glUniform1i(shader_uni_tex1, 0));

  GL_CHECK(glBindVertexArray(vtx_array_obj));
  GL_CHECK(glDrawElements(GL_TRIANGLES, model_index_count, model_index_type, 0));
}
