/* render.c */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "render.h"
#include "shader.h"
#include "model.h"
#include "font.h"
#include "matrix.h"
#include "camera.h"
#include "game.h"

#define PROJECTION_FOV  (M_PI*75/180)

#define GFX_MESH_TYPE_CREATURE 0
#define GFX_MESH_TYPE_WORLD    1

struct GFX_MESH {
  GLuint vtx_array_obj;
  GLuint vtx_buf_obj;
  GLuint index_buf_obj;
  GLuint texture_id;
  
  uint32_t index_count;
  uint32_t index_type;
  float matrix[16];
  uint32_t type;
  uint32_t info;
};

struct GFX_TEXTURE {
  GLuint id;

  int used;
};

struct GFX_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_light_pos;
  GLint uni_camera_pos;
  GLint uni_mat_model_view_projection;
  GLint uni_mat_normal;
  GLint uni_mat_model;
};

struct GFX_FONT_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_text_scale;
  GLint uni_text_pos;
  GLint uni_text_color;
  GLint uni_char_uv;
};

#define MAX_MESHES 16
#define MAX_TEXTURES 16

static int n_meshes;
static struct GFX_MESH meshes[MAX_MESHES];
static struct GFX_TEXTURE textures[MAX_TEXTURES];
static struct GFX_SHADER shader;

static struct GFX_MESH font_mesh;
static struct GFX_TEXTURE font_texture;
static struct GFX_FONT_SHADER font_shader;
static float text_scale[2];
static float text_color[4];

static int load_shader(void)
{
  shader.id = load_program_shader("data/model_vert.glsl", "data/model_frag.glsl");
  if (shader.id == 0)
    return 1;
  get_shader_uniform_id(shader.id, &shader.uni_tex1, "tex1");
  get_shader_uniform_id(shader.id, &shader.uni_light_pos, "light_pos");
  get_shader_uniform_id(shader.id, &shader.uni_camera_pos, "camera_pos");
  get_shader_uniform_id(shader.id, &shader.uni_mat_model_view_projection, "mat_model_view_projection");
  get_shader_uniform_id(shader.id, &shader.uni_mat_model, "mat_model");
  get_shader_uniform_id(shader.id, &shader.uni_mat_normal, "mat_normal");

  font_shader.id = load_program_shader("data/font_vert.glsl", "data/font_frag.glsl");
  if (font_shader.id == 0)
    return 1;
  get_shader_uniform_id(font_shader.id, &font_shader.uni_tex1, "tex1");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_scale, "text_scale");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_pos, "text_pos");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_color, "text_color");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_char_uv, "char_uv");

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
  gfx->used = 1;

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (texture->n_chan == 3)
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  texture->width, texture->height, 0, GL_RGB,  GL_UNSIGNED_BYTE, texture->data));
  else
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data));
  
  GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  return 0;
}

static int upload_model(struct MODEL *model, uint32_t type, uint32_t info)
{
  if (n_meshes + model->n_meshes >= MAX_MESHES)
    return -1;
  
  int first_mesh = n_meshes;
  int first_free_gfx_texture = -1;
  for (int i = 0; i < MAX_TEXTURES; i++) {
    if (! textures[i].used) {
      first_free_gfx_texture = i;
      break;
    }
  }
  if (first_free_gfx_texture < 0)
    return -1;
  
  for (int i = 0; i < model->n_meshes; i++) {
    upload_model_mesh(model->meshes[i], &meshes[n_meshes]);
    int model_tex_index = model->meshes[i]->tex0_index;
    int gfx_tex_index = first_free_gfx_texture + model->meshes[i]->tex0_index;
    if (! textures[gfx_tex_index].used)
      upload_model_texture(&model->textures[model_tex_index], &textures[gfx_tex_index]);
    meshes[n_meshes].texture_id = textures[gfx_tex_index].id;
    meshes[n_meshes].type = type;
    meshes[n_meshes].info = info;
    
    n_meshes++;
  }

  return first_mesh;
}

static int load_models(void)
{
  struct {
    uint32_t type;
    uint32_t info;
    char *filename;
  } model_info[] = {
    { GFX_MESH_TYPE_WORLD,    0, "data/world.glb" },
    { GFX_MESH_TYPE_CREATURE, 0, "data/player.glb" },
    { 0, 0, NULL }
  };

  n_meshes = 0;
  for (int i = 0; i < MAX_TEXTURES; i++)
    textures[i].used = 0;

  struct MODEL model;
  for (int i = 0; model_info[i].filename != NULL; i++) {
    if (read_glb_model(&model, model_info[i].filename) != 0) {
      debug("ERROR reading file '%s'\n", model_info[i].filename);
      return 1;
    }
    if (upload_model(&model, model_info[i].type, model_info[i].info) < 0) {
      debug("ERROR uploading model '%s'\n", model_info[i].filename);
      return 1;
    }
    free_model(&model);
  }

  return 0;
}

static int load_font(void)
{
  const char *font_filename = "data/font.png";
  
  struct FONT font;
  if (read_font(&font, font_filename) != 0) {
    debug("ERROR loading font file '%s'\n", font_filename);
    return 1;
  }

  upload_model_mesh(font.mesh, &font_mesh);
  upload_model_texture(&font.texture, &font_texture);
  font_mesh.texture_id = font_texture.id;
  
  free_font(&font);
  return 0;
}

int render_setup(int width, int height)
{
  if (load_shader() != 0)
    return 1;

  if (load_models() != 0)
    return 1;

  if (load_font() != 0)
    return 1;

  init_camera(&camera);
  camera.distance = 4.0;

  render_set_viewport(width, height);

  vec4_load(text_color, 1,1,1,1);
  
  glClearColor(0.0, 0.0, 0.4, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  
  return 0;
}

void render_set_viewport(int width, int height)
{
  glViewport(0, 0, width, height);
  projection_aspect = (float) width / height;

  float text_base_size = 1.0 / 28.0;
  text_scale[0] = text_base_size * 0.5;
  text_scale[1] = text_base_size * projection_aspect;
}

static void get_creature_model_matrix(float *restrict mat_model, float *restrict mesh_matrix, struct CREATURE *creature)
{
  float rot[16];
  mat4_load_rot_y(rot, creature->theta);
  mat4_mul(mat_model, mesh_matrix, rot);
  
  mat_model[ 3] += creature->pos[0];
  mat_model[ 7] += creature->pos[1];
  mat_model[11] += creature->pos[2];
}

static void render_mesh(struct GFX_MESH *mesh, float *mat_view_projection, float *mat_view)
{
  float mat_model[16];
  switch (mesh->type) {
  case GFX_MESH_TYPE_WORLD:
    mat4_copy(mat_model, mesh->matrix);
    break;

  case GFX_MESH_TYPE_CREATURE:
    get_creature_model_matrix(mat_model, mesh->matrix, &creatures[mesh->info]);
    break;

  default:
    mat4_copy(mat_model, mesh->matrix);
    break;
  }
  
  float mat_model_view_projection[16];
  mat4_mul(mat_model_view_projection, mat_view_projection, mat_model);

  float mat_inv[16], mat_normal[16];
  mat4_inverse(mat_inv, mat_model);
  mat4_transpose(mat_normal, mat_inv);

  if (shader.uni_mat_model_view_projection >= 0)
    GL_CHECK(glUniformMatrix4fv(shader.uni_mat_model_view_projection, 1, GL_TRUE, mat_model_view_projection));
  if (shader.uni_mat_normal >= 0)
    GL_CHECK(glUniformMatrix4fv(shader.uni_mat_normal, 1, GL_TRUE, mat_normal));
  if (shader.uni_mat_model >= 0)
    GL_CHECK(glUniformMatrix4fv(shader.uni_mat_model, 1, GL_TRUE, mat_model));
  
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, mesh->texture_id));
  GL_CHECK(glBindVertexArray(mesh->vtx_array_obj));
  GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->index_count, mesh->index_type, 0));
}

static void render_text(float x, float y, float size, const char *text, size_t len)
{
  float char_uv[2*FONT_MAX_CHARS_PER_DRAW];
  float text_pos[2];

  const float delta_u = 1.0 / 16.0;
  const float delta_v = 1.0 / 8.0;

  memset(char_uv, 0, sizeof(char_uv));

  float size_x = text_scale[0] * size;
  float size_y = text_scale[1] * size;
  float uni_scale[2] = { size_x, -size_y };
  
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, font_mesh.texture_id));
  GL_CHECK(glUniform2fv(font_shader.uni_text_scale, 1, uni_scale));
  GL_CHECK(glUniform4fv(font_shader.uni_text_color, 1, text_color));
  GL_CHECK(glBindVertexArray(font_mesh.vtx_array_obj));

  if (len == 0)
    len = strlen(text);
  
  float pos_x = x;
  float pos_y = y;
  const char *cur_text = text;
  const char *end_text = text + len;
  while (*cur_text && cur_text < end_text) {
    text_pos[0] = pos_x - 1.0 / size_x;
    text_pos[1] = pos_y - 1.0 / size_y;
    
    int n_chars = 0;
    for (int i = 0; i < FONT_MAX_CHARS_PER_DRAW; i++) {
      if (cur_text >= end_text)
        break;
      unsigned char ch = *cur_text;
      if (ch == '\0')
        break;
      cur_text++;
      if (ch == '\n') {
        pos_y += 1.0;
        pos_x = x;
        break;
      }
      if (ch < 32 || ch >= 128)
        ch = '?';
      char_uv[2*i+0] = ((ch - 32) % 16) * delta_u;
      char_uv[2*i+1] = ((ch - 32) / 16) * delta_v;
      n_chars++;
      pos_x += 1.0;
    }

    GL_CHECK(glUniform2fv(font_shader.uni_text_pos, 1, text_pos));
    GL_CHECK(glUniform2fv(font_shader.uni_char_uv, FONT_MAX_CHARS_PER_DRAW, char_uv));
    
    GL_CHECK(glDrawElements(GL_TRIANGLES, n_chars * 6, font_mesh.index_type, 0));
  }
}

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // models
  float camera_pos[3];
  get_camera_pos(&camera, camera_pos);

  float light_pos[3];
  get_light_pos(light_pos);
  
  glEnable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(shader.id));
  GL_CHECK(glUniform1i(shader.uni_tex1, 0));
  GL_CHECK(glUniform3fv(shader.uni_light_pos, 1, light_pos));
  GL_CHECK(glUniform3fv(shader.uni_camera_pos, 1, camera_pos));

  float mat_projection[16];
  mat4_inf_perspective(mat_projection, projection_aspect, projection_fov, 0.1);

  float mat_view[16];
  get_camera_matrix(&camera, mat_view);
  
  float mat_view_projection[16];
  mat4_mul(mat_view_projection, mat_projection, mat_view);

  for (int i = 0; i < n_meshes; i++)
    render_mesh(&meshes[i], mat_view_projection, mat_view);

  // text
  glDisable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(font_shader.id));
  GL_CHECK(glUniform1i(font_shader.uni_tex1, 0));

  char fps[1024];
  snprintf(fps, sizeof(fps), "%4.1f fps", fps_counter.fps);
  render_text(0, 0, 1, fps, 0);
}
