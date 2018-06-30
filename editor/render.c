/* render.c */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "render.h"
#include "gl_error.h"
#include "matrix.h"
#include "shader.h"
#include "gfx.h"
#include "font.h"
#include "grid.h"
#include "camera.h"
#include "editor.h"

struct GFX_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_light_pos;
  GLint uni_camera_pos;
  GLint uni_mat_model_view_projection;
  GLint uni_mat_normal;
  GLint uni_mat_model;
  GLint uni_color;
};

struct GFX_FONT_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_text_scale;
  GLint uni_text_pos;
  GLint uni_text_color;
  GLint uni_char_uv;
};

struct GFX_GRID_SHADER {
  GLuint id;
  GLint uni_color;
  GLint uni_pos;
  GLint uni_mat_model_view_projection;
};

float projection_aspect = 1.0;
float projection_fovy = 50*M_PI/180;
int viewport_width;
int viewport_height;

static struct GFX_SHADER shader;
static struct GFX_FONT_SHADER font_shader;
static struct GFX_GRID_SHADER grid_shader;

static struct GFX_MESH *grid_mesh;

static struct GFX_MESH *font_mesh;
static float text_scale[2];
static float text_color[4];

static void render_format(float x, float y, float size, const char *fmt, ...) __attribute__((format(printf, 4, 5)));

static int load_shader(void)
{
  // model
  shader.id = load_program_shader("data/model_vert.glsl", "data/model_trans_frag.glsl");
  if (shader.id == 0)
    return 1;
  get_shader_uniform_id(shader.id, &shader.uni_tex1, "tex1");
  get_shader_uniform_id(shader.id, &shader.uni_light_pos, "light_pos");
  get_shader_uniform_id(shader.id, &shader.uni_camera_pos, "camera_pos");
  get_shader_uniform_id(shader.id, &shader.uni_mat_model_view_projection, "mat_model_view_projection");
  get_shader_uniform_id(shader.id, &shader.uni_mat_model, "mat_model");
  get_shader_uniform_id(shader.id, &shader.uni_mat_normal, "mat_normal");
  get_shader_uniform_id(shader.id, &shader.uni_color, "color");

  // font
  font_shader.id = load_program_shader("data/font_vert.glsl", "data/font_frag.glsl");
  if (font_shader.id == 0)
    return 1;
  get_shader_uniform_id(font_shader.id, &font_shader.uni_tex1, "tex1");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_scale, "text_scale");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_pos, "text_pos");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_text_color, "text_color");
  get_shader_uniform_id(font_shader.id, &font_shader.uni_char_uv, "char_uv");

  // grid
  grid_shader.id = load_program_shader("data/grid_vert.glsl", "data/grid_frag.glsl");
  if (grid_shader.id == 0)
    return 1;
  get_shader_uniform_id(grid_shader.id, &grid_shader.uni_color, "color");
  get_shader_uniform_id(grid_shader.id, &grid_shader.uni_mat_model_view_projection, "mat_model_view_projection");
  
  return 0;
}

static void set_text_color(float r, float g, float b, float a)
{
  vec4_load(text_color, r, g, b, a);
}

static int load_font(void)
{
  const char *font_filename = "data/font.png";
  
  struct FONT font;
  if (read_font(&font, font_filename) != 0) {
    debug("ERROR loading font file '%s'\n", font_filename);
    return 1;
  }

  font_mesh = gfx_upload_font(&font);
  
  free_font(&font);
  return (font_mesh != NULL) ? 0 : 1;
}

static int load_grid(void)
{
  struct MODEL_MESH *mesh = make_grid(81, 0.25);
  if (! mesh) {
    debug("ERROR creating grid mesh\n");
    return 1;
  }
  
  grid_mesh = gfx_upload_model_mesh(mesh, GFX_MESH_TYPE_GRID, 0, NULL);

  free_grid(mesh);
  return (grid_mesh != NULL) ? 0 : 1;
}

int render_setup(int width, int height)
{
  if (load_shader() != 0)
    return 1;

  if (load_font() != 0)
    return 1;

  if (load_grid() != 0)
    return 1;

  render_set_viewport(width, height);
  set_text_color(1, 1, 1, 1);

  glClearColor(0.2, 0.2, 0.2, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glDisable(GL_DEPTH_TEST);

  return 0;
}

void render_set_viewport(int width, int height)
{
  glViewport(0, 0, width, height);
  projection_aspect = (float) width / height;
  viewport_width = width;
  viewport_height = height;

  float text_base_size = 1.0 / 28.0;
  text_scale[0] = text_base_size * 0.5;
  text_scale[1] = text_base_size * projection_aspect;
}

static void render_mesh(struct GFX_MESH *mesh, float *mat_view_projection, float *mat_view)
{
  float mat_model[16];
  float color[4];
  switch (mesh->type) {
  case GFX_MESH_TYPE_ROOM:
    {
      struct EDITOR_ROOM *room = mesh->data;
      vec4_copy(color, room->display_color);
      
      mat4_copy(mat_model, mesh->matrix);
      mat_model[3]  += room->pos[0];
      mat_model[7]  += room->pos[1];
      mat_model[11] += room->pos[2];
    }
    break;

  default:
    return;
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
  if (shader.uni_color >= 0)
    GL_CHECK(glUniform4fv(shader.uni_color, 1, color));
  
  if (mesh->texture) {
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, mesh->texture->id));
  }
  GL_CHECK(glBindVertexArray(mesh->vtx_array_obj));
  GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->index_count, mesh->index_type, 0));
}

static void render_grid(struct GFX_MESH *mesh, float *mat_view_projection)
{
  float mat_model[16];
  mat4_load_translation(mat_model, editor.grid_pos[0], editor.grid_pos[1], editor.grid_pos[2]);
  
  float mat_model_view_projection[16];
  mat4_mul(mat_model_view_projection, mat_view_projection, mat_model);

  if (grid_shader.uni_mat_model_view_projection >= 0)
    GL_CHECK(glUniformMatrix4fv(grid_shader.uni_mat_model_view_projection, 1, GL_TRUE, mat_model_view_projection));
  
  GL_CHECK(glBindVertexArray(mesh->vtx_array_obj));
  GL_CHECK(glDrawElements(GL_LINES, mesh->index_count, mesh->index_type, 0));
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
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, font_mesh->texture->id));
  GL_CHECK(glUniform2fv(font_shader.uni_text_scale, 1, uni_scale));
  GL_CHECK(glUniform4fv(font_shader.uni_text_color, 1, text_color));
  GL_CHECK(glBindVertexArray(font_mesh->vtx_array_obj));

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
    
    GL_CHECK(glDrawElements(GL_TRIANGLES, n_chars * 6, font_mesh->index_type, 0));
  }
}

static void render_format(float x, float y, float size, const char *fmt, ...)
{
  static char buf[1024];
  
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  render_text(x, y, size, buf, 0);
}

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float camera_pos[3];
  get_camera_pos(&editor.camera, camera_pos);

  float light_pos[3] = { 10.0, 10.0, 0.0 };
  
  float mat_projection[16];
  mat4_inf_perspective(mat_projection, projection_aspect, projection_fovy, 0.1);

  float mat_view[16];
  get_camera_matrix(&editor.camera, mat_view);
  
  float mat_view_projection[16];
  mat4_mul(mat_view_projection, mat_projection, mat_view);

  //glEnable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(grid_shader.id));
  GL_CHECK(glUniform4fv(grid_shader.uni_color, 1, editor.grid_color));
  render_grid(grid_mesh, mat_view_projection);

  GL_CHECK(glUseProgram(shader.id));
  GL_CHECK(glUniform1i(shader.uni_tex1, 0));
  GL_CHECK(glUniform3fv(shader.uni_light_pos, 1, light_pos));
  GL_CHECK(glUniform3fv(shader.uni_camera_pos, 1, camera_pos));
  for (int i = 0; i < num_gfx_meshes; i++) {
    if (gfx_meshes[i].use_count != 0)
      render_mesh(&gfx_meshes[i], mat_view_projection, mat_view);
  }

  //glDisable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(font_shader.id));
  GL_CHECK(glUniform1i(font_shader.uni_tex1, 0));
  if (editor.selected_room) {
    struct EDITOR_ROOM *room = editor.selected_room;
    set_text_color(1, 1, 1, 1);
    render_format(0, 0, 0.75, "(%+7.2f,%+7.2f,%+7.2f) %s",
                  room->pos[0],
                  room->pos[1],
                  room->pos[2],
                  room->name);
    set_text_color(0.5, 0.5, 1, 1);
    for (int i = 0; i < room->n_neighbors; i++)
      render_format(0, 1+i, 0.75, "+ %s", room->neighbors[i]->name);
  }
  set_text_color(1, 1, 1, 0.75);
  render_format(110, 0, 0.75, "view (%+7.2f,%+7.2f,%+7.2f)",
                editor.camera.center[0],
                editor.camera.center[1],
                editor.camera.center[2]);
  set_text_color(1, 1, 1, 0.3);
  for (int i = 0; i < 10; i++)
    render_text(0, 21+i, 1, editor.text_screen[EDITOR_SCREEN_LINES-10+i], EDITOR_SCREEN_COLS);
  if (editor.input.active) {
    set_text_color(1, 1, 0, 1); render_text(0, 30, 1, ">", 0);
    set_text_color(1, 1, 1, 1); render_text(2, 30, 1, editor.input.line, 0);
    set_text_color(1, 0, 0, 1); render_text(2 + editor.input.cursor_pos, 30, 1, "_", 0);
  }
}
