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
#include "gfx.h"
#include "bff.h"

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

static struct GFX_SHADER shader;
static struct GFX_FONT_SHADER font_shader;

static struct GFX_MESH *font_mesh;
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

static int load_models(void)
{
  struct BWF_READER bwf;
  if (open_bwf(&bwf, "data/world.bwf") != 0) {
    debug("** ERROR: can't open data/world.bmf\n");
    return 1;
  }
  if (load_bwf_room(&bwf, 5) != 0) {
    debug("** ERROR: can't load room\n");
    close_bwf(&bwf);
    return 1;
  }
  close_bwf(&bwf);

  if (load_bmf("data/player.bmf", GFX_MESH_TYPE_CREATURE, 0, NULL) != 0) {
    debug("** ERROR: can't load player model from data/player.bmf\n");
    return 1;
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

  font_mesh = gfx_upload_font(&font);
  
  free_font(&font);
  return (font_mesh != NULL) ? 0 : 1;
}

int render_setup(int width, int height)
{
  if (load_shader() != 0)
    return 1;

  if (load_models() != 0)
    return 1;
  
  if (load_font() != 0)
    return 1;

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
  set_camera_viewport(&game.camera, width, height);

  float text_base_size = 1.0 / 28.0;
  text_scale[0] = text_base_size * 0.5;
  text_scale[1] = text_base_size * width / height;
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
  case GFX_MESH_TYPE_ROOM:
    mat4_copy(mat_model, mesh->matrix);
    break;

  case GFX_MESH_TYPE_CREATURE:
    get_creature_model_matrix(mat_model, mesh->matrix, &game.creatures[mesh->info]);
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
  
  if (mesh->texture) {
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, mesh->texture->id));
  }
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

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // models
  float camera_pos[3];
  get_camera_pos(&game.camera, camera_pos);

  float light_pos[3];
  get_light_pos(light_pos);
  
  float mat_projection[16];
  get_camera_projection_matrix(&game.camera, mat_projection);

  float mat_view[16];
  get_camera_view_matrix(&game.camera, mat_view);
  
  float mat_view_projection[16];
  mat4_mul(mat_view_projection, mat_projection, mat_view);

  glEnable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(shader.id));
  GL_CHECK(glUniform1i(shader.uni_tex1, 0));
  GL_CHECK(glUniform3fv(shader.uni_light_pos, 1, light_pos));
  GL_CHECK(glUniform3fv(shader.uni_camera_pos, 1, camera_pos));
  for (int i = 0; i < num_gfx_meshes; i++) {
    if (gfx_meshes[i].use_count != 0)
      render_mesh(&gfx_meshes[i], mat_view_projection, mat_view);
  }

  // text
  glDisable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(font_shader.id));
  GL_CHECK(glUniform1i(font_shader.uni_tex1, 0));

  char fps[1024];
  snprintf(fps, sizeof(fps), "%4.1f fps", game.fps_counter.fps);
  render_text(0, 0, 1, fps, 0);
}
