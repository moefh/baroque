/* render.c */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "render.h"
#include "shader.h"

static GLuint vtx_array_obj;
static GLuint vtx_buf_obj;
static GLuint index_buf_obj;
static GLuint texture_id;

static GLuint shader_id;
static GLint shader_uni_tex1;

float model_vtx[] = {
  // position           // color            // uv
   0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
   0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
  -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
  -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,
};
unsigned int model_indices[] = {  
  0, 1, 3,
  1, 2, 3,
};

static int load_shader(void)
{
  shader_id = load_program_shader("data/vert.glsl", "data/frag.glsl");
  if (shader_id == 0)
    return 1;

  get_shader_uniform_id(shader_id, &shader_uni_tex1, "tex1");
  
  return 0;
}

static int load_model(void)
{
  GL_CHECK(glGenVertexArrays(1, &vtx_array_obj));
  GL_CHECK(glBindVertexArray(vtx_array_obj));

  GL_CHECK(glGenBuffers(1, &vtx_buf_obj));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vtx_buf_obj));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(model_vtx), model_vtx, GL_STATIC_DRAW));
  
  GL_CHECK(glGenBuffers(1, &index_buf_obj));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buf_obj));
  GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(model_indices), model_indices, GL_STATIC_DRAW));
  
  GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void *) 0));
  GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void *) (3*sizeof(GLfloat))));
  GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void *) (6*sizeof(GLfloat))));

  GL_CHECK(glEnableVertexAttribArray(0));
  GL_CHECK(glEnableVertexAttribArray(1));
  GL_CHECK(glEnableVertexAttribArray(2));
  
  return 0;
}

static int load_texture(void)
{
  stbi_set_flip_vertically_on_load(1);
  int width, height, n_chan;
  unsigned char *data = stbi_load("data/texture.png", &width, &height, &n_chan, 0);
  if (! data) {
    debug("* ERROR loading texture\n");
    return 1;
  }
  if (n_chan != 3 && n_chan != 4) {
    debug("* ERROR: texture has invalid format\n");
    stbi_image_free(data);
    return 1;
  }
  
  GL_CHECK(glGenTextures(1, &texture_id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (n_chan == 3)
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_RGB,  GL_UNSIGNED_BYTE, data));
  else
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
  
  GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  stbi_image_free(data);

  return 0;
}

int render_setup(void)
{
  if (load_shader() != 0)
    return 1;

  if (load_model() != 0)
    return 1;

  if (load_texture() != 0)
    return 1;

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
  GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
}
