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
#include "skeleton.h"
#include "room.h"

struct RENDER_MODEL {
  struct RENDER_MODEL *next;
  int use_count;
  struct SKELETON skel;
  int n_gfx_meshes;
  struct GFX_MESH *gfx_meshes[MODEL_MAX_MESHES];
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

struct GFX_ANIM_SHADER {
  struct GFX_SHADER base;
  GLint uni_mat_bones;
};

struct GFX_FONT_SHADER {
  GLuint id;
  GLint uni_tex1;
  GLint uni_text_scale;
  GLint uni_text_pos;
  GLint uni_text_color;
  GLint uni_char_uv;
};

static struct RENDER_MODEL *render_models_free_list;
static struct RENDER_MODEL *render_models_used_list;
static struct RENDER_MODEL render_models[MAX_RENDER_MODELS];

static struct RENDER_MODEL_INSTANCE *render_model_instances_free_list;
static struct RENDER_MODEL_INSTANCE *render_model_instances_used_list;
static struct RENDER_MODEL_INSTANCE render_model_instances[MAX_RENDER_MODEL_INSTANCES];

static struct GFX_SHADER shader;
static struct GFX_ANIM_SHADER anim_shader;
static struct GFX_FONT_SHADER font_shader;

static struct GFX_MESH *font_mesh;
static float text_scale[2];
static float text_color[4];

static void init_render_lists(void)
{
  for (int i = 0; i < MAX_RENDER_MODELS-1; i++)
    render_models[i].next = &render_models[i+1];
  render_models[MAX_RENDER_MODELS-1].next = NULL;
  render_models_free_list = &render_models[0];
  render_models_used_list = NULL;

  for (int i = 0; i < MAX_RENDER_MODEL_INSTANCES-1; i++)
    render_model_instances[i].next = &render_model_instances[i+1];
  render_model_instances[MAX_RENDER_MODEL_INSTANCES-1].next = NULL;
  render_model_instances_free_list = &render_model_instances[0];
  render_model_instances_used_list = NULL;
}

struct RENDER_MODEL *alloc_render_model(void)
{
  struct RENDER_MODEL *model = render_models_free_list;
  if (model == NULL)
    return NULL;
  render_models_free_list = model->next;
  model->next = render_models_used_list;
  render_models_used_list = model;

  model->n_gfx_meshes = 0;
  init_skeleton(&model->skel, 0, 0);
  return model;
}

void set_render_model_meshes(struct RENDER_MODEL *model, int n_meshes, struct GFX_MESH **meshes)
{
  model->n_gfx_meshes = n_meshes;
  for (int i = 0; i < n_meshes; i++)
    model->gfx_meshes[i] = meshes[i];
}

struct SKELETON *get_render_model_skeleton(struct RENDER_MODEL *model)
{
  return &model->skel;
}

void free_render_model(struct RENDER_MODEL *model)
{
  free_skeleton(&model->skel);
  
  // remove from used list
  struct RENDER_MODEL **p = &render_models_used_list;
  while (*p && *p != model)
    p = &(*p)->next;
  if (! *p) {
    debug("** ERROR: trying to free unused render model\n");
    return;
  }
  *p = model->next;
  
  // add to free list
  model->next = render_models_free_list;
  render_models_free_list = model;
}

struct RENDER_MODEL_INSTANCE *alloc_render_model_instance(struct RENDER_MODEL *model)
{
  struct RENDER_MODEL_INSTANCE *inst = render_model_instances_free_list;
  if (inst == NULL)
    return NULL;
  render_model_instances_free_list = inst->next;
  inst->next = render_model_instances_used_list;
  render_model_instances_used_list = inst;

  inst->model = model;
  inst->anim = NULL;
  model->use_count++;
  return inst;
}

void free_render_model_instance(struct RENDER_MODEL_INSTANCE *inst)
{
  inst->model->use_count--;
  
  // remove from used list
  struct RENDER_MODEL_INSTANCE **p = &render_model_instances_used_list;
  while (*p && *p != inst)
    p = &(*p)->next;
  if (! *p) {
    debug("** ERROR: trying to free unused render model\n");
    return;
  }
  *p = inst->next;
  
  // add to free list
  inst->next = render_model_instances_free_list;
  render_model_instances_free_list = inst;
}

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

  anim_shader.base.id = load_program_shader("data/model_anim_vert.glsl", "data/model_frag.glsl");
  if (anim_shader.base.id == 0)
    return 1;
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_tex1, "tex1");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_light_pos, "light_pos");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_camera_pos, "camera_pos");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_mat_model_view_projection, "mat_model_view_projection");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_mat_model, "mat_model");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.base.uni_mat_normal, "mat_normal");
  get_shader_uniform_id(anim_shader.base.id, &anim_shader.uni_mat_bones, "mat_bones");
  
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
  init_gfx();
  init_render_lists();
  
  if (load_shader() != 0)
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

static void render_mesh(struct GFX_SHADER *shader, struct GFX_MESH *mesh, float *mat_view_projection, float *mat_view, float *mat_model)
{
  if (mesh->texture && (mesh->texture->flags & GFX_TEX_FLAG_LOADED) == 0)
    return;
  
  float mat_model_view_projection[16];
  mat4_mul(mat_model_view_projection, mat_view_projection, mat_model);

  float mat_inv[16], mat_normal[16];
  mat4_inverse(mat_inv, mat_model);
  mat4_transpose(mat_normal, mat_inv);

  if (shader->uni_mat_model_view_projection >= 0)
    GL_CHECK(glUniformMatrix4fv(shader->uni_mat_model_view_projection, 1, GL_TRUE, mat_model_view_projection));
  if (shader->uni_mat_normal >= 0)
    GL_CHECK(glUniformMatrix4fv(shader->uni_mat_normal, 1, GL_TRUE, mat_normal));
  if (shader->uni_mat_model >= 0)
    GL_CHECK(glUniformMatrix4fv(shader->uni_mat_model, 1, GL_TRUE, mat_model));
  
  if (mesh->texture) {
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, mesh->texture->id));
  }
  GL_CHECK(glBindVertexArray(mesh->vtx_array_obj));
  GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->index_count, mesh->index_type, 0));
}

static void render_room_mesh(struct GFX_MESH *mesh, float *mat_view_projection, float *mat_view)
{
  struct ROOM *room = mesh->data;
  float mat_model[16];
  mat4_copy(mat_model, mesh->matrix);
  mat_model[ 3] += room->pos[0];
  mat_model[ 7] += room->pos[1];
  mat_model[11] += room->pos[2];

  render_mesh(&shader, mesh, mat_view_projection, mat_view, mat_model);
}

static void render_model_instance(struct RENDER_MODEL_INSTANCE *inst, float *mat_view_projection, float *mat_view)
{
  struct GFX_SHADER *model_shader;
  if (inst->anim) {
    model_shader = &anim_shader.base;
#if 0
    console("%d bones:\n", inst->anim->skel->n_bones);
    for (int i = 0; i < inst->anim->skel->n_bones; i++) {
      console("bone [%d]\n", i);
      mat4_dump(&inst->anim->matrices[16*i]);
    }
    for (int i = 0; i < inst->model->n_gfx_meshes; i++) {
      console("mesh[%d] matrix:\n", i);
      mat4_dump(inst->model->gfx_meshes[i]->matrix);
    }
    console("inst matrix:\n");
    mat4_dump(inst->matrix);
#endif
    GL_CHECK(glUniformMatrix4fv(anim_shader.uni_mat_bones, inst->anim->skel->n_bones, GL_TRUE, inst->anim->matrices));
  } else {
    model_shader = &shader;
  }
  
  struct RENDER_MODEL *model = inst->model;
  for (int i = 0; i < model->n_gfx_meshes; i++) {
    struct GFX_MESH *gfx_mesh = model->gfx_meshes[i];
    float mat_model[16];

    if (inst->anim)
      mat4_copy(mat_model, inst->matrix);
    else
      mat4_mul(mat_model, inst->matrix, gfx_mesh->matrix);
    render_mesh(model_shader, gfx_mesh, mat_view_projection, mat_view, mat_model);
  }
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
  for (int i = 0; i < NUM_GFX_MESHES; i++) {
    if (gfx_meshes[i].use_count != 0 && gfx_meshes[i].type == GFX_MESH_TYPE_ROOM)
      render_room_mesh(&gfx_meshes[i], mat_view_projection, mat_view);
  }
  for (struct RENDER_MODEL_INSTANCE *inst = render_model_instances_used_list; inst != NULL; inst = inst->next) {
    if (! inst->anim)
      render_model_instance(inst, mat_view_projection, mat_view);
  }

  GL_CHECK(glUseProgram(anim_shader.base.id));
  GL_CHECK(glUniform1i(anim_shader.base.uni_tex1, 0));
  GL_CHECK(glUniform3fv(anim_shader.base.uni_light_pos, 1, light_pos));
  GL_CHECK(glUniform3fv(anim_shader.base.uni_camera_pos, 1, camera_pos));
  for (struct RENDER_MODEL_INSTANCE *inst = render_model_instances_used_list; inst != NULL; inst = inst->next) {
    if (inst->anim)
      render_model_instance(inst, mat_view_projection, mat_view);
  }
  
  // text
  glDisable(GL_DEPTH_TEST);
  GL_CHECK(glUseProgram(font_shader.id));
  GL_CHECK(glUniform1i(font_shader.uni_tex1, 0));

  char text[1024];
  snprintf(text, sizeof(text), "%4.1f fps", fps_counter.fps);
  render_text(0, 0, 1, text, 0);

  if (game.show_camera_info) {
    snprintf(text, sizeof(text), "cam.dist=+%f, cam.theta=%+f, cam.phi=%+f, fov=%+f\n",
             game.camera.distance, game.camera.theta, game.camera.phi, game.camera.fovy/M_PI*180);
    render_text(0, 1, 1, text, 0);
  }
}
